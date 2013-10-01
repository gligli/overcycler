#ifndef MAIN_H
#define MAIN_H

#define DEBUG 1

#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include <core_cm3.h>
#include <core_cmFunc.h>
#include <core_cmInstr.h>
#include <serial.h>
#include <rprintf.h>

#include <lpc17xx_clkpwr.h>
#include <lpc17xx_pinsel.h>
#include <lpc17xx_gpio.h>
#include <lpc17xx_timer.h>
#include <lpc17xx_ssp.h>
#include <lpc17xx_adc.h>
#include <lpc17xx_uart.h>
#include <lpc17xx_gpdma.h>
#include <lpc17xx_spi.h>

// assumes 100Mhz clock
#define DELAY_100NS() asm volatile ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop")

void delay_us(uint32_t);
void delay_ms(uint32_t);

static inline uint32_t __disable_irq_stub(void)
{
	uint32_t pm=__get_PRIMASK();
	__set_PRIMASK(1);
	return pm;
}

static inline int __restore_irq_stub(uint32_t pm)
{
	__set_PRIMASK(pm);
	return 2;
}

#define BLOCK_INT for(uint32_t __int_block=__disable_irq_stub();__int_block<2;__int_block=__restore_irq_stub(__int_block))

#endif
