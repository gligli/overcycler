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

#define SYNTH_TIMER_HZ 2000
#define DISK_TIMER_HZ 100

FATFS fatFS;
static volatile int synthReady;

__attribute__ ((used)) void SysTick_Handler(void)
{
	static int frc=0;
	
	if(frc>=SYNTH_TIMER_HZ/DISK_TIMER_HZ)
	{
		// 100hz for disk		
		disk_timerproc();
		frc=0;
	}
	++frc;
	
	if(synthReady)
		synth_timerInterrupt();
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
	rprintf(0,"storage_write %d\n",pageIdx);
}

void storage_read(uint32_t pageIdx, uint8_t *buf)
{
	rprintf(0,"storage_read %d\n",pageIdx);
}

int main(void)
{
	FRESULT res;
	
	delay_ms(500);

	SystemCoreClockUpdate();
	
	init_serial0(230400);
    rprintf_devopen(0,putc_serial0); 
	
	rprintf(0,"\nOverCycler %d Hz\n",SystemCoreClock);
	
	rprintf(0,"storage init...\n");

	synthReady=0;
	SysTick_Config(SystemCoreClock / SYNTH_TIMER_HZ);
	NVIC_SetPriority(SysTick_IRQn,16);
	
	if((res=disk_initialize(0)))
		rprintf(0,"disk_initialize res=%d\n",res);
	
	if((res=f_mount(0,&fatFS)))
		rprintf(0,"f_mount res=%d\n",res);

	rprintf(0,"synth init...\n");

	BLOCK_INT
	{
		synth_init();
		synthReady=1;
	}
	
	rprintf(0,"done\n");
	
	for(;;)
	{
		synth_update();
	}
}

