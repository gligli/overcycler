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
#include "lpc177x_8x_pinsel.h"
#include "serial.h"

/* Initialize Serial Interface UART0 */
void init_serial0 ( unsigned long baudrate )
{
	// Initialize UART0 pin connect
	PINSEL_ConfigPin(0, 2, 1);
	PINSEL_ConfigPin(0, 3, 1);
	
	// UART Configuration structure variable
	UART_CFG_Type UARTConfigStruct;
		
	/* Initialize UART Configuration parameter structure to default state:
	* Baudrate = 115200 bps
	* 8 data bit
	* 1 Stop bit
	* None parity
	*/
	UART_ConfigStructInit(&UARTConfigStruct);
	//Configure baud rate
	UARTConfigStruct.Baud_rate = baudrate;
	// Initialize UART0 & UART3 peripheral with given to corresponding parameter
	UART_Init(UART_0, &UARTConfigStruct);

	// Enable UART Transmit
	UART_TxCmd(UART_0, ENABLE);
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

	UART_SendByte(UART_0, ch);

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
