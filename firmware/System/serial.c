/******************************************************************************/
/*  This file is part of the uVision/ARM development tools                    */
/*  Copyright KEIL ELEKTRONIK GmbH 2002-2004                                  */
/******************************************************************************/
/*                                                                            */
/*  SERIAL.C:  Low Level Serial Routines                                      */
/*  modified and extended by Martin Thomas                                    */
/*                                                                            */
/******************************************************************************/

#include "LPC214x.h"
#include "target.h"
#include "serial.h"

#define CR     0x0D

/* Initialize Serial Interface UART0 */
void init_serial0 ( unsigned long baudrate )
{
    unsigned long Fdiv;

    PINSEL0 = 0x00000005;                  /* Enable RxD0 and TxD0              */
    U0LCR = 0x83;                          /* 8 bits, no Parity, 1 Stop bit     */
    Fdiv = ( Fcclk / 16 ) / baudrate ;     /* baud rate                        */
    U0DLM = Fdiv / 256;
    U0DLL = Fdiv % 256;
    U0LCR = 0x03;                           /* DLAB = 0                         */
}

/* Initialize Serial Interface UART0 */
void init_serial1 ( unsigned long baudrate )
{
    unsigned long Fdiv;

    PINSEL0 |= (1<<16) | (1<<18);         /* Enable RxD1 and TxD1              */
    U1LCR = 0x83;                          /* 8 bits, no Parity, 1 Stop bit     */
    Fdiv = ( Fcclk / 16 ) / baudrate ;     /* baud rate                        */
    U1DLM = Fdiv / 256;
    U1DLL = Fdiv % 256;
    U1LCR = 0x03;                           /* DLAB = 0                         */
}

/* Write character to Serial Port 0 with \n -> \r\n  */
int putchar_serial0 (int ch)
{
    if (ch == '\n')
    {
        while (!(U0LSR & 0x20));
        U0THR = CR;                  /* output CR */
    }
    while (!(U0LSR & 0x20));
    return (U0THR = ch);
}

/* Write character to Serial Port 0 without \n -> \r\n  */
int putc_serial0 (int ch)
{
    while (!(U0LSR & 0x20));
    return (U0THR = ch);
}

/* Write character to Serial Port 1 without \n -> \r\n  */
int putc_serial1 (int ch)
{
    while (!(U1LSR & 0x20));
    return (U1THR = ch);
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
	if (U0LSR & 0x01)
    {
        return (U0RBR);
    }
    else
    {
        return 0;
    }
}

/* Read character from Serial Port   */
int getc0 (void)
{
	while ( (U0LSR & 0x01) == 0 ); //Wait for character
	return U0RBR;
}
