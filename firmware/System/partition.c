
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "partition.h"

#include <stdlib.h>
#include <string.h>

/**
 * \addtogroup partition Partition table support
 *
 * Support for reading partition tables and access to partitions.
 *
 * @{
 */
/**
 * \file
 * Partition table implementation.
 *
 * \author Roland Riegel
 */

/**
 * Opens a partition.
 *
 * Opens a partition by its index number and returns a partition
 * handle which describes the opened partition.
 *
 * \note This function does not support extended partitions.
 *
 * \param[in] device_read A function pointer which is used to read from the disk.
 * \param[in] device_read_interval A function pointer which is used to read in constant intervals from the disk.
 * \param[in] device_write A function pointer which is used to write to the disk.
 * \param[in] index The index of the partition which should be opened, range 0 to 3.
 *                  A negative value is allowed as well. In this case, the partition opened is
 *                  not checked for existance, begins at offset zero, has a length of zero
 *                  and is of an unknown type.
 * \returns 0 on failure, a partition descriptor on success.
 * \see partition_close
 */
struct partition_struct* partition_open(device_read_t device_read, device_read_interval_t device_read_interval, device_write_t device_write, int8_t index0)
{
    struct partition_struct* new_partition = 0;
    uint8_t buffer[0x10];

    if(!device_read || !device_read_interval || index0 >= 4)
        return 0;

    if(index0 >= 0)
    {
        /* read specified partition table index */
        if(!device_read(0x01be + index0 * 0x10, buffer, sizeof(buffer)))
            return 0;

        /* abort on empty partition entry */
        if(buffer[4] == 0x00)
            return 0;
    }

    /* allocate partition descriptor */
    new_partition = malloc(sizeof(*new_partition));
    if(!new_partition)
        return 0;
    memset(new_partition, 0, sizeof(*new_partition));

    /* fill partition descriptor */
    new_partition->device_read = device_read;
    new_partition->device_read_interval = device_read_interval;
    new_partition->device_write = device_write;

    if(index0 >= 0)
    {
        new_partition->type = buffer[4];
        new_partition->offset = ((uint32_t) buffer[8]) |
                                ((uint32_t) buffer[9] << 8) |
                                ((uint32_t) buffer[10] << 16) |
                                ((uint32_t) buffer[11] << 24);
        new_partition->length = ((uint32_t) buffer[12]) |
                                ((uint32_t) buffer[13] << 8) |
                                ((uint32_t) buffer[14] << 16) |
                                ((uint32_t) buffer[15] << 24);
    }
    else
    {
        new_partition->type = 0xff;
    }

    return new_partition;
}

/**
 * Closes a partition.
 *
 * This function destroys a partition descriptor which was
 * previously obtained from a call to partition_open().
 * When this function returns, the given descriptor will be
 * invalid.
 *
 * \param[in] partition The partition descriptor to destroy.
 * \returns 0 on failure, 1 on success.
 * \see partition_open
 */
uint8_t partition_close(struct partition_struct* partition)
{
    if(!partition)
        return 0;

    /* destroy partition descriptor */
    free(partition);

    return 1;
}

/**
 * @}
 */

