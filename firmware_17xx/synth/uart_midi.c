///////////////////////////////////////////////////////////////////////////////
// LCD / keypad / potentiometers user interface
///////////////////////////////////////////////////////////////////////////////

#include "uart_midi.h"

__attribute__ ((used)) void UART2_IRQHandler(void)
{
	synth_uartEvent(UART_ReceiveData(LPC_UART2));
}

void uartMidi_init(void)
{
	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_UART2,CLKPWR_PCLKSEL_CCLK_DIV_1);	
	PINSEL_SetPinFunc(0,11,1);
	
	UART_CFG_Type uart;
	UART_ConfigStructInit(&uart);
	uart.Baud_rate=31250;
	UART_Init(LPC_UART2,&uart);

	UART_FIFO_CFG_Type fifo;
	UART_FIFOConfigStructInit(&fifo);
	UART_FIFOConfig((LPC_UART_TypeDef*)LPC_UART2,&fifo);
	
	UART_IntConfig(LPC_UART2,UART_INTCFG_RBR,ENABLE);

	NVIC_SetPriority(UART2_IRQn,1);
	NVIC_EnableIRQ(UART2_IRQn);
}
