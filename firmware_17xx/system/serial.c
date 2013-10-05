/******************************************************************************/
/*  This file is part of the uVision/ARM development tools                    */
/*  Copyright KEIL ELEKTRONIK GmbH 2002-2004                                  */
/******************************************************************************/
/*                                                                            */
/*  SERIAL.C:  Low Level Serial Routines                                      */
/*  modified and extended by Martin Thomas                                    */
/*                                                                            */
/******************************************************************************/

#include "LPC17xx.h"
#include "lpc17xx_uart.h"
#include "serial.h"
#include "system_LPC17xx.h"

#define CR     0x0D

/* Initialize Serial Interface UART0 */
void init_serial0 ( unsigned long baudrate )
{
    unsigned long Fdiv;

    LPC_PINCON->PINSEL0 |= 0x00000050;                  /* Enable RxD0 and TxD0              */
    LPC_UART0->LCR = 0x83;                          /* 8 bits, no Parity, 1 Stop bit     */
    Fdiv = ( SystemCoreClock / 16 ) / baudrate ;     /* baud rate                        */
    LPC_UART0->DLM = Fdiv / 256;
    LPC_UART0->DLL = Fdiv % 256;
    LPC_UART0->LCR = 0x03;                           /* DLAB = 0                         */
}

/* Write character to Serial Port 0 with \n -> \r\n  */
int putchar_serial0 (int ch)
{
    if (ch == '\n')
    {
        while (!(LPC_UART0->LSR & 0x20));
        LPC_UART0->THR = CR;                  /* output CR */
    }
    while (!(LPC_UART0->LSR & 0x20));
    return (LPC_UART0->THR = ch);
}

/* Write character to Serial Port 0 without \n -> \r\n  */
int putc_serial0 (int ch)
{
    while (!(LPC_UART0->LSR & 0x20));
    return (LPC_UART0->THR = ch);
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


/* Read character from Serial Port   */
int getkey_serial0 (void)
{
	if (LPC_UART0->LSR & 0x01)
    {
        return (LPC_UART0->RBR);
    }
    else
    {
        return 0;
    }
}

/* Read character from Serial Port   */
int getc_serial0 (void)
{
	while ( (LPC_UART0->LSR & 0x01) == 0 ); //Wait for character
	return LPC_UART0->RBR;
}
