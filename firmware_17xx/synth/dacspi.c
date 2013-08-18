///////////////////////////////////////////////////////////////////////////////
// 12bit voice DACs communication through SPI
///////////////////////////////////////////////////////////////////////////////

#include "dacspi.h"

struct
{
	int8_t currentChannel;
	uint16_t prevCommands[DACSPI_CHANNEL_COUNT][2];
} dacspi;


void dacspi_init(void)
{
	memset(&dacspi,0,sizeof(dacspi));
	
	// SSP

	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_SSP0,CLKPWR_PCLKSEL_CCLK_DIV_1);
	CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCSSP0,ENABLE);

	LPC_SSP0->CPSR=2;
	LPC_SSP0->CR0=0x020f;
	LPC_SSP0->CR1=2;
	
	// SSP pins

	PINSEL_SetPinFunc(1,20,3);
	PINSEL_SetPinFunc(1,21,3);
	PINSEL_SetPinFunc(1,24,3);
	
	// /LDAC pins (6 voices + CV dac)
	for(int i=0;i<7;++i)
	{
		PINSEL_SetPinFunc(2,i,0);
		GPIO_SetDir(2,1<<i,1);
		GPIO_SetValue(2,1<<i);
	}
	
	SSP_Cmd(LPC_SSP0,ENABLE);
}

inline void dacspi_sendCommand(uint8_t channel, uint16_t command)
{
	uint8_t mask;
	
	if(channel!=DACSPI_CV_CHANNEL)
	{
		dacspi.prevCommands[channel][command>>15]=command;
		LPC_SSP0->DR=dacspi.prevCommands[channel][0];
		LPC_SSP0->DR=dacspi.prevCommands[channel][1];
	}
	else
	{
		LPC_SSP0->DR=command;
	}
	
	while(LPC_SSP0->SR&SSP_SR_BSY)
		__NOP();
	
	mask=1<<channel;
	LPC_GPIO2->FIOCLR0=mask;
	DELAY_100NS();
	LPC_GPIO2->FIOSET0=mask;
}
