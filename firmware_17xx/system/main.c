///////////////////////////////////////////////////////////////////////////////
// main code
///////////////////////////////////////////////////////////////////////////////

#include <sys/stat.h>

#include "main.h"

#include "LPC177x_8x.h"
#include "rprintf.h"
#include "serial.h"
#include "diskio.h"
#include "ff.h"
#include "nand.h"
#include "usb/usbapi.h"
#include "usb/usb_power.h"
#include "usb/usb_midi.h"
#include "usb/usb_msc.h"

#include "synth/synth.h"
#include "synth/ui.h"
#include "synth/utils.h"

#define IRQ_MASK 0x00000080
#define FIQ_MASK 0x00000040
#define INT_MASK (IRQ_MASK | FIQ_MASK)

#define SYNTH_TIMER_HZ 500

#define STORAGE_PATH "/STORAGE"

usbMode_t usbMode = umNone;
FATFS fatFS;

__attribute__ ((used)) void SysTick_Handler(void)
{
	synth_timerInterrupt();
}

__attribute__ ((used)) void USB_IRQHandler(void)
{
	USBHwISR();
}

void delay_us(uint32_t count)
{
	uint32_t i;

	for (i = 0; i < count; i++)
	{
		DELAY_100NS();
		DELAY_100NS();
		DELAY_100NS();
		DELAY_100NS();
		DELAY_100NS();
		DELAY_100NS();
		DELAY_100NS();
		DELAY_100NS();
		DELAY_100NS();
		DELAY_100NS();
	}
}

void delay_ms(uint32_t count)
{
	delay_us(1000*count);
}

void storage_write(uint32_t pageIdx, uint8_t *buf)
{
	FIL f;
	char fn[_MAX_LFN];
	UINT bw;
	
	snprintf(fn,_MAX_LFN,STORAGE_PATH "/page_%04x.bin",pageIdx);

#ifdef DEBUG
	rprintf(0,"storage_write %d %s\n",pageIdx,fn);
#endif		

	if(f_open(&f,fn,FA_WRITE|FA_CREATE_ALWAYS))
		return;
	f_write(&f,buf,STORAGE_PAGE_SIZE,&bw);
	f_close(&f);

#ifdef DEBUG
	if(bw!=STORAGE_PAGE_SIZE)
		rprintf(0,"storage_write %d bytes\n",bw);
#endif		
}

void storage_read(uint32_t pageIdx, uint8_t *buf)
{
	FIL f;
	char fn[_MAX_LFN];
	UINT br;

	snprintf(fn,_MAX_LFN,STORAGE_PATH "/page_%04x.bin",pageIdx);

#ifdef DEBUG
	rprintf(0,"storage_read %d %s\n",pageIdx,fn);
#endif		

	memset(buf,0,STORAGE_PAGE_SIZE);

	if(f_open(&f,fn,FA_READ|FA_OPEN_EXISTING))
		return;
	f_read(&f,buf,STORAGE_PAGE_SIZE,&br);
	f_close(&f);

#ifdef DEBUG
	if(br!=STORAGE_PAGE_SIZE)
		rprintf(0,"storage_read %d bytes\n",br);
#endif		
}

void storage_delete(uint32_t pageIdx)
{
	char fn[_MAX_LFN];

	snprintf(fn,_MAX_LFN,STORAGE_PATH "/page_%04x.bin",pageIdx);

#ifdef DEBUG
	rprintf(0,"storage_delete %d %s\n",pageIdx,fn);
#endif		

	f_unlink(fn);
}

int8_t storage_pageExists(uint32_t pageIdx)
{
#ifdef DEBUG
	rprintf(0,"storage_pageExists %d\n",pageIdx);
#endif		
	
	FIL f;
	char fn[_MAX_LFN];

	snprintf(fn,_MAX_LFN,STORAGE_PATH "/page_%04x.bin",pageIdx);
	
	if(f_open(&f,fn,FA_READ|FA_OPEN_EXISTING))
		return 0;
	
	f_close(&f);
	
	return 1;
}

int8_t storage_samePage(uint32_t pageIdx, uint32_t pageIdx2)
{
	FIL f,f2;
	char fn[_MAX_LFN];
	uint8_t buf[64],buf2[64];
	UINT br,br2;
	int8_t same=1;

#ifdef DEBUG
	rprintf(0,"storage_samePage %d %d\n",pageIdx,pageIdx2);
#endif		

	snprintf(fn,_MAX_LFN,STORAGE_PATH "/page_%04x.bin",pageIdx);
	if(f_open(&f,fn,FA_READ|FA_OPEN_EXISTING))
		return 0;
	
	snprintf(fn,_MAX_LFN,STORAGE_PATH "/page_%04x.bin",pageIdx2);
	if(f_open(&f2,fn,FA_READ|FA_OPEN_EXISTING))
		return 0;
	
	do
	{
		f_read(&f,buf,sizeof(buf),&br);
		f_read(&f2,buf2,sizeof(buf2),&br2);
		same&=memcmp(buf,buf2,sizeof(buf))==0;
	}while(same && br == sizeof(buf) && br2 == sizeof(buf2));

	f_close(&f);
	f_close(&f2);
	
	return same;
}

void usb_setMode(usbMode_t mode, usb_MSC_continue_callback_t usbMSCContinue)
{
	if(mode==usbMode)
		return;
	
#ifdef DEBUG
	rprintf(0,"usb_setMode %d\n",mode);
#endif		

	NVIC_SetPriority(USB_IRQn,17);
	NVIC_EnableIRQ(USB_IRQn);
	
	if(usbMode!=umNone || mode==umNone)
	{
		USBHwConnect(FALSE);
		delay_ms(1000);
	}
	
	switch(mode)
	{
	case umPowerOnly:
		usb_power_start();
		break;
	case umMIDI:
		usb_midi_start();
		break;
	case umMSC:
		BLOCK_INT(1)
		{
			usb_msc_start();
			for(uint32_t t=0;;++t)
			{
				USBHwISR();
				
				if(!(t&0xffff)) // every ~100ms, to ensure USB interrupts promptness
				{
					if(usbMSCContinue && !usbMSCContinue())
						break;
				}
			}
		}
		break;
	default:
		/* nothing */;
	}
	
	usbMode=mode;
}

int main(void)
{
	FRESULT res;
	
	__disable_irq();

	delay_ms(500);

	SystemCoreClockUpdate();
	init_serial0(57600);
	rprintf_devopen(0,putc_serial0); 
	
	ui_init(); // called early to get a splash screen
	
	delay_ms(500);

	rprintf(0,"\nOverCycler %d Hz\n",SystemCoreClock);
	
	rprintf(0,"storage init...\n");
	
	SysTick_Config(SystemCoreClock / SYNTH_TIMER_HZ);
	NVIC_SetPriority(SysTick_IRQn,16);
	
	if((res=disk_initialize(0)))
	{
		rprintf(0,"disk_initialize res=%d\n",res);
		rprintf(1,"Error: disk_initialize res=%d",res);
		for(;;);
	}

	if((res=f_mount(0,&fatFS)))
	{
		rprintf(0,"f_mount res=%d\n",res);
		rprintf(1,"Error: f_mount res=%d",res);
		for(;;);
	}

	if((res=f_mkdir(STORAGE_PATH)))
	{
		if(res==FR_NO_FILESYSTEM)
		{
			rprintf(0,"Formatting disk...");
			rprintf(1,"Formatting disk...");
			if((res=f_mkfs(0,0,0)))
			{
				rprintf(0,"f_mkfs res=%d\n",res);
				rprintf(1,"Error: f_mkfs res=%d",res);
				for(;;);
			}
			rprintf(0,"done!");
			rprintf(1,"done!");

			if((res=f_mkdir(STORAGE_PATH)))
			{
				rprintf(0,"f_mkdir res=%d\n",res);
				rprintf(1,"Error: f_mkdir res=%d",res);
				for(;;);
			}
		}
	}
	
	rprintf(0,"synth init...\n");

	synth_init();

	__enable_irq();
	
	rprintf(0,"done\n");
	
	for(;;)
	{
		synth_update();
	}
}

