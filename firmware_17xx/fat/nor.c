#include <string.h>

#include "nor.h"

#include "main.h"
#include "w25q.h"
#include "rprintf.h"
#include "synth/utils.h"

DSTATUS nor_disk_initialize(void)
{
	if(W25Q_init())
		return STA_NOINIT;
	
	return 0;
}

DSTATUS nor_disk_status(void)
{
//	rprintf(0,"nand_disk_status\n");

	return 0;
}

DRESULT nor_disk_ioctl(BYTE ctrl, void *buff)
{
//	rprintf(0,"nor_disk_ioctl %d\n",ctrl);

	DRESULT res;
	res=RES_OK;
	switch(ctrl) {
		case CTRL_SYNC:
			/* nothing */
			break;
		case GET_SECTOR_SIZE:
			*(WORD*)buff=W25Q_SECTOR_SIZE;
			break;
		case GET_SECTOR_COUNT:
			*(DWORD*)buff=W25Q_SECTOR_COUNT;
			break;
		case GET_BLOCK_SIZE:
			*(DWORD*)buff=1;
			break;
		default:
			res=RES_PARERR;
			break;
	}
	return res;
}

DRESULT nor_disk_read(BYTE* buff, DWORD sector, BYTE count)
{
//	rprintf(0,"nor_disk_read %x %d\n",sector,count);
	
	for(;count;--count)
	{
		W25Q_readSector(sector,buff);

		buff+=W25Q_SECTOR_SIZE;
		++sector;
	}
	
	return RES_OK;
}

DRESULT nor_disk_write(const BYTE* buff, DWORD sector, BYTE count)
{
//	rprintf(0,"nor_disk_write %x %d\n",sector,count);

	for(;count;--count)
	{
		W25Q_writeSector(sector,buff);
		
		buff+=W25Q_SECTOR_SIZE;
		++sector;
	}

	return RES_OK;
}

void nor_test(void)
{
	FRESULT res;
	FIL f;
	UINT bw;
	char buf[STORAGE_PAGE_SIZE];

	strcpy(buf, "Test 1234 Although NAND Flash memory devices may contain bad blocks, they can be used reliably in systems that provide bad-block management and error-correction algorithms. This ensures data integrity.");
	if((res=f_open(&f,"/test.bin",FA_WRITE|FA_CREATE_ALWAYS)))
	{
		rprintf(0,"f_open %d\n",res);
		for(;;);
	}
	f_write(&f,buf,STORAGE_PAGE_SIZE,&bw);
	f_close(&f);
	
	memset(buf,0,STORAGE_PAGE_SIZE);

	if((res=f_open(&f,"/test.bin",FA_READ|FA_OPEN_EXISTING)))
	{
		rprintf(0,"f_open %d\n",res);
		for(;;);
	}
	f_lseek(&f,10);
	f_read(&f,buf,STORAGE_PAGE_SIZE,&bw);
	f_close(&f);
	rprintf(0,"%s\n",buf);

	if(strncmp(buf,"Although", 8))
	{
		rprintf(0,"strncmp\n");
		for(;;);
	}
	
	if((res=f_unlink("/test.bin")))
	{
		rprintf(0,"f_unlink %d\n",res);
		for(;;);
	}
}