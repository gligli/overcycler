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

/* Routine Block for startup code */
/* Define catchall routines for vector table */
void IRQ_Routine (void)   __attribute__ ((interrupt("IRQ")));
void IRQ_Routine (void)
{
}

void FIQ_Routine (void)   __attribute__ ((interrupt("FIQ")));
void FIQ_Routine (void)
{
}

void SWI_Routine (void)   __attribute__ ((interrupt("SWI")));
void SWI_Routine (void)
{
}

void UNDEF_Routine (void) __attribute__ ((interrupt("UNDEF")));
void UNDEF_Routine (void)
{
};

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

	IODIR0 |= (1 << 31);
	IOCLR0 |= (1 << 31); //Turn on USB LED

    //Init UART0 for debug
    PINSEL0 |= 0x00000005; //enable uart0
    U0LCR = 0x83; // 8 bits, no Parity, 1 Stop bit, DLAB = 1 
    U0DLM = 0x00; 
    U0DLL = 0x20; // 115200 Baud Rate @ 58982400 VPB Clock  
    U0LCR = 0x03; // DLAB = 0                          

    //Init rprintf
    rprintf_devopen(putc_serial0); 
    rprintf("\n\n\nOverCycler\n");

	//IOSET0 |= (1 << 31); //Turn off USB LED
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

    // Enabling MAM and setting number of clocks used for Flash memory fetch (4 cclks in this case)
    //MAMTIM=0x3; //VCOM?
    MAMCR=0x2;
    MAMTIM=0x4; //Original

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
