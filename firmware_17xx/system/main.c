///////////////////////////////////////////////////////////////////////////////
// main code
///////////////////////////////////////////////////////////////////////////////

#include <sys/stat.h>

#include "main.h"

#include "LPC17xx.h"
#include "rprintf.h"
#include "serial.h"
#include "diskio.h"
#include "ff.h"

#include "synth/synth.h"
#include "synth/ui.h"

#define IRQ_MASK 0x00000080
#define FIQ_MASK 0x00000040
#define INT_MASK (IRQ_MASK | FIQ_MASK)

#define SYNTH_TIMER_HZ 2000
#define DISK_TIMER_HZ 100

#define STORAGE_PATH "/STORAGE"

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
	FIL f;
	char fn[_MAX_LFN];
	UINT bw;
	
	snprintf(fn,_MAX_LFN,STORAGE_PATH "/page_%04x.bin",pageIdx);

	rprintf(0,"storage_write %d %s\n",pageIdx,fn);

	if(f_open(&f,fn,FA_WRITE|FA_CREATE_ALWAYS))
		return;
	f_write(&f,buf,STORAGE_PAGE_SIZE,&bw);
	f_close(&f);

	rprintf(0,"%d bytes\n",bw);
}

void storage_read(uint32_t pageIdx, uint8_t *buf)
{
	FIL f;
	char fn[_MAX_LFN];
	UINT br;

	snprintf(fn,_MAX_LFN,STORAGE_PATH "/page_%04x.bin",pageIdx);

	rprintf(0,"storage_read %d %s\n",pageIdx,fn);

	memset(buf,0,STORAGE_PAGE_SIZE);

	if(f_open(&f,fn,FA_READ|FA_OPEN_EXISTING))
		return;
	f_read(&f,buf,STORAGE_PAGE_SIZE,&br);
	f_close(&f);

	rprintf(0,"%d bytes\n",br);
}

int main(void)
{
	FRESULT res;
	
	ui_init(); // called early to get a splash screen
	
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
	{
		rprintf(0,"disk_initialize res=%d\n",res);
		rprintf(1,"Error: disk_initialize res=%d",res);
	}
	
	if((res=f_mount(0,&fatFS)))
	{
		rprintf(0,"f_mount res=%d\n",res);
		rprintf(1,"Error: f_mount res=%d",res);
	}

	f_mkdir(STORAGE_PATH);
	
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

