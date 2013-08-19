///////////////////////////////////////////////////////////////////////////////
// LCD / keypad / potentiometers user interface
///////////////////////////////////////////////////////////////////////////////

#include "ui.h"

void ui_init(void)
{
	// init screen serial
	
	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_UART1,CLKPWR_PCLKSEL_CCLK_DIV_1);	
	init_serial1(115200);
	
	PINSEL_SetPinFunc(0,15,1);
	PINSEL_SetPinFunc(0,16,1);
	
	rprintf_devopen(1,putc_serial1);
	
	// init screen
	
	rprintf(1,"\r\x1b[c"); // reset
	delay_ms(500); // init time
	rprintf(1,"\x1b[20c"); // screen size (20 cols)
	rprintf(1,"\x1b[4L"); // screen size (4 rows)
	rprintf(1,"\x1b[?25I"); // hide cursor
	rprintf(1,"\x1b[1x"); // disable scrolling
	rprintf(1,"\x1b[2J"); // clear screen

	// welcome message
	
	rprintf(1,"    GliGli's DIY    ");
	rprintf(1,"                    ");
	rprintf(1,"-={[ OverCycler ]}=-");
	rprintf(1,"                    ");
}

void ui_update(void)
{
}

