/******************************************************************************/
/*  This file is part of the uVision/ARM development tools                    */
/*  Copyright KEIL ELEKTRONIK GmbH 2002-2004                                  */
/******************************************************************************/
/*                                                                            */
/*  SERIAL.C:  Low Level Serial Routines                                      */
/*  modified and extended by Martin Thomas                                    */
/*                                                                            */
/******************************************************************************/

#include "LPC177x_8x.h"
#include "lpc177x_8x_uart.h"
#include "lpc177x_8x_clkpwr.h"
#include "lpc177x_8x_pinsel.h"
#include "serial.h"

/* Initialize Serial Interface UART0 */
void init_serial0 ( unsigned long baudrate )
{
	// power UART0 up
	CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCUART0,ENABLE);

	// Initialize UART0 pin connect
	PINSEL_ConfigPin(0, 2, 1);
	PINSEL_ConfigPin(0, 3, 1);
	
    LPC_UART0->LCR = 0x83;											/* 8 bits, no Parity, 1 Stop bit     */
    unsigned long Fdiv = ( SystemCoreClock / 32 ) / baudrate ;		/* baud rate                        */
    LPC_UART0->DLM = Fdiv / 256;
    LPC_UART0->DLL = Fdiv % 256;
    LPC_UART0->LCR = 0x03;											/* DLAB = 0                         */
}

/* Write character to Serial Port 0 with \n -> \r\n  */
int putchar_serial0 (int ch)
{
    if (ch == '\n')
    {
		putc_serial0('\r');
    }

    return putc_serial0(ch);
}

/* Write character to Serial Port 0 without \n -> \r\n  */
int putc_serial0 (int ch)
{
    while (UART_CheckBusy(UART_0) == SET);

	UART_SendByte(LPC_UART0, ch);

    return ch;
}

void putstring_serial0 (const char *string)
{
    char ch;

    while ((ch = *string))
    {
        putchar_serial0(ch);
        string++;
    }
}
