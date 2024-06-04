#include <time.h>

#include "w25q.h"

#include "main.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_ssp.h"
#include "lpc177x_8x_pinsel.h"
#include "rprintf.h"

#define CMD_WRITE_ENABLE 0x06
#define CMD_WRITE_DISABLE 0x04
#define CMD_ENTER_4BA_MODE 0xB7
#define CMD_GLOBAL_SECTOR_UNLOCK 0x98
#define CMD_READ_ID 0x9F
#define CMD_READ_STATUS 0x05
#define CMD_FAST_READ 0x0B
#define CMD_SECTOR_ERASE 0x20
#define CMD_PAGE_PROGRAM 0x02
#define CMD_ENABLE_RESET 0x66
#define CMD_RESET 0x99

#define STATUS_BUSY 1

static inline void csSet(uint32_t dummy)
{
	GPIO_OutputValue(0,1<<6,1);
	
	DELAY_50NS();
}

static inline uint32_t csClear(void)
{
	GPIO_OutputValue(0,1<<6,0);
	
	DELAY_50NS();

	return 0;
}

#define HANDLE_CS for(uint32_t __ctr=1,__dummy=csClear();__ctr;csSet(__dummy),__ctr=0)

static uint8_t sendRead8(uint8_t value)
{
	SSP_DATA_SETUP_Type sds;
	uint8_t res=0;
	
	sds.length=1;
	sds.tx_data=&value;
	sds.rx_data=&res;
	
	if (SSP_ReadWrite(LPC_SSP1,&sds,SSP_TRANSFER_POLLING)!=1)
	{
		rprintf(0,"W25Q: read/write failure has occurred\n");
		for(;;);
	}
	
	return res;
}

static void send32(uint32_t value)
{
	sendRead8((value>>24)&0xff);
	sendRead8((value>>16)&0xff);
	sendRead8((value>>8)&0xff);
	sendRead8(value&0xff);
}

static int8_t waitBUSY(void)
{
	uint8_t status;
	
	do
	{
		HANDLE_CS
		{
			sendRead8(CMD_READ_STATUS);
			status=sendRead8(0x00);
		}
	}
	while(status&STATUS_BUSY);
	
	return 0;
}

static void enableWrites(void)
{
	HANDLE_CS
	{
		sendRead8(CMD_WRITE_ENABLE);
	}
}

static void reset(void)
{
	HANDLE_CS
	{
		sendRead8(CMD_ENABLE_RESET);
	}

	HANDLE_CS
	{
		sendRead8(CMD_RESET);
	}
	
	waitBUSY();
}

void W25Q_readSector(uint32_t index, uint8_t * buffer)
{
	HANDLE_CS
	{
		sendRead8(CMD_FAST_READ);
		send32(index<<W25Q_SECTOR_BITS);
		sendRead8(0x00); // dummy byte		
		
		for(int size=0;size<W25Q_SECTOR_SIZE;++size)
		{
			*buffer++=sendRead8(0x00);
		}
	}
}

void W25Q_writeSector(uint32_t index, const uint8_t * buffer)
{
	// erase sector
	
	enableWrites();
	
	HANDLE_CS
	{
		sendRead8(CMD_SECTOR_ERASE);
		send32(index<<W25Q_SECTOR_BITS);
	}

	waitBUSY();
	
	// program individual pages

	for(int pageAddr=0;pageAddr<W25Q_SECTOR_SIZE;pageAddr+=W25Q_PAGE_SIZE)
	{
		enableWrites();
		
		HANDLE_CS
		{
			sendRead8(CMD_PAGE_PROGRAM);
			send32((index<<W25Q_SECTOR_BITS)+pageAddr);
			for(int size=0;size<W25Q_PAGE_SIZE;++size)
			{
				sendRead8(*buffer++);
			}
		}
		
		waitBUSY();
	}
}

//void W25Q_test(void)
//{
//	uint8_t buf[W25Q_SECTOR_SIZE];
//
//	memset(buf, 0, W25Q_SECTOR_SIZE);
//	W25Q_writeSector(42, buf);
//
//	W25Q_readSector(42, buf);
//	rprintf(0, "sector 42 before write\n");
//	buffer_dump(buf, W25Q_SECTOR_SIZE);		
//
//	memset(buf, 0, W25Q_SECTOR_SIZE);
//	strcpy(buf, "Test 1234 Although NAND Flash memory devices may contain bad blocks, they can be used reliably in systems that provide bad-block management and error-correction algorithms. This ensures data integrity.");
//	W25Q_writeSector(42, buf);
//			
//	W25Q_readSector(41, buf);
//	rprintf(0, "sector 41 after write\n");
//	buffer_dump(buf, W25Q_SECTOR_SIZE);		
//
//	W25Q_readSector(42, buf);
//	rprintf(0, "sector 42 after write\n");
//	buffer_dump(buf, W25Q_SECTOR_SIZE);		
//}

int8_t W25Q_init(void)
{
	//Configure pins and gpios
	PINSEL_ConfigPin(0,6,0);
	PINSEL_ConfigPin(0,7,2);
	PINSEL_ConfigPin(0,8,2);
	PINSEL_ConfigPin(0,9,2);
	
	GPIO_SetDir(0,1<<6,GPIO_DIRECTION_OUTPUT);
	GPIO_OutputValue(0,1<<6,1);

	// SSP Configuration structure variable
	SSP_CFG_Type SSP_ConfigStruct;
	// initialize SSP configuration structure to default
	SSP_ConfigStructInit(&SSP_ConfigStruct);
	SSP_ConfigStruct.ClockRate=15000000;
	// Initialize SSP peripheral with parameter given in structure above
	SSP_Init(LPC_SSP1,&SSP_ConfigStruct);
	// Enable SD_SSP peripheral
	SSP_Cmd(LPC_SSP1,ENABLE);
	
	// reset the W25Q

	reset();
		
	// get model / device id

	uint8_t mid;
	uint16_t did;

	HANDLE_CS
	{
		sendRead8(CMD_READ_ID);
		mid=sendRead8(0x00);
		did=sendRead8(0x00)<<8;
		did|=sendRead8(0x00);
	}
	
	rprintf(0, "W25Q: mid %02x did %04x\n", mid, did);
	
	if(mid!=0xef || did!=0x4020)
		return 1;
	
	HANDLE_CS
	{
		sendRead8(CMD_ENTER_4BA_MODE);
	}

//	W25Q_test();
	
	return 0;
}
