///////////////////////////////////////////////////////////////////////////////
// main code
///////////////////////////////////////////////////////////////////////////////

#include "main.h"

#include "LPC17xx.h"
#include "rprintf.h"
#include "serial.h"
#include "diskio.h"
#include "ff.h"

#include "synth/synth.h"

#define IRQ_MASK 0x00000080
#define FIQ_MASK 0x00000040
#define INT_MASK (IRQ_MASK | FIQ_MASK)

FATFS fatFS;

__attribute__ ((used)) void SysTick_Handler(void)
{
	disk_timerproc();
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

int main (void)
{
	FRESULT res;
	
	SystemCoreClockUpdate();

	init_serial0(230400);
    rprintf_devopen(0,putc_serial0); 
	
	rprintf(0,"\nOverCycler\n");
	
	rprintf(0,"storage init...\n");

	SysTick_Config(SystemCoreClock / 100); // 10ms ticks for disk access
	
	if((res=disk_initialize(0)))
		rprintf(0,"disk_initialize res=%d\n",res);
	
	if((res=f_mount(0,&fatFS)))
		rprintf(0,"f_mount res=%d\n",res);

	rprintf(0,"synth init...\n");

	synth_init();

	rprintf(0,"done\n");
	
	for(;;)
	{
		synth_update();
	}
}

