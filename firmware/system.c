/******************************************************************************/
/*                							                                  */
/*		  			Copyright Spark Fun Electronics                           */
/******************************************************************************/
#include "system.h"
#include "LPC214x.h"

//Debug UART0
#include "rprintf.h"
#include "serial.h"

#include "sd_raw.h"

unsigned char charY;
unsigned char charX;

//Short delay
void delay_ms(int count)
{
    int i;
    count *= 10000;
    for (i = 0; i < count; i++)
    {
        asm volatile ("nop");
    }
}

void boot_up(void)
{
    //Initialize the MCU clock PLL
    system_init();

    //Init UART0 for debug
    PINSEL0 |= 0x00000005; //enable uart0
    U0LCR = 0x83; // 8 bits, no Parity, 1 Stop bit, DLAB = 1 
    U0DLM = 0x00; 
    U0DLL = 0x20; // 115200 Baud Rate @ 58982400 VPB Clock  
    U0LCR = 0x03; // DLAB = 0                          

    //Init rprintf
    rprintf_devopen(putc_serial0); 
    rprintf("\nOverCycler\n");
}

/**********************************************************
  Initialize
 **********************************************************/

#define PLOCK 0x400

void system_init(void)
{
    // Setting Multiplier and Divider values
    PLLCFG=0x24;
    feed();

    // Enabling the PLL */
    PLLCON=0x1;
    feed();

    // Wait for the PLL to lock to set frequency
    while(!(PLLSTAT & PLOCK)) ;

    // Connect the PLL as the clock source
    PLLCON=0x3;
    feed();

    // Enabling MAM and setting number of clocks used for Flash memory fetch
    MAMTIM=0x2;
    MAMCR=0x2;

    // Setting peripheral Clock (pclk) to System Clock (cclk)
    VPBDIV=0x1;
}

void feed(void)
{
    PLLFEED=0xAA;
    PLLFEED=0x55;
}

void reset(void)
{
    // Intentionally fault Watchdog to trigger a reset condition
    WDMOD |= 3;
    WDFEED = 0xAA;
    WDFEED = 0x55;
    WDFEED = 0xAA;
    WDFEED = 0x00;
}

#define IRQ_MASK 0x00000080
#define FIQ_MASK 0x00000040
#define INT_MASK (IRQ_MASK | FIQ_MASK)

static inline unsigned long __get_cpsr(void)
{
	unsigned long retval;
	asm volatile (" mrs %0, cpsr" : "=r" (retval) : /* no inputs */ );
	return retval;
}

static inline void __set_cpsr(unsigned long val)
{
	asm volatile (" msr cpsr, %0" : /* no outputs */ : "r" (val) );
}

unsigned long disableInts(void)
{
	unsigned long _cpsr;

	_cpsr = __get_cpsr();
	__set_cpsr(_cpsr | INT_MASK);
	return _cpsr;
}

unsigned long enableInts(void)
{
	unsigned _cpsr;

	_cpsr = __get_cpsr();
	__set_cpsr(_cpsr & ~INT_MASK);
	return _cpsr;
}

unsigned long restoreInts(unsigned long oldCPSR)
{
	unsigned long _cpsr;

	_cpsr = __get_cpsr();
	__set_cpsr((_cpsr & ~INT_MASK) | (oldCPSR & INT_MASK));
	return _cpsr;
}
