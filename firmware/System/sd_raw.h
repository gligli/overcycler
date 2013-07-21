
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef SD_RAW_H
#define SD_RAW_H

#include "sd_raw_config.h"

/**
 * \addtogroup sd_raw
 *
 * @{
 */
/**
 * \file
 * MMC/SD raw access header.
 *
 * \author Roland Riegel
 */

/**
 * The card's layout is harddisk-like, which means it contains
 * a master boot record with a partition table.
 */
#define SD_RAW_FORMAT_HARDDISK 0
/**
 * The card contains a single filesystem and no partition table.
 */
#define SD_RAW_FORMAT_SUPERFLOPPY 1
/**
 * The card's layout follows the Universal File Format.
 */
#define SD_RAW_FORMAT_UNIVERSAL 2
/**
 * The card's layout is unknown.
 */
#define SD_RAW_FORMAT_UNKNOWN 3

/**
 * This struct is used by sd_raw_get_info() to return
 * manufacturing and status information of the card.
 */
struct sd_raw_info
{
    /**
     * A manufacturer code globally assigned by the SD card organization.
     */
    unsigned char manufacturer;
    /**
     * A string describing the card's OEM or content, globally assigned by the SD card organization.
     */
    unsigned char oem[3];
    /**
     * A product name.
     */
    unsigned char product[6];
    /**
     * The card's revision, coded in packed BCD.
     *
     * For example, the revision value \c 0x32 means "3.2".
     */
    unsigned char revision;
    /**
     * A serial number assigned by the manufacturer.
     */
    unsigned int serial;
    /**
     * The year of manufacturing.
     *
     * A value of zero means year 2000.
     */
    unsigned char manufacturing_year;
    /**
     * The month of manufacturing.
     */
    unsigned char manufacturing_month;
    /**
     * The card's total capacity in bytes.
     */
    unsigned int capacity;
    /**
     * Defines wether the card's content is original or copied.
     *
     * A value of \c 0 means original, \c 1 means copied.
     */
    unsigned char flag_copy;
    /**
     * Defines wether the card's content is write-protected.
     *
     * \note This is an internal flag and does not represent the
     *       state of the card's mechanical write-protect switch.
     */
    unsigned char flag_write_protect;
    /**
     * Defines wether the card's content is temporarily write-protected.
     *
     * \note This is an internal flag and does not represent the
     *       state of the card's mechanical write-protect switch.
     */
    unsigned char flag_write_protect_temp;
    /**
     * The card's data layout.
     *
     * See the \c SD_RAW_FORMAT_* constants for details.
     *
     * \note This value is not guaranteed to match reality.
     */
    unsigned char format;
};

typedef unsigned char (*sd_raw_interval_handler)(unsigned char* buffer, unsigned int offset, void* p);

unsigned char sd_raw_init(void);
unsigned char sd_raw_available(void);
unsigned char sd_raw_locked(void);

unsigned char sd_raw_read(unsigned int offset, unsigned char* buffer, unsigned short length);
unsigned char sd_raw_read_interval(unsigned int offset, unsigned char* buffer, unsigned short interval, unsigned short length, sd_raw_interval_handler callback, void* p);
unsigned char sd_raw_write(unsigned int offset, const unsigned char* buffer, unsigned short length);
unsigned char sd_raw_sync(void);

unsigned char sd_raw_get_info(struct sd_raw_info* info);
void SDoff(void);

char format_card(char make_sure);

/**
 * @}
 */

#endif

