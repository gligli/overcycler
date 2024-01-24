#include "nand.h"

#include "main.h"
#include "xt26g.h"
#include "lpc177x_8x_eeprom.h"
#include "rprintf.h"
#include "synth/utils.h"

#define NAND_EMULATED_SECTOR_BITS 9
#define NAND_EMULATED_SECTOR_SIZE (1<<NAND_EMULATED_SECTOR_BITS)
#define NAND_EMULATED_SECTOR_MASK (NAND_EMULATED_SECTOR_SIZE-1)

#define NAND_EEPROM_MAGIC 0x42381337

static uint32_t readBlockTable(uint16_t idx)
{
	uint16_t block=0xffff;
	idx<<=1;
	EEPROM_Read(idx&0x3f,idx>>6,&block,MODE_16_BIT,1);
	return block;
}

static void writeBlockTable(uint16_t idx, uint16_t block)
{
	idx<<=1;
	EEPROM_Write(idx&0x3f,idx>>6,&block,MODE_16_BIT,1);
}

static void buildUsedBlockMap(uint8_t * used_bits)
{
	memset(used_bits,0,XT26G_TOTAL_BLOCK_COUNT>>3);
	
	for(uint16_t i=0;i<XT26G_USABLE_BLOCK_COUNT;++i)
	{
		uint16_t block=readBlockTable(i);
		used_bits[block>>3]|=1<<(block&0x07);
	}
}

static int8_t isBlockUsed(uint16_t block, uint8_t * used_bits)
{
	return !!(used_bits[block>>3]&(1<<(block&0x07)));
}

static int8_t isBlockBad(uint16_t block)
{
	uint8_t badBlockMark = 0xff;
	uint8_t badBlockMark2 = 0xff;
	
	// check the first 2 pages of the block
	XT26G_readPage((uint32_t)block<<XT26G_BLOCK_BITS,1,1,&badBlockMark);
	XT26G_readPage(((uint32_t)block<<XT26G_BLOCK_BITS)+XT26G_PAGE_SIZE,1,1,&badBlockMark2);
	
	return !(badBlockMark&badBlockMark2);
}

static uint32_t findFreeBlock(void)
{
	uint8_t blockMap[XT26G_TOTAL_BLOCK_COUNT>>3];
	uint16_t block=XT26G_TOTAL_BLOCK_COUNT;
	
	buildUsedBlockMap(blockMap);
	
	for(uint16_t i=0;i<XT26G_TOTAL_BLOCK_COUNT;++i)
	{
		if(!isBlockUsed(i,blockMap)&&!isBlockBad(i))
		{
			block=i;
			break;
		}
	}
		
	if(block>=XT26G_TOTAL_BLOCK_COUNT)
	{
		rprintf(0,"nand: out of valid blocks!\n");
		rprintf(1,"nand: out of valid blocks!");
		for(;;);
	}
	
	return block;
}

static void lowLevelFormat(void)
{
	uint16_t block=0;

	rprintf(0,"nand: lowLevelFormat...\n");

	for(uint16_t i=0;i<XT26G_USABLE_BLOCK_COUNT;++i)
	{
		while(isBlockBad(block))
		{
			rprintf(0,"nand: bad block (%d)\n",block);
			++block;
		}
		
		writeBlockTable(i, block);
		++block;
	}
	
	// mark EEPROM as formatted
	uint32_t magic=NAND_EEPROM_MAGIC;
	EEPROM_Write((XT26G_USABLE_BLOCK_COUNT<<1)&0x3f,(XT26G_USABLE_BLOCK_COUNT<<1)>>6,&magic,MODE_32_BIT,1);

	rprintf(0, "done!\n");
}

static uint32_t translateAddress(uint32_t address)
{
	return (readBlockTable(address>>XT26G_BLOCK_BITS)<<XT26G_BLOCK_BITS)|(address&XT26G_BLOCK_MASK);
}

DSTATUS nand_disk_initialize(void)
{
	if(XT26G_init())
		return STA_NOINIT;
	
	EEPROM_Init();
	
	// is EEPROM formatted?	
	uint32_t magic=0;
	EEPROM_Read((XT26G_USABLE_BLOCK_COUNT<<1)&0x3f,(XT26G_USABLE_BLOCK_COUNT<<1)>>6,&magic,MODE_32_BIT,1);

	if(magic!=NAND_EEPROM_MAGIC)
		lowLevelFormat();
	
//	for(uint16_t i=0;i<XT26G_USABLE_BLOCK_COUNT;++i)
//	{
//		rprintf(0, "%04x",readBlockTable(i));
//	}
//	rprintf(0, "\n");

//	rprintf(0,"%04x\n",findFreeBlock());
		
	return 0;
}

DSTATUS nand_disk_status(void)
{
	rprintf(0,"nand_disk_status\n");

	return 0;
}

DRESULT nand_disk_ioctl(BYTE ctrl, void *buff)
{
	rprintf(0,"nand_disk_ioctl %d\n",ctrl);

	DRESULT res;
	res = RES_OK;
	switch(ctrl) {
		case CTRL_SYNC:
			/* nothing */
			break;
		case GET_SECTOR_SIZE:
			*(WORD*)buff = NAND_EMULATED_SECTOR_SIZE;
			break;
		case GET_SECTOR_COUNT:
			*(DWORD*)buff = (XT26G_USABLE_BLOCK_COUNT*XT26G_BLOCK_SIZE)/NAND_EMULATED_SECTOR_SIZE;
			break;
		case GET_BLOCK_SIZE:
			*(DWORD*)buff = XT26G_BLOCK_SIZE/XT26G_PAGE_SIZE;
			break;
		default:
			res = RES_PARERR;
			break;
	}
	return res;
}

DRESULT nand_disk_read(BYTE* buff, DWORD sector, BYTE count)
{
	rprintf(0,"nand_disk_read %x %d\n",sector,count);
	
	for(;count;--count)
	{
		uint32_t srcAddress=translateAddress(sector<<NAND_EMULATED_SECTOR_BITS);

		XT26G_readPage(srcAddress,NAND_EMULATED_SECTOR_SIZE,0,buff);

		buff+=NAND_EMULATED_SECTOR_SIZE;
		++sector;
	}

	return RES_OK;
}

DRESULT nand_disk_write(const BYTE* buff, DWORD sector, BYTE count)
{
	rprintf(0,"nand_disk_write %x %d\n",sector,count);

	for(;count;--count)
	{
		uint32_t srcAddress=translateAddress(sector<<NAND_EMULATED_SECTOR_BITS);
		uint32_t srcBlockAddress=(srcAddress>>XT26G_BLOCK_BITS)<<XT26G_BLOCK_BITS;
		uint32_t dstBlockAddress=findFreeBlock()<<XT26G_BLOCK_BITS;
		
		// erase the new location
		XT26G_eraseBlock(dstBlockAddress);
		
		// move the entire block to the new location, merging in the sector write
		for(uint32_t pageAddr=0;pageAddr<XT26G_BLOCK_SIZE;pageAddr+=XT26G_PAGE_SIZE)
		{
			if(srcAddress>=srcBlockAddress+pageAddr && srcAddress<srcBlockAddress+pageAddr+XT26G_PAGE_SIZE)
			{
				// this is the page where the sector must be written
				XT26G_movePage(srcAddress,dstBlockAddress+pageAddr,NAND_EMULATED_SECTOR_SIZE,(uint8_t*)buff);
			}
			else
			{
				// nothing to update in the page, just move it
				XT26G_movePage(srcBlockAddress+pageAddr,dstBlockAddress+pageAddr,0,NULL);
			}
		}
				
		// update block table with new raw block index
		writeBlockTable((sector<<NAND_EMULATED_SECTOR_BITS)>>XT26G_BLOCK_BITS,dstBlockAddress>>XT26G_BLOCK_BITS);
		
		buff+=NAND_EMULATED_SECTOR_SIZE;
		++sector;
	}

	return RES_OK;
}

void nand_test(void)
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