///////////////////////////////////////////////////////////////////////////////
// main code
///////////////////////////////////////////////////////////////////////////////

#include "main.h"

#include "LPC177x_8x.h"
#include "rprintf.h"
#include "serial.h"
#include "diskio.h"
#include "ff.h"
#include "usb/usbapi.h"
#include "usb/usb_power.h"
#include "usb/usb_midi.h"
#include "usb/usb_msc.h"

#include "synth/synth.h"
#include "synth/ui.h"
#include "synth/utils.h"

usbMode_t usbMode = umNone;
FATFS fatFS;

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

void usb_setMode(usbMode_t mode, usb_MSC_continue_callback_t usbMSCContinue)
{
	if(mode==usbMode)
		return;
	
	rprintf(0,"usb_setMode %d\n",mode);

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

	init_serial0(DEBUG_UART_BAUD);
	rprintf_devopen(0,putc_serial0); 
	
	ui_init(); // called early to get a splash screen
	
	delay_ms(500);

	rprintf(0,"\nOverCycler %d Hz\n",SystemCoreClock);
	
	rprintf(0,"storage init...\n");
	
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

	if((res=f_mkdir(SYNTH_WAVEDATA_PATH)))
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

			if((res=f_mkdir(SYNTH_WAVEDATA_PATH)))
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

