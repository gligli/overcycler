///////////////////////////////////////////////////////////////////////////////
// 12bit DACs communication through SPI
///////////////////////////////////////////////////////////////////////////////

#include "dacspi.h"

extern volatile uint32_t dacspi_states[];
extern volatile uint32_t dacspi_ref;

struct
{
	int8_t currentChannel;
} dacspi;


void dacspi_init(void)
{
	memset(&dacspi,0,sizeof(dacspi));
/*
	// SSP
	SSPCPSR=2; // 1/2 APB clock
	SSPCR0=0x0f | (0<<8); // 16bit; spi; mode 0,0; 1x multiplier
	SSPCR1=0x02; // normal; enable; master
	
	// SSP pins
	PINSEL1&=~0x03cc;
	PINSEL1|=0x0288;
	
	// /LDAC pins
	PINSEL0&=~0xff300000;
	FIO0DIR|=0xf400;
 */
/*	
	// SSEL pins
	PINSEL0&=~0xff300000;
	FIO0DIR|=0xf400;
	FIO0SET=0xf400;
	
	// SDI / SCK
	
	PINSEL2=0;
	FIO1DIR|=0x60000000;
	FIO1CLR=0x60000000;*/
}

void dacspi_setState(int channel, int dsidx, uint32_t value)
{
//	dacspi_states[channel*8+dsidx]=value;
}

void dacspi_setReference(uint16_t value)
{
//	dacspi_ref=0x7000|(value>>4);
}
