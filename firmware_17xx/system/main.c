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
    rprintf_devopen(putc_serial0); 
	
	rprintf("\nOverCycler\n");
	
	rprintf("storage init...\n");

	SysTick_Config(SystemCoreClock / 100); // 10ms ticks for disk access
	
	if((res=disk_initialize(0)))
		rprintf("disk_initialize res=%d\n",res);
	
	if((res=f_mount(0,&fatFS)))
		rprintf("f_mount res=%d\n",res);

	rprintf("synth init...\n");

	__disable_irq();
	
	synth_init();
	
	__enable_irq();

	rprintf("done\n");
	
	for(;;)
	{
		synth_update();
	}
}

