/*
 * spi_sd_lpc17xx.h
 *
 *  Created on: 09.07.2010
 *      Author: mthomas
 */

#ifndef SPI_SD_LPC17XX_H_
#define SPI_SD_LPC17XX_H_

#include "diskio.h"

void MMC_disk_timerproc(void);

DSTATUS MMC_disk_initialize(void);
DSTATUS MMC_disk_status(void);
DSTATUS MMC_disk_ioctl(BYTE ctrl, void *buff);
DRESULT MMC_disk_read(BYTE* buff, DWORD sector, BYTE count);
DRESULT MMC_disk_write(const BYTE* buff, DWORD sector, BYTE count);

#endif /* SPI_SD_LPC17XX_H_ */
