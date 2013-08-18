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

// assumes 100Mhz clock
#define DELAY_100NS() asm volatile ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop")

void delay_us(uint32_t);
void delay_ms(uint32_t);

static inline int __disable_irq_stub(void)
{
	__disable_irq();
	return 1;
}

static inline int __enable_irq_stub(void)
{
	__enable_irq();
	return 0;
}

#define BLOCK_INT for(int __int_block=__disable_irq_stub();__int_block;__int_block=__enable_irq_stub())

#endif
