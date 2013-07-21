/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdio.h>

//Debug
#include "rprintf.h"

#include "partition.h"
#include "fat16.h"
#include "fat16_config.h"
#include "sd_raw.h"

#include <stdlib.h>
#include <string.h>

/**
 * \addtogroup fat16 FAT16 support
 *
 * This module implements FAT16 read and write access.
 *
 * The following features are supported:
 * - File names up to 31 characters long.
 * - Unlimited depth of subdirectories.
 * - Short 8.3 and long filenames.
 * - Creating and deleting files.
 * - Reading and writing from and to files.
 * - File resizing.
 * - File sizes of up to 4 gigabytes.
 *
 * @{
 */
/**
 * \file
 * FAT16 implementation.
 *
 * \author Roland Riegel
 */

/**
 * \addtogroup fat16_config FAT16 configuration
 * Preprocessor defines to configure the FAT16 implementation.
 */

/**
 * \addtogroup fat16_fs FAT16 access
 * Basic functions for handling a FAT16 filesystem.
 */

/**
 * \addtogroup fat16_file FAT16 file functions
 * Functions for managing files.
 */

/**
 * \addtogroup fat16_dir FAT16 directory functions
 * Functions for managing directories.
 */

/**
 * @}
 */

#define FAT16_CLUSTER_FREE 0x0000
#define FAT16_CLUSTER_RESERVED_MIN (uint16_t)0xfff0
#define FAT16_CLUSTER_RESERVED_MAX (uint16_t)0xfff6
#define FAT16_CLUSTER_BAD (uint16_t)0xfff7
#define FAT16_CLUSTER_LAST_MIN (uint16_t)0xfff8
#define FAT16_CLUSTER_LAST_MAX (uint16_t)0xffff

#define FAT16_DIRENTRY_DELETED 0xe5
#define FAT16_DIRENTRY_LFNLAST (1 << 6)
#define FAT16_DIRENTRY_LFNSEQMASK ((1 << 6) - 1)

/* Each entry within the directory table has a size of 32 bytes
 * and either contains a 8.3 DOS-style file name or a part of a
 * long file name, which may consist of several directory table
 * entries at once.
 *
 * multi-byte integer values are stored little-endian!
 *
 * 8.3 file name entry:
 * ====================
 * offset  length  description
 *      0       8  name (space padded)
 *      8       3  extension (space padded)
 *     11       1  attributes (FAT16_ATTRIB_*)
 
 *     0x0E     2  Creation Time
 *     0x10     2  Creation Date
 
 *
 * long file name (lfn) entry ordering for a single file name:
 * ===========================================================
 * LFN entry n
 *     ...
 * LFN entry 2
 * LFN entry 1
 * 8.3 entry (see above)
 *
 * lfn entry:
 * ==========
 * offset  length  description
 *      0       1  ordinal field
 *      1       2  unicode character 1
 *      3       3  unicode character 2
 *      5       3  unicode character 3
 *      7       3  unicode character 4
 *      9       3  unicode character 5
 *     11       1  attribute (always 0x0f)
 *     12       1  type (reserved, always 0)
 *     13       1  checksum
 *     14       2  unicode character 6
 *     16       2  unicode character 7
 *     18       2  unicode character 8
 *     20       2  unicode character 9
 *     22       2  unicode character 10
 *     24       2  unicode character 11
 *     26       2  cluster (unused, always 0)
 *     28       2  unicode character 12
 *     30       2  unicode character 13
 *
 * The ordinal field contains a descending number, from n to 1.
 * For the n'th lfn entry the ordinal field is or'ed with 0x40.
 * For deleted lfn entries, the ordinal field is set to 0xe5.
 */

struct fat16_header_struct
{
    uint32_t size;

    uint32_t fat_offset;
    uint32_t fat_size;

    uint16_t sector_size;
    uint16_t cluster_size;

    uint32_t root_dir_offset;

    uint32_t cluster_zero_offset;
};

struct fat16_fs_struct
{
    struct partition_struct* partition;
    struct fat16_header_struct header;
};

struct fat16_file_struct
{
    struct fat16_fs_struct* fs;
    struct fat16_dir_entry_struct dir_entry;
    uint32_t pos;
    uint16_t pos_cluster;
};

struct fat16_dir_struct
{
    struct fat16_fs_struct* fs;
    struct fat16_dir_entry_struct dir_entry;
    uint16_t entry_next;
};

struct fat16_read_callback_arg
{
    uint16_t entry_cur;
    uint16_t entry_num;
    uint32_t entry_offset;
    uint8_t byte_count;
};

struct fat16_usage_count_callback_arg
{
    uint16_t cluster_count;
    uint8_t buffer_size;
};

static uint8_t fat16_read_header(struct fat16_fs_struct* fs);
static uint8_t fat16_read_root_dir_entry(const struct fat16_fs_struct* fs, uint16_t entry_num, struct fat16_dir_entry_struct* dir_entry);
static uint8_t fat16_read_sub_dir_entry(const struct fat16_fs_struct* fs, uint16_t entry_num, const struct fat16_dir_entry_struct* parent, struct fat16_dir_entry_struct* dir_entry);
static uint8_t fat16_dir_entry_seek_callback(uint8_t* buffer, uint32_t offset, void* p);
static uint8_t fat16_dir_entry_read_callback(uint8_t* buffer, uint32_t offset, void* p);
static uint8_t fat16_interpret_dir_entry(struct fat16_dir_entry_struct* dir_entry, const uint8_t* raw_entry);
static uint16_t fat16_get_next_cluster(const struct fat16_fs_struct* fs, uint16_t cluster_num);
static uint16_t fat16_append_clusters(const struct fat16_fs_struct* fs, uint16_t cluster_num, uint16_t count);
static uint8_t fat16_free_clusters(const struct fat16_fs_struct* fs, uint16_t cluster_num);
static uint8_t fat16_terminate_clusters(const struct fat16_fs_struct* fs, uint16_t cluster_num);
static uint8_t fat16_write_dir_entry(const struct fat16_fs_struct* fs, const struct fat16_dir_entry_struct* dir_entry);

static uint8_t fat16_get_fs_free_callback(uint8_t* buffer, uint32_t offset, void* p);

/**
 * \ingroup fat16_fs
 * Opens a FAT16 filesystem.
 *
 * \param[in] partition Discriptor of partition on which the filesystem resides.
 * \returns 0 on error, a FAT16 filesystem descriptor on success.
 * \see fat16_open
 */
struct fat16_fs_struct* fat16_open(struct partition_struct* partition)
{
    if(!partition ||
        #if FAT16_WRITE_SUPPORT
        !partition->device_write
       #else
        0
       #endif
       )
    return 0;

    struct fat16_fs_struct* fs = malloc(sizeof(*fs));
    if(!fs)
    {
        rprintf("MALLOC FAILS\n\r");
        return 0;
    }
    memset(fs, 0, sizeof(*fs));

    fs->partition = partition;
    if(!fat16_read_header(fs))
    {
        rprintf("Failed Reading Header\n\r");
        free(fs);
        return 0;
    }

    return fs;
}

/**
 * \ingroup fat16_fs
 * Closes a FAT16 filesystem.
 *
 * When this function returns, the given filesystem descriptor
 * will be invalid.
 *
 * \param[in] fs The filesystem to close.
 * \see fat16_open
 */
void fat16_close(struct fat16_fs_struct* fs)
{
    if(!fs)
        return;

    free(fs);
}

/**
 * \ingroup fat16_fs
 * Reads and parses the header of a FAT16 filesystem.
 *
 * \param[inout] fs The filesystem for which to parse the header.
 * \returns 0 on failure, 1 on success.
 */
uint8_t fat16_read_header(struct fat16_fs_struct* fs)
{
    if(!fs)
        return 0;

    struct partition_struct* partition = fs->partition;
    if(!partition)
    {
        rprintf("Partition = 0\n\r");
        return 0;
    }

    /* read fat parameters */
    uint8_t buffer[25];
    uint32_t partition_offset = partition->offset * 512;

    if(!partition->device_read(partition_offset + 0x0b, buffer, sizeof(buffer)))
        return 0;

    uint16_t bytes_per_sector = ((uint16_t) buffer[0x00]) |
    ((uint16_t) buffer[0x01] << 8);
    uint8_t sectors_per_cluster = buffer[0x02];
    uint16_t reserved_sectors = ((uint16_t) buffer[0x03]) |
    ((uint16_t) buffer[0x04] << 8);
    uint8_t fat_copies = buffer[0x05];
    uint16_t max_root_entries = ((uint16_t) buffer[0x06]) |
    ((uint16_t) buffer[0x07] << 8);
    uint16_t sector_count_16 = ((uint16_t) buffer[0x08]) |
    ((uint16_t) buffer[0x09] << 8);
    uint16_t sectors_per_fat = ((uint16_t) buffer[0x0b]) |
    ((uint16_t) buffer[0x0c] << 8);
    uint32_t sector_count = ((uint32_t) buffer[0x15]) |
    ((uint32_t) buffer[0x16] << 8) |
    ((uint32_t) buffer[0x17] << 16) |
    ((uint32_t) buffer[0x18] << 24);

    if(sectors_per_fat == 0)
    /* this is not a FAT16 */
        return 0;

    if(sector_count == 0)
    {
        if(sector_count_16 == 0)
    /* illegal volume size */
            return 0;
        else
            sector_count = sector_count_16;
    }

    /* ensure we really have a FAT16 fs here */
    uint32_t data_sector_count = sector_count
    - reserved_sectors
    - (uint32_t) sectors_per_fat * fat_copies
    - ((max_root_entries * 32 + bytes_per_sector - 1) / bytes_per_sector);
    uint32_t data_cluster_count = data_sector_count / sectors_per_cluster;
    if(data_cluster_count < 4085 || data_cluster_count >= 65525)
    /* this is not a FAT16 */
        return 0;

    partition->type = PARTITION_TYPE_FAT16;

    /* fill header information */
    struct fat16_header_struct* header = &fs->header;
    memset(header, 0, sizeof(*header));

    header->size = sector_count * bytes_per_sector;

    header->fat_offset = /* jump to partition */
    partition_offset +
    /* jump to fat */
    (uint32_t) reserved_sectors * bytes_per_sector;
    header->fat_size = (data_cluster_count + 2) * 2;

    header->sector_size = bytes_per_sector;
    header->cluster_size = (uint32_t) bytes_per_sector * sectors_per_cluster;

    header->root_dir_offset = /* jump to fats */
    header->fat_offset +
    /* jump to root directory entries */
    (uint32_t) fat_copies * sectors_per_fat * bytes_per_sector;

    header->cluster_zero_offset = /* jump to root directory entries */
    header->root_dir_offset +
    /* skip root directory entries */
    (uint32_t) max_root_entries * 32;

    return 1;
}

/**
 * \ingroup fat16_fs
 * Reads a directory entry of the root directory.
 *
 * \param[in] fs Descriptor of file system to use.
 * \param[in] entry_num The index of the directory entry to read.
 * \param[out] dir_entry Directory entry descriptor which will get filled.
 * \returns 0 on failure, 1 on success
 * \see fat16_read_sub_dir_entry, fat16_read_dir_entry_by_path
 */
uint8_t fat16_read_root_dir_entry(const struct fat16_fs_struct* fs, uint16_t entry_num, struct fat16_dir_entry_struct* dir_entry)
{
    if(!fs || !dir_entry)
        return 0;

    /* we read from the root directory entry */
    const struct fat16_header_struct* header = &fs->header;
    device_read_interval_t device_read_interval = fs->partition->device_read_interval;
    uint8_t buffer[32];

    /* seek to the n-th entry */
    struct fat16_read_callback_arg arg;
    memset(&arg, 0, sizeof(arg));
    arg.entry_num = entry_num;
    if(!device_read_interval(header->root_dir_offset,
        buffer,
       sizeof(buffer),
       header->cluster_zero_offset - header->root_dir_offset,
       fat16_dir_entry_seek_callback,
       &arg) ||
       arg.entry_offset == 0
       )
    return 0;

    /* read entry */
    memset(dir_entry, 0, sizeof(*dir_entry));
    if(!device_read_interval(arg.entry_offset,
        buffer,
       sizeof(buffer),
       arg.byte_count,
       fat16_dir_entry_read_callback,
       dir_entry))
    return 0;

    return dir_entry->long_name[0] != '\0' ? 1 : 0;
}

/**
 * \ingroup fat16_fs
 * Reads a directory entry of a given parent directory.
 *
 * \param[in] fs Descriptor of file system to use.
 * \param[in] entry_num The index of the directory entry to read.
 * \param[in] parent Directory entry descriptor in which to read directory entry.
 * \param[out] dir_entry Directory entry descriptor which will get filled.
 * \returns 0 on failure, 1 on success
 * \see fat16_read_root_dir_entry, fat16_read_dir_entry_by_path
 */
uint8_t fat16_read_sub_dir_entry(const struct fat16_fs_struct* fs, uint16_t entry_num, const struct fat16_dir_entry_struct* parent, struct fat16_dir_entry_struct* dir_entry)
{
    if(!fs || !parent || !dir_entry)
        return 0;

    /* we are in a parent directory and want to search within its directory entry table */
    if(!(parent->attributes & FAT16_ATTRIB_DIR))
        return 0;

    /* loop through all clusters of the directory */
    uint8_t buffer[32];
    uint32_t cluster_offset;
    uint16_t cluster_size = fs->header.cluster_size;
    uint16_t cluster_num = parent->cluster;
    struct fat16_read_callback_arg arg;

    while(1)
    {
        /* calculate new cluster offset */
        cluster_offset = fs->header.cluster_zero_offset + (uint32_t) (cluster_num - 2) * cluster_size;

        /* seek to the n-th entry */
        memset(&arg, 0, sizeof(arg));
        arg.entry_num = entry_num;
        if(!fs->partition->device_read_interval(cluster_offset,
            buffer,
           sizeof(buffer),
           cluster_size,
           fat16_dir_entry_seek_callback,
           &arg)
           )
        return 0;

        /* check if we found the entry */
        if(arg.entry_offset)
            break;

        /* get number of next cluster */
        if(!(cluster_num = fat16_get_next_cluster(fs, cluster_num)))
            return 0; /* directory entry not found */
    }

    memset(dir_entry, 0, sizeof(*dir_entry));

    /* read entry */
    if(!fs->partition->device_read_interval(arg.entry_offset,
        buffer,
       sizeof(buffer),
       arg.byte_count,
       fat16_dir_entry_read_callback,
       dir_entry))
    return 0;

    return dir_entry->long_name[0] != '\0' ? 1 : 0;
}

/**
 * \ingroup fat16_fs
 * Callback function for seeking through subdirectory entries.
 */
uint8_t fat16_dir_entry_seek_callback(uint8_t* buffer, uint32_t offset, void* p)
{
    struct fat16_read_callback_arg* arg = p;

    /* skip deleted or empty entries */
    if(buffer[0] == FAT16_DIRENTRY_DELETED || !buffer[0])
        return 1;

    if(arg->entry_cur == arg->entry_num)
    {
        arg->entry_offset = offset;
        arg->byte_count = buffer[11] == 0x0f ?
        ((buffer[0] & FAT16_DIRENTRY_LFNSEQMASK) + 1) * 32 :
        32;
        return 0;
    }

    /* if we read a 8.3 entry, we reached a new directory entry */
    if(buffer[11] != 0x0f)
        ++arg->entry_cur;

    return 1;
}

/**
 * \ingroup fat16_fs
 * Callback function for reading a directory entry.
 */
uint8_t fat16_dir_entry_read_callback(uint8_t* buffer, uint32_t offset, void* p)
{
    struct fat16_dir_entry_struct* dir_entry = p;

    /* there should not be any deleted or empty entries */
    if(buffer[0] == FAT16_DIRENTRY_DELETED || !buffer[0])
        return 0;

    if(!dir_entry->entry_offset)
        dir_entry->entry_offset = offset;

    switch(fat16_interpret_dir_entry(dir_entry, buffer))
    {
        case 0: /* failure */
            return 0;
            case 1: /* buffer successfully parsed, continue */
            return 1;
            case 2: /* directory entry complete, finish */
            return 0;
        }

    return 0;
}

/**
 * \ingroup fat16_fs
 * Interprets a raw directory entry and puts the contained
 * information into the directory entry.
 *
 * For a single file there may exist multiple directory
 * entries. All except the last one are lfn entries, which
 * contain parts of the long filename. The last directory
 * entry is a traditional 8.3 style one. It contains all
 * other information like size, cluster, date and time.
 *
 * \param[in,out] dir_entry The directory entry to fill.
 * \param[in] raw_entry A pointer to 32 bytes of raw data.
 * \returns 0 on failure, 1 on success and 2 if the
 *          directory entry is complete.
 */
uint8_t fat16_interpret_dir_entry(struct fat16_dir_entry_struct* dir_entry, const uint8_t* raw_entry)
{
    if(!dir_entry || !raw_entry || !raw_entry[0])
        return 0;

    char* long_name = dir_entry->long_name;
    if(raw_entry[11] == 0x0f)
    {
        uint16_t char_offset = ((raw_entry[0] & 0x3f) - 1) * 13;

        if(char_offset + 12 < sizeof(dir_entry->long_name))
        {
            /* Lfn supports unicode, but we do not, for now.
                                                 * So we assume pure ascii and read only every
                                                 * second byte.
                                                 */
            long_name[char_offset + 0] = raw_entry[1];
            long_name[char_offset + 1] = raw_entry[3];
            long_name[char_offset + 2] = raw_entry[5];
            long_name[char_offset + 3] = raw_entry[7];
            long_name[char_offset + 4] = raw_entry[9];
            long_name[char_offset + 5] = raw_entry[14];
            long_name[char_offset + 6] = raw_entry[16];
            long_name[char_offset + 7] = raw_entry[18];
            long_name[char_offset + 8] = raw_entry[20];
            long_name[char_offset + 9] = raw_entry[22];
            long_name[char_offset + 10] = raw_entry[24];
            long_name[char_offset + 11] = raw_entry[28];
            long_name[char_offset + 12] = raw_entry[30];
        }

        return 1;
    }
    else
    {
        /* if we do not have a long name, take the short one */
        if(long_name[0] == '\0')
        {
            uint8_t i;
            for(i = 0; i < 8; ++i)
            {
                if(raw_entry[i] == ' ')
                    break;
                long_name[i] = raw_entry[i];
            }
            if(raw_entry[8] != ' ')
            {
                long_name[i++] = '.';

                uint8_t j = 8;
                for(; j < 11; ++j)
                {
                    if(raw_entry[j] != ' ')
                    {
                        long_name[i++] = raw_entry[j];
                    }
                    else
                    {
                        break;
                    }
                }
            }

            long_name[i] = '\0';
        }

        /* extract properties of file and store them within the structure */
        dir_entry->attributes = raw_entry[11];
        dir_entry->cluster = ((uint16_t) raw_entry[26]) |
        ((uint16_t) raw_entry[27] << 8);
        dir_entry->file_size = ((uint32_t) raw_entry[28]) |
        ((uint32_t) raw_entry[29] << 8) |
        ((uint32_t) raw_entry[30] << 16) |
        ((uint32_t) raw_entry[31] << 24);

        return 2;
    }
}

/**
 * \ingroup fat16_file
 * Retrieves the directory entry of a path.
 *
 * The given path may both describe a file or a directory.
 *
 * \param[in] fs The FAT16 filesystem on which to search.
 * \param[in] path The path of which to read the directory entry.
 * \param[out] dir_entry The directory entry to fill.
 * \returns 0 on failure, 1 on success.
 * \see fat16_read_dir
 */
uint8_t fat16_get_dir_entry_of_path(struct fat16_fs_struct* fs, const char* path, struct fat16_dir_entry_struct* dir_entry)
{
    if(!fs || !path || path[0] == '\0' || !dir_entry)
        return 0;

    if(path[0] == '/')
        ++path;

    /* begin with the root directory */
    memset(dir_entry, 0, sizeof(*dir_entry));
    dir_entry->attributes = FAT16_ATTRIB_DIR;

    if(path[0] == '\0')
        return 1;

    while(1)
    {
        struct fat16_dir_struct* dd = fat16_open_dir(fs, dir_entry);
        if(!dd)
            break;

        /* extract the next hierarchy we will search for */
        const char* sep_pos = strchr(path, '/');
        if(!sep_pos)
            sep_pos = path + strlen(path);
        uint8_t length_to_sep = sep_pos - path;

        /* read directory entries */
        while(fat16_read_dir(dd, dir_entry))
        {
            /* check if we have found the next hierarchy */
            if((strlen(dir_entry->long_name) != length_to_sep ||
                strncmp(path, dir_entry->long_name, length_to_sep) != 0))
            continue;

            fat16_close_dir(dd);
            dd = 0;

            if(path[length_to_sep] == '\0')
    /* we iterated through the whole path and have found the file */
                return 1;

            if(dir_entry->attributes & FAT16_ATTRIB_DIR)
            {
                /* we found a parent directory of the file we are searching for */
                path = sep_pos + 1;
                break;
            }

            /* a parent of the file exists, but not the file itself */
            return 0;
        }

        fat16_close_dir(dd);
    }

    return 0;
}

/**
 * \ingroup fat16_fs
 * Retrieves the next following cluster of a given cluster.
 *
 * Using the filesystem file allocation table, this function returns
 * the number of the cluster containing the data directly following
 * the data within the cluster with the given number.
 *
 * \param[in] fs The filesystem for which to determine the next cluster.
 * \param[in] cluster_num The number of the cluster for which to determine its successor.
 * \returns The wanted cluster number, or 0 on error.
 */
uint16_t fat16_get_next_cluster(const struct fat16_fs_struct* fs, uint16_t cluster_num)
{
    if(!fs || cluster_num < 2)
        return 0;

    /* read appropriate fat entry */
    uint8_t fat_entry[2];
    if(!fs->partition->device_read(fs->header.fat_offset + 2 * cluster_num, fat_entry, 2))
        return 0;

    /* determine next cluster from fat */
    cluster_num = ((uint16_t) fat_entry[0]) |
    ((uint16_t) fat_entry[1] << 8);
	///*
    if(cluster_num == FAT16_CLUSTER_FREE ||
        cluster_num == FAT16_CLUSTER_BAD ||
       (cluster_num >= FAT16_CLUSTER_RESERVED_MIN && cluster_num <= FAT16_CLUSTER_RESERVED_MAX) ||
       (cluster_num >= FAT16_CLUSTER_LAST_MIN && cluster_num <= FAT16_CLUSTER_LAST_MAX))
    //*/
	return 0;

    return cluster_num;
}

/**
 * \ingroup fat16_fs
 * Appends a new cluster chain to an existing one.
 *
 * Set cluster_num to zero to create a completely new one.
 *
 * \param[in] fs The file system on which to operate.
 * \param[in] cluster_num The cluster to which to append the new chain.
 * \param[in] count The number of clusters to allocate.
 * \returns 0 on failure, the number of the first new cluster on success.
 */
uint16_t fat16_append_clusters(const struct fat16_fs_struct* fs, uint16_t cluster_num, uint16_t count)
{
    #if FAT16_WRITE_SUPPORT
        if(!fs)
            return 0;
    
        device_read_t device_read = fs->partition->device_read;
        device_write_t device_write = fs->partition->device_write;
        uint32_t fat_offset = fs->header.fat_offset;
        uint16_t cluster_max = fs->header.fat_size / 2;
        uint16_t cluster_next = 0;
        uint16_t count_left = count;
        uint8_t buffer[2];
        uint16_t cluster_new;
        for(cluster_new = 0; cluster_new < cluster_max; ++cluster_new)
        {
            if(!device_read(fat_offset + 2 * cluster_new, buffer, sizeof(buffer)))
                return 0;
    
            /* check if this is a free cluster */
            if(buffer[0] == (FAT16_CLUSTER_FREE & 0xff) &&
                buffer[1] == ((FAT16_CLUSTER_FREE >> 8) & 0xff))
            {
                /* allocate cluster */
                if(count_left == count)
                {
                    buffer[0] = FAT16_CLUSTER_LAST_MAX & 0xff;
                    buffer[1] = (FAT16_CLUSTER_LAST_MAX >> 8) & 0xff;
                }
                else
                {
                    buffer[0] = cluster_next & 0xff;
                    buffer[1] = (cluster_next >> 8) & 0xff;
                }
    
                if(!device_write(fat_offset + 2 * cluster_new, buffer, sizeof(buffer)))
                    break;
    
                cluster_next = cluster_new;
                if(--count_left == 0)
                    break;
            }
        }
    
        do
        {
            if(count_left > 0)
                break;
    
            /* We allocated a new cluster chain. Now join
                                             * it with the existing one.
                                             */
            if(cluster_num >= 2)
            {
                buffer[0] = cluster_next & 0xff;
                buffer[1] = (cluster_next >> 8) & 0xff;
                if(!device_write(fat_offset + 2 * cluster_num, buffer, sizeof(buffer)))
                    break;
            }
    
            return cluster_next;
    
        }
        while(0);
    
        /* No space left on device or writing error.
                             * Free up all clusters already allocated.
                             */
        fat16_free_clusters(fs, cluster_next);
    
        return 0;
    #else
        return 0;
    #endif
}

/**
 * \ingroup fat16_fs
 * Frees a cluster chain, or a part thereof.
 *
 * Marks the specified cluster and all clusters which are sequentially
 * referenced by it as free. They may then be used again for future
 * file allocations.
 *
 * \note If this function is used for freeing just a part of a cluster
 *       chain, the new end of the chain is not correctly terminated
 *       within the FAT. Use fat16_terminate_clusters() instead.
 *
 * \param[in] fs The filesystem on which to operate.
 * \param[in] cluster_num The starting cluster of the chain which to free.
 * \returns 0 on failure, 1 on success.
 * \see fat16_terminate_clusters
 */
uint8_t fat16_free_clusters(const struct fat16_fs_struct* fs, uint16_t cluster_num)
{
    #if FAT16_WRITE_SUPPORT
        if(!fs || cluster_num < 2)
            return 0;
    
        uint32_t fat_offset = fs->header.fat_offset;
        uint8_t buffer[2];
        while(cluster_num)
        {
            if(!fs->partition->device_read(fat_offset + 2 * cluster_num, buffer, 2))
                return 0;
    
            /* get next cluster of current cluster before freeing current cluster */
            uint16_t cluster_num_next = ((uint16_t) buffer[0]) |
            ((uint16_t) buffer[1] << 8);
    
            if(cluster_num_next == FAT16_CLUSTER_FREE)
                return 1;
            if(cluster_num_next == FAT16_CLUSTER_BAD ||
                (cluster_num_next >= FAT16_CLUSTER_RESERVED_MIN &&
               cluster_num_next <= FAT16_CLUSTER_RESERVED_MAX
               )
               )
            return 0;
			///*
            if((cluster_num_next >= FAT16_CLUSTER_LAST_MIN) &&
                (cluster_num_next <= FAT16_CLUSTER_LAST_MAX)
               )
			//*/
            cluster_num_next = 0;
    
            /* free cluster */
            buffer[0] = FAT16_CLUSTER_FREE & 0xff;
            buffer[1] = (FAT16_CLUSTER_FREE >> 8) & 0xff;
            fs->partition->device_write(fat_offset + 2 * cluster_num, buffer, 2);
    
            /* We continue in any case here, even if freeing the cluster failed.
                                             * The cluster is lost, but maybe we can still free up some later ones.
                                             */
    
            cluster_num = cluster_num_next;
        }
    
        return 1;
    #else
        return 0;
    #endif
}

/**
 * \ingroup fat16_fs
 * Frees a part of a cluster chain and correctly terminates the rest.
 *
 * Marks the specified cluster as the new end of a cluster chain and
 * frees all following clusters.
 *
 * \param[in] fs The filesystem on which to operate.
 * \param[in] cluster_num The new end of the cluster chain.
 * \returns 0 on failure, 1 on success.
 * \see fat16_free_clusters
 */
uint8_t fat16_terminate_clusters(const struct fat16_fs_struct* fs, uint16_t cluster_num)
{
    #if FAT16_WRITE_SUPPORT
        if(!fs || cluster_num < 2)
            return 0;
    
        /* fetch next cluster before overwriting the cluster entry */
        uint16_t cluster_num_next = fat16_get_next_cluster(fs, cluster_num);
    
        /* mark cluster as the last one */
        uint8_t buffer[2];
        buffer[0] = FAT16_CLUSTER_LAST_MAX & 0xff;
        buffer[1] = (FAT16_CLUSTER_LAST_MAX >> 8) & 0xff;
        if(!fs->partition->device_write(fs->header.fat_offset + 2 * cluster_num, buffer, 2))
            return 0;
    
        /* free remaining clusters */
        if(cluster_num_next)
            return fat16_free_clusters(fs, cluster_num_next);
        else
            return 1;
    #else
        return 0;
    #endif
}

/**
 * \ingroup fat16_file
 * Opens a file on a FAT16 filesystem.
 *
 * \param[in] fs The filesystem on which the file to open lies.
 * \param[in] dir_entry The directory entry of the file to open.
 * \returns The file handle, or 0 on failure.
 * \see fat16_close_file
 */
struct fat16_file_struct* fat16_open_file(struct fat16_fs_struct* fs, const struct fat16_dir_entry_struct* dir_entry)
{
    if(!fs || !dir_entry || (dir_entry->attributes & FAT16_ATTRIB_DIR))
        return 0;

    struct fat16_file_struct* fd = malloc(sizeof(*fd));
    if(!fd)
        return 0;

    memcpy(&fd->dir_entry, dir_entry, sizeof(*dir_entry));
    fd->fs = fs;
    fd->pos = 0;
    fd->pos_cluster = dir_entry->cluster;

    return fd;
}

/**
 * \ingroup fat16_file
 * Closes a file.
 *
 * \param[in] fd The file handle of the file to close.
 * \see fat16_open_file
 */
void fat16_close_file(struct fat16_file_struct* fd)
{
    if(fd)
        free(fd);
}

/**
 * \ingroup fat16_file
 * Reads data from a file.
 *
 * The data requested is read from the current file location.
 *
 * \param[in] fd The file handle of the file from which to read.
 * \param[out] buffer The buffer into which to write.
 * \param[in] buffer_len The amount of data to read.
 * \returns The number of bytes read, 0 on end of file, or -1 on failure.
 * \see fat16_write_file
 */
int16_t fat16_read_file(struct fat16_file_struct* fd, uint8_t* buffer, uint16_t buffer_len)
{
    /* check arguments */
    if(!fd || !buffer || buffer_len < 1)
        return -1;

    /* determine number of bytes to read */
    if(fd->pos + buffer_len > fd->dir_entry.file_size)
        buffer_len = fd->dir_entry.file_size - fd->pos;
    if(buffer_len == 0)
        return 0;

    uint16_t cluster_size = fd->fs->header.cluster_size;
    uint16_t cluster_num = fd->pos_cluster;
    uint16_t buffer_left = buffer_len;
    uint16_t first_cluster_offset = fd->pos % cluster_size;

    /* find cluster in which to start reading */
    if(!cluster_num)
    {
        cluster_num = fd->dir_entry.cluster;

        if(!cluster_num)
        {
            if(!fd->pos)
                return 0;
            else
                return -1;
        }

        if(fd->pos)
        {
            uint32_t pos = fd->pos;
            while(pos >= cluster_size)
            {
                pos -= cluster_size;
                cluster_num = fat16_get_next_cluster(fd->fs, cluster_num);
                if(!cluster_num)
                    return -1;
            }
        }
    }

    /* read data */
    do
    {
        /* calculate data size to copy from cluster */
        uint32_t cluster_offset = fd->fs->header.cluster_zero_offset +
        (uint32_t) (cluster_num - 2) * cluster_size + first_cluster_offset;
        uint16_t copy_length = cluster_size - first_cluster_offset;
        if(copy_length > buffer_left)
            copy_length = buffer_left;

        /* read data */
        if(!fd->fs->partition->device_read(cluster_offset, buffer, copy_length))
            return buffer_len - buffer_left;

        /* calculate new file position */
        buffer += copy_length;
        buffer_left -= copy_length;
        fd->pos += copy_length;

        if(first_cluster_offset + copy_length >= cluster_size)
        {
            /* we are on a cluster boundary, so get the next cluster */
            if((cluster_num = fat16_get_next_cluster(fd->fs, cluster_num)))
            {
                first_cluster_offset = 0;
            }
            else
            {
                fd->pos_cluster = 0;
                return buffer_len - buffer_left;
            }
        }

        fd->pos_cluster = cluster_num;

    }
    while(buffer_left > 0); /* check if we are done */

    return buffer_len;
}

/**
 * \ingroup fat16_file
 * Writes data to a file.
 *
 * The data is written to the current file location.
 *
 * \param[in] fd The file handle of the file to which to write.
 * \param[in] buffer The buffer from which to read the data to be written.
 * \param[in] buffer_len The amount of data to write.
 * \returns The number of bytes written, 0 on disk full, or -1 on failure.
 * \see fat16_read_file
 */
int16_t fat16_write_file(struct fat16_file_struct* fd, const uint8_t* buffer, uint16_t buffer_len)
{
    #if FAT16_WRITE_SUPPORT
        /* check arguments */
        if(!fd || !buffer || buffer_len < 1)
            return -1;
        if(fd->pos > fd->dir_entry.file_size)
            return -1;
    
        uint16_t cluster_size = fd->fs->header.cluster_size;
        uint16_t cluster_num = fd->pos_cluster;
        uint16_t buffer_left = buffer_len;
        uint16_t first_cluster_offset = fd->pos % cluster_size;
    
        /* find cluster in which to start writing */
        if(!cluster_num)
        {
            cluster_num = fd->dir_entry.cluster;
    
            if(!cluster_num)
            {
                if(!fd->pos)
                {
                    /* empty file */
                    fd->dir_entry.cluster = cluster_num = fat16_append_clusters(fd->fs, 0, 1);
                    if(!cluster_num)
                        return -1;
                }
                else
                {
                    return -1;
                }
            }
    
            if(fd->pos)
            {
                uint32_t pos = fd->pos;
                uint16_t cluster_num_next;
                while(pos >= cluster_size)
                {
                    pos -= cluster_size;
                    cluster_num_next = fat16_get_next_cluster(fd->fs, cluster_num);
                    if(!cluster_num_next && pos == 0)
        /* the file exactly ends on a cluster boundary, and we append to it */
                        cluster_num_next = fat16_append_clusters(fd->fs, cluster_num, 1);
                    if(!cluster_num_next)
                        return -1;
    
                    cluster_num = cluster_num_next;
                }
            }
        }
    
        /* write data */
        do
        {
            /* calculate data size to write to cluster */
            uint32_t cluster_offset = fd->fs->header.cluster_zero_offset +
            (uint32_t) (cluster_num - 2) * cluster_size + first_cluster_offset;
            uint16_t write_length = cluster_size - first_cluster_offset;
            if(write_length > buffer_left)
                write_length = buffer_left;
    
            /* write data which fits into the current cluster */
            if(!fd->fs->partition->device_write(cluster_offset, buffer, write_length))
                break;
    
            /* calculate new file position */
            buffer += write_length;
            buffer_left -= write_length;
            fd->pos += write_length;
    
            if(first_cluster_offset + write_length >= cluster_size)
            {
                /* we are on a cluster boundary, so get the next cluster */
                uint16_t cluster_num_next = fat16_get_next_cluster(fd->fs, cluster_num);
                if(!cluster_num_next && buffer_left > 0)
        /* we reached the last cluster, append a new one */
                    cluster_num_next = fat16_append_clusters(fd->fs, cluster_num, 1);
                if(!cluster_num_next)
                {
                    fd->pos_cluster = 0;
                    break;
                }
    
                cluster_num = cluster_num_next;
                first_cluster_offset = 0;
            }
    
            fd->pos_cluster = cluster_num;
    
        }
        while(buffer_left > 0); /* check if we are done */
    
        /* update directory entry */
        if(fd->pos > fd->dir_entry.file_size)
        {
            uint32_t size_old = fd->dir_entry.file_size;
    
            /* update file size */
            fd->dir_entry.file_size = fd->pos;
            /* write directory entry */
            if(!fat16_write_dir_entry(fd->fs, &fd->dir_entry))
            {
                /* We do not return an error here since we actually wrote
                                                             * some data to disk. So we calculate the amount of data
                                                             * we wrote to disk and which lies within the old file size.
                                                             */
                buffer_left = fd->pos - size_old;
                fd->pos = size_old;
            }
        }
    
        return buffer_len - buffer_left;
    
    #else
        return -1;
    #endif
}

/**
 * \ingroup fat16_file
 * Repositions the read/write file offset.
 *
 * Changes the file offset where the next call to fat16_read_file()
 * or fat16_write_file() starts reading/writing.
 *
 * If the new offset is beyond the end of the file, fat16_resize_file()
 * is implicitly called, i.e. the file is expanded.
 *
 * The new offset can be given in different ways determined by
 * the \c whence parameter:
 * - \b FAT16_SEEK_SET: \c *offset is relative to the beginning of the file.
 * - \b FAT16_SEEK_CUR: \c *offset is relative to the current file position.
 * - \b FAT16_SEEK_END: \c *offset is relative to the end of the file.
 *
 * The resulting absolute offset is written to the location the \c offset
 * parameter points to.
 *
 * \param[in] fd The file decriptor of the file on which to seek.
 * \param[in,out] offset A pointer to the new offset, as affected by the \c whence
 *                   parameter. The function writes the new absolute offset
 *                   to this location before it returns.
 * \param[in] whence Affects the way \c offset is interpreted, see above.
 * \returns 0 on failure, 1 on success.
 */
uint8_t fat16_seek_file(struct fat16_file_struct* fd, int32_t* offset, uint8_t whence)
{
    if(!fd || !offset)
        return 0;

    uint32_t new_pos = fd->pos;
    switch(whence)
    {
        case FAT16_SEEK_SET:
            new_pos = *offset;
            break;
        case FAT16_SEEK_CUR:
            new_pos += *offset;
            break;
        case FAT16_SEEK_END:
            new_pos = fd->dir_entry.file_size + *offset;
            break;
        default:
            return 0;
    }

    if(new_pos > fd->dir_entry.file_size && !fat16_resize_file(fd, new_pos))
        return 0;

    fd->pos = new_pos;
    fd->pos_cluster = 0;

    *offset = new_pos;
    return 1;
}

/**
 * \ingroup fat16_file
 * Resizes a file to have a specific size.
 *
 * Enlarges or shrinks the file pointed to by the file descriptor to have
 * exactly the specified size.
 *
 * If the file is truncated, all bytes having an equal or larger offset
 * than the given size are lost. If the file is expanded, the additional
 * bytes are allocated.
 *
 * \note Please be aware that this function just allocates or deallocates disk
 * space, it does not explicitely clear it. To avoid data leakage, this
 * must be done manually.
 *
 * \param[in] fd The file decriptor of the file which to resize.
 * \param[in] size The new size of the file.
 * \returns 0 on failure, 1 on success.
 */
uint8_t fat16_resize_file(struct fat16_file_struct* fd, uint32_t size)
{
    #if FAT16_WRITE_SUPPORT
        if(!fd)
            return 0;
    
        uint16_t cluster_num = fd->dir_entry.cluster;
        uint16_t cluster_size = fd->fs->header.cluster_size;
        uint32_t size_new = size;
    
        do
        {
            if(cluster_num == 0 && size_new == 0)
        /* the file stays empty */
                break;
    
            /* seek to the next cluster as long as we need the space */
            while(size_new > cluster_size)
            {
                /* get next cluster of file */
                uint16_t cluster_num_next = fat16_get_next_cluster(fd->fs, cluster_num);
                if(cluster_num_next)
                {
                    cluster_num = cluster_num_next;
                    size_new -= cluster_size;
                }
                else
                {
                    break;
                }
            }
    
            if(size_new > cluster_size || cluster_num == 0)
            {
                /* Allocate new cluster chain and append
                                                             * it to the existing one, if available.
                                                             */
                uint16_t cluster_count = size_new / cluster_size;
                if((uint32_t) cluster_count * cluster_size < size_new)
                    ++cluster_count;
                uint16_t cluster_new_chain = fat16_append_clusters(fd->fs, cluster_num, cluster_count);
                if(!cluster_new_chain)
                    return 0;
    
                if(!cluster_num)
                {
                    cluster_num = cluster_new_chain;
                    fd->dir_entry.cluster = cluster_num;
                }
            }
    
            /* write new directory entry */
            fd->dir_entry.file_size = size;
            if(size == 0)
                fd->dir_entry.cluster = 0;
            if(!fat16_write_dir_entry(fd->fs, &fd->dir_entry))
                return 0;
    
            if(size == 0)
            {
                /* free all clusters of file */
                fat16_free_clusters(fd->fs, cluster_num);
            }
            else if(size_new <= cluster_size)
            {
                /* free all clusters no longer needed */
                fat16_terminate_clusters(fd->fs, cluster_num);
            }
    
        }
        while(0);
    
        /* correct file position */
        if(size < fd->pos)
        {
            fd->pos = size;
            fd->pos_cluster = 0;
        }
    
        return 1;
    #else
        return 0;
    #endif
}

/**
 * \ingroup fat16_dir
 * Opens a directory.
 *
 * \param[in] fs The filesystem on which the directory to open resides.
 * \param[in] dir_entry The directory entry which stands for the directory to open.
 * \returns An opaque directory descriptor on success, 0 on failure.
 * \see fat16_close_dir
 */
struct fat16_dir_struct* fat16_open_dir(struct fat16_fs_struct* fs, const struct fat16_dir_entry_struct* dir_entry)
{
    if(!fs || !dir_entry || !(dir_entry->attributes & FAT16_ATTRIB_DIR))
        return 0;

    struct fat16_dir_struct* dd = malloc(sizeof(*dd));
    if(!dd)
        return 0;

    memcpy(&dd->dir_entry, dir_entry, sizeof(*dir_entry));
    dd->fs = fs;
    dd->entry_next = 0;

    return dd;
}

/**
 * \ingroup fat16_dir
 * Closes a directory descriptor.
 *
 * This function destroys a directory descriptor which was
 * previously obtained by calling fat16_open_dir(). When this
 * function returns, the given descriptor will be invalid.
 *
 * \param[in] dd The directory descriptor to close.
 * \see fat16_open_dir
 */
void fat16_close_dir(struct fat16_dir_struct* dd)
{
    if(dd)
        free(dd);
}

/**
 * \ingroup fat16_dir
 * Reads the next directory entry contained within a parent directory.
 *
 * \param[in] dd The descriptor of the parent directory from which to read the entry.
 * \param[out] dir_entry Pointer to a buffer into which to write the directory entry information.
 * \returns 0 on failure, 1 on success.
 * \see fat16_reset_dir
 */
uint8_t fat16_read_dir(struct fat16_dir_struct* dd, struct fat16_dir_entry_struct* dir_entry)
{
    if(!dd || !dir_entry)
        return 0;

    if(dd->dir_entry.cluster == 0)
    {
        /* read entry from root directory */
        if(fat16_read_root_dir_entry(dd->fs, dd->entry_next, dir_entry))
        {
            ++dd->entry_next;
            return 1;
        }
    }
    else
    {
        /* read entry from a subdirectory */
        if(fat16_read_sub_dir_entry(dd->fs, dd->entry_next, &dd->dir_entry, dir_entry))
        {
            ++dd->entry_next;
            return 1;
        }
    }

    /* restart reading */
    dd->entry_next = 0;

    return 0;
}

/**
 * \ingroup fat16_dir
 * Resets a directory handle.
 *
 * Resets the directory handle such that reading restarts
 * with the first directory entry.
 *
 * \param[in] dd The directory handle to reset.
 * \returns 0 on failure, 1 on success.
 * \see fat16_read_dir
 */
uint8_t fat16_reset_dir(struct fat16_dir_struct* dd)
{
    if(!dd)
        return 0;

    dd->entry_next = 0;
    return 1;
}

uint8_t fat16_set_dir(struct fat16_dir_struct* dd, uint16_t offset)
{
    if(!dd)
        return 0;

    dd->entry_next = offset;
    return 1;
}

/**
 * \ingroup fat16_fs
 * Writes a directory entry to disk.
 *
 * \note The file name is not checked for invalid characters.
 *
 * \note The generation of the short 8.3 file name is quite
 * simple. The first eight characters are used for the filename.
 * The extension, if any, is made up of the first three characters
 * following the last dot within the long filename. If the
 * filename (without the extension) is longer than eight characters,
 * the lower byte of the cluster number replaces the last two
 * characters to avoid name clashes. In any other case, it is your
 * responsibility to avoid name clashes.
 *
 * \param[in] fs The filesystem on which to operate.
 * \param[in] dir_entry The directory entry to write.
 * \returns 0 on failure, 1 on success.
 */
uint8_t fat16_write_dir_entry(const struct fat16_fs_struct* fs, const struct fat16_dir_entry_struct* dir_entry)
{
    #if FAT16_WRITE_SUPPORT
        if(!fs || !dir_entry)
            return 0;
    
        device_write_t device_write = fs->partition->device_write;
        uint32_t offset = dir_entry->entry_offset;
        uint8_t name_len = strlen(dir_entry->long_name);
        uint8_t lfn_entry_count = (name_len + 12) / 13;
        uint8_t buffer[32];
    
        /* write 8.3 entry */
    
        /* generate 8.3 file name */
        memset(&buffer[0], ' ', 11);
        char* name_ext = strrchr(dir_entry->long_name, '.');
        if(name_ext)
        {
            ++name_ext;
    
            uint8_t name_ext_len = strlen(name_ext);
            name_len -= name_ext_len + 1;
    
            if(name_ext_len > 3)
                name_ext_len = 3;
    
            memcpy(&buffer[8], name_ext, name_ext_len);
        }
    
        if(name_len <= 8)
        {
            memcpy(buffer, dir_entry->long_name, name_len);
        }
        else
        {
            memcpy(buffer, dir_entry->long_name, 8);
    
            /* Minimize 8.3 name clashes by appending
                                             * the lower byte of the cluster number.
                                             */
            uint8_t num = dir_entry->cluster & 0xff;
    
            buffer[6] = (num < 0xa0) ? ('0' + (num >> 4)) : ('a' + (num >> 4));
            num &= 0x0f;
            buffer[7] = (num < 0x0a) ? ('0' + num) : ('a' + num);
        }
    
        /* fill directory entry buffer */
        memset(&buffer[11], 0, sizeof(buffer) - 11);
        buffer[0x0b] = dir_entry->attributes;

        //Not used in bootloader
		#ifdef MAIN_FIRMWARE
			//Added modified date/time 12-26-07
			//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
			char * current_time;
/*			current_time = GPS_get_time();
			char * current_date;
			current_date = GPS_get_date();
			
			uint16_t modified_time, modified_date;
			uint8_t c_hour, c_minute, c_second;
			uint8_t c_year, c_month, c_day;
			
			c_hour = (current_time[0] - '0') * 10;
			c_hour += (current_time[1] - '0');
			c_minute = (current_time[3] - '0') * 10;
			c_minute += (current_time[4] - '0');
			c_second = (current_time[6] - '0') * 10;
			c_second += (current_time[7] - '0');
			
			c_day = (current_date[0] - '0') * 10;
			c_day += (current_date[1] - '0');
			c_month = (current_date[3] - '0') * 10;
			c_month += (current_date[4] - '0');
			c_year = (current_date[6] - '0') * 10;
			c_year += (current_date[7] - '0');
			
			//Correct for 1980 = 0, 2000 = 20, year code
			c_year += 20;
			
			modified_time = (c_hour << (16-5)) | (c_minute << (16-11)) | (c_second);
			modified_date = (c_year << (16-7)) | (c_month << (16-11)) | (c_day);

			//Do a little error correction
			if (c_year < 26) //26 = 2006
			{
				modified_time = 0;
				modified_date = 0;
			}

			//rprintf("c_time=%d, %d, %d\n", c_hour, c_minute, c_second);
			//rprintf("c_date=%d, %d, %d\n", c_month, c_day, c_year);
			//rprintf("mod_time=%x\n", modified_time);
			//rprintf("mod_date=%x\n", modified_date);
			
			buffer[0x16] = (modified_time >> 0) & 0xFF;
			buffer[0x17] = (modified_time >> 8) & 0xFF;
			buffer[0x18] = (modified_date >> 0) & 0xFF;
			buffer[0x19] = (modified_date >> 8) & 0xFF;
			*/
			//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
		#endif

        buffer[0x1a] = (dir_entry->cluster >> 0) & 0xff;
        buffer[0x1b] = (dir_entry->cluster >> 8) & 0xff;
        buffer[0x1c] = (dir_entry->file_size >> 0) & 0xff;
        buffer[0x1d] = (dir_entry->file_size >> 8) & 0xff;
        buffer[0x1e] = (dir_entry->file_size >> 16) & 0xff;
        buffer[0x1f] = (dir_entry->file_size >> 24) & 0xff;
    
        /* write to disk */
        if(!device_write(offset + (uint32_t) lfn_entry_count * 32, buffer, sizeof(buffer)))
            return 0;
    
        /* calculate checksum of 8.3 name */
        uint8_t checksum = buffer[0];
        uint8_t i;
        for(i = 1; i < 11; ++i)
            checksum = ((checksum >> 1) | (checksum << 7)) + buffer[i];
    
        /* write lfn entries */
        uint8_t lfn_entry;
        for(lfn_entry = lfn_entry_count; lfn_entry > 0; --lfn_entry)
        {
            memset(buffer, 0, sizeof(buffer));
            memset(&buffer[0x01], 0xff, 10);
            memset(&buffer[0x0e], 0xff, 12);
            memset(&buffer[0x1c], 0xff, 4);
    
            buffer[0x00] = lfn_entry;
            if(lfn_entry == lfn_entry_count)
                buffer[0x00] |= FAT16_DIRENTRY_LFNLAST;
    
            /* set file name */
            const char* long_name_curr = dir_entry->long_name + (lfn_entry - 1) * 13;
            i = 1;
            while(i < 0x1f)
            {
                buffer[i++] = *long_name_curr;
                buffer[i++] = 0;
    
                switch(i)
                {
                    case 0x0b:
                        i = 0x0e;
                        break;
                    case 0x1a:
                        i = 0x1c;
                        break;
                }
    
                if(!*long_name_curr++)
                    break;
            }
    
            /* mark as lfn entry */
            buffer[0x0b] = 0x0f;
    
            /* set checksum */
            buffer[0x0d] = checksum;
    
            /* write entry */
            device_write(offset, buffer, sizeof(buffer));
    
            offset += sizeof(buffer);
        }
    
        return 1;
    
    #else
        return 0;
    #endif
}

/**
 * \ingroup fat16_file
 * Creates a file.
 *
 * Creates a file and obtains the directory entry of the
 * new file. If the file to create already exists, the
 * directory entry of the existing file will be returned
 * within the dir_entry parameter.
 *
 * \note The file name is not checked for invalid characters.
 *
 * \note The generation of the short 8.3 file name is quite
 * simple. The first eight characters are used for the filename.
 * The extension, if any, is made up of the first three characters
 * following the last dot within the long filename. If the
 * filename (without the extension) is longer than eight characters,
 * the lower byte of the cluster number replaces the last two
 * characters to avoid name clashes. In any other case, it is your
 * responsibility to avoid name clashes.
 *
 * \param[in] parent The handle of the directory in which to create the file.
 * \param[in] file The name of the file to create.
 * \param[out] dir_entry The directory entry to fill for the new file.
 * \returns 0 on failure, 1 on success.
 * \see fat16_delete_file
 */
uint8_t fat16_create_file(struct fat16_dir_struct* parent, const char* file, struct fat16_dir_entry_struct* dir_entry)
{
    #if FAT16_WRITE_SUPPORT
        if(!parent || !file || !file[0])
            return 0;
    
        /* check if the file already exists */
        while(1)
        {
            if(!fat16_read_dir(parent, dir_entry))
                break;
    
            if(strcmp(file, dir_entry->long_name) == 0)
            {
                fat16_reset_dir(parent);
                return 1;
            }
        }
    
        memset(dir_entry, 0, sizeof(*dir_entry));
        strncpy(dir_entry->long_name, file, sizeof(dir_entry->long_name) - 1);
    
        /* search for a place where to write the directory entry to disk */
        uint8_t free_dir_entries_needed = strlen(file) / 13 + 1 + 1;
        uint8_t free_dir_entries_found = 0;
        struct fat16_fs_struct* fs = parent->fs;
        uint16_t cluster_num = parent->dir_entry.cluster;
        uint32_t dir_entry_offset = 0;
        uint32_t offset = 0;
        uint32_t offset_to = 0;
    
        if(cluster_num == 0)
        {
            /* we read/write from the root directory entry */
            offset = fs->header.root_dir_offset;
            offset_to = fs->header.cluster_zero_offset;
            dir_entry_offset = offset;
        }
    
        while(1)
        {
            if(offset == offset_to)
            {
                if(cluster_num == 0)
				/* We iterated through the whole root directory entry
                * and could not find enough space for the directory entry.
                */
                return 0;
    
                if(offset)
                {
                    /* We reached a cluster boundary and have to
                    * switch to the next cluster.
                    */
    
                    uint16_t cluster_next = fat16_get_next_cluster(fs, cluster_num);
                    if(!cluster_next)
                    {
                        cluster_next = fat16_append_clusters(fs, cluster_num, 1);
                        if(!cluster_next)
                            return 0;
    
                        /* we appended a new cluster and know it is free */
                        dir_entry_offset = fs->header.cluster_zero_offset +
                        (uint32_t) (cluster_next - 2) * fs->header.cluster_size;
    
                        /* TODO: This cluster has to be zeroed in an efficient way, or at least
                        *       every 32th byte should be set to FAT16_DIRENTRY_DELETED.
                        */
                        break;
                    }
                    cluster_num = cluster_next;
                }
    
                offset = fs->header.cluster_zero_offset +
                (uint32_t) (cluster_num - 2) * fs->header.cluster_size;
                offset_to = offset + fs->header.cluster_size;
                dir_entry_offset = offset;
                free_dir_entries_found = 0;
            }
    
            /* read next lfn or 8.3 entry */
            uint8_t first_char;
            if(!fs->partition->device_read(offset, &first_char, sizeof(first_char)))
                return 0;
    
            /* check if we found a free directory entry */
            if(first_char == FAT16_DIRENTRY_DELETED || !first_char)
            {
                /* check if we have the needed number of available entries */
                ++free_dir_entries_found;
                if(free_dir_entries_found >= free_dir_entries_needed)
                    break;
    
                offset += 32;
            }
            else
            {
                offset += 32;
                dir_entry_offset = offset;
                free_dir_entries_found = 0;
            }
        }

        /* write directory entry to disk */
        dir_entry->entry_offset = dir_entry_offset;
        if(!fat16_write_dir_entry(fs, dir_entry))
            return 0;
    
        return 1;
    
    #else
        return 0;
    #endif
}

/**
 * \ingroup fat16_file
 * Deletes a file or directory.
 *
 * It is not checked if the file to delete is a directory.
 * If a directory is deleted without first deleting its
 * subdirectories and files, disk space occupied by these
 * files will get wasted as there is no chance to release
 * it and mark it as free.
 *
 * \param[in] fs The filesystem on which to operate.
 * \param[in] dir_entry The directory entry of the file to delete.
 * \returns 0 on failure, 1 on success.
 * \see fat16_create_file
 */
uint8_t fat16_delete_file(struct fat16_fs_struct* fs, struct fat16_dir_entry_struct* dir_entry)
{
    #if FAT16_WRITE_SUPPORT
        if(!fs || !dir_entry)
            return 0;
    
        /* get offset of the file's directory entry */
        uint32_t dir_entry_offset = dir_entry->entry_offset;
        if(!dir_entry_offset)
            return 0;
    
        uint8_t buffer[12];
        while(1)
        {
            /* read directory entry */
            if(!fs->partition->device_read(dir_entry_offset, buffer, sizeof(buffer)))
                return 0;
    
            /* mark the directory entry as deleted */
            buffer[0] = FAT16_DIRENTRY_DELETED;
    
            /* write back entry */
            if(!fs->partition->device_write(dir_entry_offset, buffer, sizeof(buffer)))
                return 0;
    
            /* check if we deleted the whole entry */
            if(buffer[11] != 0x0f)
                break;
    
            dir_entry_offset += 32;
        }
    
        /* We deleted the directory entry. The next thing to do is
                             * marking all occupied clusters as free.
                             */
        return fat16_free_clusters(fs, dir_entry->cluster);
    #else
        return 0;
    #endif
}

/**
 * \ingroup fat16_fs
 * Returns the amount of total storage capacity of the filesystem in bytes.
 *
 * \param[in] fs The filesystem on which to operate.
 * \returns 0 on failure, the filesystem size in bytes otherwise.
 */
uint32_t fat16_get_fs_size(const struct fat16_fs_struct* fs)
{
    if(!fs)
        return 0;

    return (fs->header.fat_size / 2 - 2) * fs->header.cluster_size;
}

/**
 * \ingroup fat16_fs
 * Returns the amount of free storage capacity on the filesystem in bytes.
 *
 * \note As the FAT16 filesystem is cluster based, this function does not
 *       return continuous values but multiples of the cluster size.
 *
 * \param[in] fs The filesystem on which to operate.
 * \returns 0 on failure, the free filesystem space in bytes otherwise.
 */
uint32_t fat16_get_fs_free(const struct fat16_fs_struct* fs)
{
    if(!fs)
        return 0;

    uint8_t fat[32];
    struct fat16_usage_count_callback_arg count_arg;
    count_arg.cluster_count = 0;
    count_arg.buffer_size = sizeof(fat);

    uint32_t fat_offset = fs->header.fat_offset;
    uint32_t fat_size = fs->header.fat_size;
    while(fat_size > 0)
    {
        uint16_t length = UINT16_MAX - 1;
        if(fat_size < length)
            length = fat_size;

        if(!fs->partition->device_read_interval(fat_offset,
            fat,
           sizeof(fat),
           length,
           fat16_get_fs_free_callback,
           &count_arg
           )
           )
        return 0;

        fat_offset += length;
        fat_size -= length;
    }

    return (uint32_t) count_arg.cluster_count * fs->header.cluster_size;
}

/**
 * \ingroup fat16_fs
 * Callback function used for counting free clusters.
 */
uint8_t fat16_get_fs_free_callback(uint8_t* buffer, uint32_t offset, void* p)
{
    struct fat16_usage_count_callback_arg* count_arg = (struct fat16_usage_count_callback_arg*) p;
    uint8_t buffer_size = count_arg->buffer_size;
    uint8_t i;
    for(i = 0; i < buffer_size; i += 2)
    {
        if((((uint16_t) buffer[1] << 8) | ((uint16_t) buffer[0] << 0)) == FAT16_CLUSTER_FREE)
            ++(count_arg->cluster_count);

        buffer += 2;
    }

    return 1;
}

uint8_t find_file_in_dir(struct fat16_fs_struct* fs, struct fat16_dir_struct* dd, const char* name, struct fat16_dir_entry_struct* dir_entry)
{
    while(fat16_read_dir(dd, dir_entry))
    {
        if(strcmp(dir_entry->long_name, name) == 0)
        {
            fat16_reset_dir(dd);
            return 1;
        }
    }

    return 0;
}

struct fat16_file_struct* open_file_in_dir(struct fat16_fs_struct* fs, struct fat16_dir_struct* dd, const char* name)
{
    struct fat16_dir_entry_struct file_entry;
    if(!find_file_in_dir(fs, dd, name, &file_entry))
        return 0;

    return fat16_open_file(fs, &file_entry);
}

int fat16_file_size(struct fat16_file_struct * file)
{
    return(file->dir_entry.file_size);
}
