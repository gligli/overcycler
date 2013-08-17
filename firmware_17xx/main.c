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

FATFS Fatfs[_DRIVES];		/* File system object for each logical drive */

__attribute__ ((used)) void SysTick_Handler(void)
{
	disk_timerproc();
}

void delay_us(uint32_t count)
{
    uint32_t i;
	
    for (i = 0; i < count; i++)
	{
		__asm volatile ("nop\nnop\nnop\nnop\nnop\nnop\n");
		__asm volatile ("nop\nnop\nnop\nnop\nnop\nnop\n");
		__asm volatile ("nop\nnop\nnop\nnop\nnop\nnop\n");
		__asm volatile ("nop\nnop\nnop\nnop\nnop\nnop\n");
		__asm volatile ("nop\nnop\nnop\nnop\nnop\nnop\n");
		__asm volatile ("nop\nnop\nnop\nnop\nnop\nnop\n");
		__asm volatile ("nop\nnop\nnop\nnop\nnop\nnop\n");
		__asm volatile ("nop\nnop\nnop\nnop\nnop\nnop\n");
		__asm volatile ("nop\nnop\nnop\nnop\nnop\nnop\n");
		__asm volatile ("nop\nnop\nnop\nnop\nnop\nnop\n");
		__asm volatile ("nop\nnop\nnop\nnop\nnop\nnop\n");
		__asm volatile ("nop\nnop\nnop\nnop\nnop\nnop\n");
	}
}

void delay_ms (uint32_t dlyTicks)
{
	delay_us(1000*dlyTicks);
}

int main (void)
{
	FRESULT res;
	
	SystemCoreClockUpdate();

	init_serial0(115200);
    rprintf_devopen(putc_serial0); 
	
	rprintf("\nOverCycler %d\n",SystemCoreClock);
	
	rprintf("storage init...\n");

	SysTick_Config(SystemCoreClock / 100); // 10ms ticks for disk access
	
	res=disk_initialize(0);
	rprintf("disk_initialize res=%d\n",res);
	
	res=f_mount(0,&Fatfs[0]);
	rprintf("f_mount res=%d\n",res);

	rprintf("synth init...\n");

	synth_init();
	
	rprintf("done\n");
	
	for(;;)
	{
		synth_update();
	}
}

