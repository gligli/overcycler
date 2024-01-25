#include <time.h>

#include "xt26g.h"

#include "main.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_ssp.h"
#include "lpc177x_8x_pinsel.h"
#include "rprintf.h"

#define CMD_WRITE_ENABLE 0x06
#define CMD_WRITE_DISABLE 0x04
#define CMD_GET_FEATURES 0x0F
#define CMD_SET_FEATURES 0x1F
#define CMD_PAGE_READ 0x13
#define CMD_READ_FROM_CACHE 0x03
#define CMD_READ_ID 0x9F
#define CMD_READ_UID 0x4B
#define CMD_PROGRAM_LOAD 0x02
#define CMD_PROGRAM_LOAD_RANDOM_DATA 0x84
#define CMD_PROGRAM_EXECUTE 0x10
#define CMD_BLOCK_ERASE 0xD8
#define CMD_RESET 0xFF

#define STATUS_OIP 1
#define STATUS_E_FAIL 4
#define STATUS_P_FAIL 8

static inline void csSet(uint32_t dummy)
{
	DELAY_50NS();

	GPIO_OutputValue(0,1<<6,1);
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
	while(SSP_GetStatus(LPC_SSP1,SSP_STAT_TXFIFO_EMPTY)==RESET)
	{
		// wait until previous bytes are transferred
	}
	
	SSP_SendData(LPC_SSP1,value);

	while(SSP_GetStatus(LPC_SSP1,SSP_STAT_RXFIFO_NOTEMPTY)==RESET)
	{
		// wait for new byte
	}

	return SSP_ReceiveData(LPC_SSP1);
}

static void send16(uint16_t value)
{
	sendRead8(value>>8);
	sendRead8(value&0xff);
}

static void send24(uint32_t value)
{
	sendRead8((value>>16)&0xff);
	sendRead8((value>>8)&0xff);
	sendRead8(value&0xff);
}

static void waitOIP(void)
{
	uint8_t status;
	
	do
	{
		HANDLE_CS
		{
			sendRead8(CMD_GET_FEATURES);
			sendRead8(0xc0);
			status=sendRead8(0x00);
		}
	}
	while(status&STATUS_OIP);
	
	if(status&STATUS_E_FAIL)
	{
		rprintf(0,"XT26G: erase failure has occurred\n");
		for(;;);
	}

	if(status&STATUS_P_FAIL)
	{
		rprintf(0,"XT26G: program failure has occurred\n");
		for(;;);
	}
}

static void enableWrites(void)
{
	HANDLE_CS
	{
		sendRead8(CMD_WRITE_ENABLE);
	}
}

void XT26G_readPage(uint32_t address, uint16_t size, int8_t is_spare, uint8_t * buffer)
{
	HANDLE_CS
	{
		sendRead8(CMD_PAGE_READ);
		send24(address>>XT26G_PAGE_BITS);
	}

	waitOIP();

	HANDLE_CS
	{
		sendRead8(CMD_READ_FROM_CACHE);
		send16((address&XT26G_PAGE_MASK) | ((is_spare)?(1<<XT26G_PAGE_BITS):0));
		sendRead8(0x00); // dummy byte
		
		for(;size;--size)
		{
			*buffer++=sendRead8(0x00);
		}
	}
}

void XT26G_writePage(uint32_t address, uint16_t size, uint8_t * buffer)
{
	HANDLE_CS
	{
		sendRead8(CMD_PROGRAM_LOAD);
		send16(address&XT26G_PAGE_MASK);

		for(;size;--size)
		{
			sendRead8(*buffer++);
		}
	}

	enableWrites();
	
	HANDLE_CS
	{
		sendRead8(CMD_PROGRAM_EXECUTE);
		send24(address>>XT26G_PAGE_BITS);
	}

	waitOIP();
}

void XT26G_eraseBlock(uint32_t address)
{
	enableWrites();
	
	HANDLE_CS
	{
		sendRead8(CMD_BLOCK_ERASE);
		send24(address>>XT26G_PAGE_BITS);
	}

	waitOIP();
}

void XT26G_movePage(uint32_t address, uint32_t newAddress, uint16_t size, uint8_t * buffer)
{
	HANDLE_CS
	{
		sendRead8(CMD_PAGE_READ);
		send24(address>>XT26G_PAGE_BITS);
	}

	waitOIP();

	HANDLE_CS
	{
		sendRead8(CMD_PROGRAM_LOAD_RANDOM_DATA);
		send16(address&XT26G_PAGE_MASK);

		for(;size;--size)
		{
			sendRead8(*buffer++);
		}
	}
	
	enableWrites();
	
	HANDLE_CS
	{
		sendRead8(CMD_PROGRAM_EXECUTE);
		send24(newAddress>>XT26G_PAGE_BITS);
	}

	waitOIP();
}

void XT26G_getUniqueID(uint8_t * buffer)
{
	HANDLE_CS
	{
		sendRead8(CMD_READ_UID);
		sendRead8(0x00);
		sendRead8(0x00);
		sendRead8(0x00);
		sendRead8(0x00);

		for(uint8_t size=16;size;--size)
		{
			*buffer++=sendRead8(0x00);
		}
	}
}

int8_t XT26G_init(void)
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
	SSP_ConfigStruct.ClockRate=10000000;
	// Initialize SSP peripheral with parameter given in structure above
	SSP_Init(LPC_SSP1,&SSP_ConfigStruct);
	// Enable SD_SSP peripheral
	SSP_Cmd(LPC_SSP1,ENABLE);
	
	// reset the XT26G

	HANDLE_CS
	{
		sendRead8(CMD_RESET);
	}
	waitOIP();
		
	// get model / device id

	uint8_t mid, did;

	HANDLE_CS
	{
		sendRead8(CMD_READ_ID);
		sendRead8(0x00);
		mid=sendRead8(0x00);
		did=sendRead8(0x00);
	}
	
	rprintf(0, "XT26G: mid %02x did %02x\n", mid, did);
	
	if(mid!=0x0b || did!=0x12)
		return 1;
	
	// deprotect blocks from writing
	
	HANDLE_CS
	{
		sendRead8(CMD_SET_FEATURES);
		sendRead8(0xa0);
		sendRead8(0x00);
	}

	return 0;
}

/*
void XT26G_test(void)
{
	uint8_t buf[2048];

	XT26G_readPage(0, 128, 1, buf);
	buffer_dump(buf, 128);		

	for(int i = 0; i < 2048; ++i)
		XT26G_readPage(i<<XT26G_BLOCK_BITS, 1, 1, &buf[i]);
	buffer_dump(buf, 2048);		
	
	XT26G_eraseBlock(0);
	
	XT26G_readPage(0, 2048, 0, buf);
	buffer_dump(buf, 2048);		
	
	strcpy(buf, "Test 1234 Although NAND Flash memory devices may contain bad blocks, they can be used reliably in systems that provide bad-block management and error-correction algorithms. This ensures data integrity.");
	XT26G_writePage(0, 2048, buf);
	buffer_dump(buf, 2048);		
			
	XT26G_readPage(0, 2048, 0, buf);
	buffer_dump(buf, 2048);		

	XT26G_movePage(5, 0x800, 4, "    ");
	
	XT26G_readPage(0x800, 2048, 0, buf);
	buffer_dump(buf, 2048);		

	XT26G_eraseBlock(0);
	
	XT26G_readPage(0, 2048, 0, buf);
	buffer_dump(buf, 2048);		
}
*/