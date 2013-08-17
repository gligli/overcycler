/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs                          */
/* (C)ChaN, 2007                                                         */
/* Copyright (c) 2010, Martin Thomas                                     */
/*-----------------------------------------------------------------------*/
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/

#include "diskio.h"
#include "spi_sd_lpc17xx.h"

// in makefile #define WITH_USB_MS   1

#if ( WITH_USB_MS == 1 )
#include "usbhost_ms.h"

USB_INT32U numBlks, blkSize;
USB_INT08U inquiryResult[INQUIRY_LENGTH];
DSTATUS usb_status = STA_NOINIT;

#endif

/*-----------------------------------------------------------------------*/
/* Correspondence between physical drive number and physical drive.      */
/*-----------------------------------------------------------------------*/

#define MMC		0
#define USB		1
#define ATA		2

/*-----------------------------------------------------------------------*/
/* disk-timer - forwarded to low-level drivers                           */
/*-----------------------------------------------------------------------*/

void disk_timerproc(void)
{
	MMC_disk_timerproc();
}

/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(BYTE drv /* Physical drive nmuber (0..) */
)
{
	DSTATUS stat;
	int result;

	(void) result;

	switch (drv)
	{
	case ATA:
		// result = ATA_disk_initialize();
		stat = STA_NOINIT;
		// translate the result code here
		return stat;

	case MMC:
		stat = MMC_disk_initialize();
		return stat;

	case USB:
#if WITH_USB_MS
		/* USB host init and enumeration */
		if ( usb_status & STA_NOINIT ) {
			Host_Init();
			if ( Host_EnumDev() == OK ) {
				/* mass-storage init */
				if ( MS_Init(&blkSize, &numBlks, inquiryResult) == OK ) {
					usb_status &= ~STA_NOINIT;
				} else {
					/* MS init fail */
				}
			} else {
				/* host init and/or enum fail */
			}
		}
		return usb_status;
#else
		stat = STA_NOINIT;
		return stat;
#endif
	}
	return STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE drv /* Physical drive number (0..) */
)
{
	DSTATUS stat;
	int result;

	(void) result;

	switch (drv)
	{
	case ATA:
		// result = ATA_disk_status();
		stat = STA_NOINIT;
		// translate the result code here

		return stat;

	case MMC:
		stat = MMC_disk_status();
		return stat;

	case USB:
#if ( WITH_USB_MS == 1 )
		stat = usb_status;
#else
		stat = STA_NOINIT;
#endif
		return stat;
	}

	return STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE drv, /* Physical drive number (0..) */
		BYTE *buff, /* Data buffer to store read data */
		DWORD sector, /* Sector address (LBA) */
		BYTE count /* Number of sectors to read (1..255) */
)
{
	DRESULT res;
	int result;

	(void) result;

	switch (drv)
	{
	case ATA:
		// result = ATA_disk_read(buff, sector, count);
		res = RES_PARERR;
		// translate the result code here
		return res;

	case MMC:
		res = MMC_disk_read(buff, sector, count);
		return res;

	case USB:
#if ( WITH_USB_MS == 1 )
		if (usb_status & STA_NOINIT) {
			res =RES_NOTRDY;
		} else {
			if ( MS_BulkRecv( sector, count, buff ) == OK ) {
				res = RES_OK;
			} else {
				res = RES_ERROR;
			}
		}
#else
		res = RES_PARERR;
#endif
		return res;
	}

	return RES_PARERR;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
/* The FatFs module will issue multiple sector transfer request
 /  (count > 1) to the disk I/O layer. The disk function should process
 /  the multiple sector transfer properly Do. not translate it into
 /  multiple single sector transfers to the media, or the data read/write
 /  performance may be drastically decreased. */

#if _READONLY == 0
DRESULT disk_write(BYTE drv, /* Physical drive number (0..) */
	const BYTE *buff, /* Data to be written */
	DWORD sector, /* Sector address (LBA) */
	BYTE count /* Number of sectors to write (1..255) */
)
{
	DRESULT res;
	int result;

	(void) result;

	switch (drv)
	{
	case ATA:
		// result = ATA_disk_write(buff, sector, count);
		res = RES_PARERR;
		// translate the result code here
		return res;

	case MMC:
		res = MMC_disk_write(buff, sector, count);
		return res;

	case USB:
#if ( WITH_USB_MS == 1 )
		if (usb_status & STA_NOINIT) return RES_NOTRDY;
		if ( MS_BulkSend( sector, count, (volatile USB_INT08U*) buff ) == OK ) {
			res = RES_OK;
		} else {
			res = RES_ERROR;
		}
#else
		res = RES_PARERR;
#endif
		return res;
	}

	return RES_PARERR;
}
#endif /* _READONLY */

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE drv, /* Physical drive number (0..) */
BYTE ctrl, /* Control code */
void *buff /* Buffer to send/receive control data */
)
{
	DRESULT res;
	int result;

	(void) result;

	switch (drv)
	{
	case ATA:
		// pre-process here
		// result = ATA_disk_ioctl(ctrl, buff);
		res = RES_PARERR;
		// post-process here
		return res;

	case MMC:
		res = MMC_disk_ioctl(ctrl, buff);
		return res;

	case USB:
#if ( WITH_USB_MS == 1 )
		//The FatFs module uses only device independent commands described below. Any device dependent function is not used.
		//Command Description
		//CTRL_SYNC Make sure that the disk drive has finished pending write process. When the disk I/O module has a write back cache, flush the dirty sector immediately. This command is not required in read-only configuration.
		//GET_SECTOR_SIZE Returns sector size of the drive into the WORD variable pointed by Buffer. This command is not required in single sector size configuration, _MAX_SS is 512.
		//GET_SECTOR_COUNT Returns total sectors on the drive into the DWORD variable pointed by Buffer. This command is used by only f_mkfs function to determine the volume size to be created.
		//GET_BLOCK_SIZE Returns erase block size of the flash memory in unit of sector into the DWORD variable pointed by Buffer. This command is used by only f_mkfs function and it attempts to align data area to the erase block boundary. The allowable value is 1 to 32768 in power of 2. Return 1 if the erase block size is unknown or disk devices.

		if (usb_status & STA_NOINIT) return RES_NOTRDY;
		res = RES_OK;
		switch(ctrl) {
			case CTRL_SYNC:
				// TODO
				break;
			case GET_SECTOR_SIZE:
				*(WORD*)buff = blkSize;
				break;
			case GET_SECTOR_COUNT:
				*(DWORD*)buff = numBlks;
				break;
			case GET_BLOCK_SIZE:
				*(DWORD*)buff = 1;
				break;
			default:
				res = RES_PARERR;
				break;
		}
#else
		res = RES_PARERR;
#endif
		return res;
	}

	return RES_PARERR;
}

