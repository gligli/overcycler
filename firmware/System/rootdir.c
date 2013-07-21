
#include "rootdir.h"
#include "fat16.h"
#include "partition.h"
#include "sd_raw.h"
#include <stdio.h>
#include "rprintf.h"

struct fat16_dir_entry_struct dir_entry;
struct fat16_fs_struct* fs;
struct partition_struct* partition;
struct fat16_dir_struct* dd;
struct fat16_file_struct * fd;

int openroot(void)
{
    /* open first partition */
    partition = partition_open((device_read_t) sd_raw_read,
                               (device_read_interval_t) sd_raw_read_interval,
                               (device_write_t) sd_raw_write,
                               0);

    if(!partition)
    {
        /* If the partition did not open, assume the storage device
             *      * is a "superfloppy", i.e. has no MBR.
             *           */
        partition = partition_open((device_read_t) sd_raw_read,
                                   (device_read_interval_t) sd_raw_read_interval,
                                   (device_write_t) sd_raw_write,
                                   -1);
        if(!partition)
        {
            rprintf("opening partition failed\n\r");
            return 1;
        }
    }

    /* open file system */
    fs = fat16_open(partition);
    if(!fs)
    {
        rprintf("opening filesystem failed\n\r");
        return 1;
    }

    /* open root directory */
    fat16_get_dir_entry_of_path(fs, "/", &dir_entry);

    dd = fat16_open_dir(fs, &dir_entry);
    if(!dd)
    {
        rprintf("opening root directory failed\n\r");
        return 1;
    }
    return 0;
}

/* returns 1 if file exists, 0 else */
int root_file_exists(char* name)
{
    return(find_file_in_dir(fs,dd,name,&dir_entry));
}

/* returns NULL if error, pointer if file opened */
struct fat16_file_struct * root_open_new(char* name)
{
    if(fat16_create_file(dd,name,&dir_entry))
    {
        return(open_file_in_dir(fs,dd,name));
    }
    else
    {
        return NULL;
    }
}

struct fat16_file_struct * root_open(char* name)
{
    return(open_file_in_dir(fs,dd,name));
}

uint8_t print_disk_info(const struct fat16_fs_struct* disk_fs)
{
    if(!disk_fs)
        return 0;

    struct sd_raw_info disk_info;
    if(!sd_raw_get_info(&disk_info))
        return 0;

//    int temp = get_output();
//    set_output(UART_ONLY);
    rprintf("manuf:  0x%02x\n\r", disk_info.manufacturer);
    rprintf("oem:    %s\n\r", disk_info.oem);
    rprintf("prod:   %s\n\r", disk_info.product);
    rprintf("rev:    %02x\n\r", disk_info.revision);
    rprintf("serial: 0x%08lx\n\r", disk_info.serial);
    rprintf("date:   %02d/%02d\n\r", disk_info.manufacturing_month, disk_info.manufacturing_year);
    rprintf("size:   %ld\n\r", disk_info.capacity);
    rprintf("copy:   %d\n\r", disk_info.flag_copy);
    rprintf("wr.pr.: %d/%d\n\r", disk_info.flag_write_protect_temp, disk_info.flag_write_protect);
    rprintf("format: %d\n\r", disk_info.format);
    rprintf("free:   %ld/%ld\n\r", fat16_get_fs_free(disk_fs), fat16_get_fs_size(disk_fs));
//    set_output(temp);
    return 1;
}

void root_disk_info(void)
{
    print_disk_info(fs);
}

/* sequential calls return sequential characters
 * of the sequence of file names in the rootdir
 * in place of '\0' it returns ',' only
 * returning a zero when the end of all files
 * has been reached.
 *
 * Assert (1) reset whenever you want to re-start
 */
char rootDirectory_files_stream(int reset)
{

    static int idx = 0;

    /* If reset, we need to reset the dir */
    if(reset)
    {
        fat16_reset_dir(dd);
        return 0;
    }

    /* Whenever IDX is zero, we're gonna start a new file,
       * so read a new one.
       * if there's no new file,
       * return 0, because it's over
       */
    if(idx == 0)
    {
        if(fat16_read_dir(dd,&dir_entry)==0)
        {
            return '\0';
        }
    }

    /* If we've reached the end of a string,
       * return comma instead of \0,
       * so the list is comma delimited,
       * and terminated with a zero
       */
    if(dir_entry.long_name[idx]=='\0')
    {
        idx = 0;
        return ',';
    }


    return dir_entry.long_name[idx++];

}
//Description: Fills buf with len number of chars.  Returns the number of files
//				that were cycled through during the read
//Pre: buf is an array of characters at least as big as len
//		len is the size of the array to read
//Post: buf contains the characters of the filenames in Root, starting at the first file
//		and ending after len characters
int rootDirectory_files(char* buf, int len)
{
    int i;
    int num=0;
    /* Loop will walk through every file in directory dd */
    fat16_reset_dir(dd);
    while(fat16_read_dir(dd,&dir_entry))
    {
        i = 0;
        /* Spin through the filename */
        while(dir_entry.long_name[i]!='\0')
        {
            /* And copy each character into buf */
            *buf++=dir_entry.long_name[i++];
            len--;
            if(len==1)
            {
                /* Buf if we ever get to the end of buf, quit */
                *buf='\0';
                return 1;
            }
        }
        *buf++=',';
        num++;
        len--;
        if(len==1)
        {
            /* Buf if we ever get to the end of buf, quit */
            *buf='\0';
            return 1;
        }
    }
    *buf='\0';
    return num;
}

void root_format(void)
{
    fat16_reset_dir(dd);
    while(fat16_read_dir(dd,&dir_entry))
    {
        fat16_delete_file(fs,&dir_entry);
        fat16_reset_dir(dd);
    }
}

int root_delete(char* filename)
{
    if(find_file_in_dir(fs,dd,filename,&dir_entry))
    {
        fat16_delete_file(fs,&dir_entry);
        return 0;
    }
    return 1;
}
