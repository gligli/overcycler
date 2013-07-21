//Ryans Mods:
//	Added fat16_set_dir(...)-Allows user to set the next entry of the directory


/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FAT16_H
#define FAT16_H

#include <stdint.h>

/**
 * \addtogroup fat16
 *
 * @{
 */
/**
 * \file
 * FAT16 header.
 *
 * \author Roland Riegel
 */

/**
 * \addtogroup fat16_file
 * @{
 */

/** The file is read-only. */
#define FAT16_ATTRIB_READONLY (1 << 0)
/** The file is hidden. */
#define FAT16_ATTRIB_HIDDEN (1 << 1)
/** The file is a system file. */
#define FAT16_ATTRIB_SYSTEM (1 << 2)
/** The file is empty and has the volume label as its name. */
#define FAT16_ATTRIB_VOLUME (1 << 3)
/** The file is a directory. */
#define FAT16_ATTRIB_DIR (1 << 4)
/** The file has to be archived. */
#define FAT16_ATTRIB_ARCHIVE (1 << 5)

/** The given offset is relative to the beginning of the file. */
#define FAT16_SEEK_SET 0
/** The given offset is relative to the current read/write position. */
#define FAT16_SEEK_CUR 1
/** The given offset is relative to the end of the file. */
#define FAT16_SEEK_END 2

/**
 * @}
 */

struct partition_struct;
struct fat16_fs_struct;
struct fat16_file_struct;
struct fat16_dir_struct;

/**
 * \ingroup fat16_file
 * Describes a directory entry.
 */
struct fat16_dir_entry_struct
{
    /** The file's name, truncated to 31 characters. */
    char long_name[32];
    /** The file's attributes. Mask of the FAT16_ATTRIB_* constants. */
    uint8_t attributes;

    /** The time the file was last modified. */
    //uint16_t modified_time;
    /** The date the file was modified. */
    //uint16_t modified_date;

    /** The cluster in which the file's first byte resides. */
    uint16_t cluster;
    /** The file's size. */
    uint32_t file_size;
    /** The total disk offset of this directory entry. */
    uint32_t entry_offset;
};

struct fat16_fs_struct* 
fat16_open(struct partition_struct* partition);

void 
fat16_close(struct fat16_fs_struct* fs);

struct 
fat16_file_struct* fat16_open_file(struct fat16_fs_struct* fs, const struct fat16_dir_entry_struct* dir_entry);

void 
fat16_close_file(struct fat16_file_struct* fd);

int16_t 
fat16_read_file(struct fat16_file_struct* fd, uint8_t* buffer, uint16_t buffer_len);

int16_t 
fat16_write_file(struct fat16_file_struct* fd, const uint8_t* buffer, uint16_t buffer_len);

uint8_t 
fat16_seek_file(struct fat16_file_struct* fd, int32_t* offset, uint8_t whence);

uint8_t 
fat16_resize_file(struct fat16_file_struct* fd, uint32_t size);

struct fat16_dir_struct* 
fat16_open_dir(struct fat16_fs_struct* fs, const struct fat16_dir_entry_struct* dir_entry);

void 
fat16_close_dir(struct fat16_dir_struct* dd);

uint8_t 
fat16_read_dir(struct fat16_dir_struct* dd, struct fat16_dir_entry_struct* dir_entry);

uint8_t 
fat16_reset_dir(struct fat16_dir_struct* dd);

uint8_t 
fat16_set_dir(struct fat16_dir_struct* dd, uint16_t offset);

uint8_t 
fat16_create_file(struct fat16_dir_struct* parent, const char* file, struct fat16_dir_entry_struct* dir_entry);

uint8_t 
fat16_delete_file(struct fat16_fs_struct* fs, struct fat16_dir_entry_struct* dir_entry);

uint8_t 
fat16_get_dir_entry_of_path(struct fat16_fs_struct* fs, const char* path, struct fat16_dir_entry_struct* dir_entry);

uint32_t 
fat16_get_fs_size(const struct fat16_fs_struct* fs);

uint32_t 
fat16_get_fs_free(const struct fat16_fs_struct* fs);

uint8_t 
find_file_in_dir(struct fat16_fs_struct* fs, struct fat16_dir_struct* dd, const char* name, struct fat16_dir_entry_struct* dir_entry);

struct fat16_file_struct* 
open_file_in_dir(struct fat16_fs_struct* fs, struct fat16_dir_struct* dd, const char* name);

int fat16_file_size(struct fat16_file_struct * file);

/**
 * @}
 */

#endif

