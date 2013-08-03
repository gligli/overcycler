///////////////////////////////////////////////////////////////////////////////
// 12bit DACs communication through SPI
///////////////////////////////////////////////////////////////////////////////

#include "dacspi.h"

extern volatile uint32_t dacspi_states[];
extern volatile uint32_t dacspi_master_period;

struct
{
	int8_t currentChannel;
} dacspi;


void dacspi_init(void)
{
	memset(&dacspi,0,sizeof(dacspi));

	// SSP
	SSPCPSR=2; // 1/2 APB clock
	SSPCR0=0x0f | (1<<8); // 16bit; spi; mode 0,0;
	SSPCR1=0x02; // normal; enable; master
	
	// pins
	PINSEL1&=~0x03cc;
	PINSEL1|=0x0288;
}

void dacspi_setState(int channel, int dsidx, uint32_t value)
{
	dacspi_states[channel*8+dsidx]=value;
}

void dacspi_setMasterPeriod(uint32_t period)
{
	dacspi_master_period=period;
}