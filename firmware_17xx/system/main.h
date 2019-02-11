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

//AVR compatibility
#define PROGMEM
#define pgm_read_byte(addr) (*(uint8_t*)(addr))
#define random rand
#define srandom srand
#define print(s) rprintf(0,s)
#define phex(x) rprintf(0,"%x",x)
#define phex16(x) phex(x)

// assumes 120Mhz clock
#define DELAY_50NS() asm volatile ("nop\nnop\nnop\nnop\nnop\nnop")
#define DELAY_100NS() {DELAY_50NS();DELAY_50NS();}

void delay_us(uint32_t);
void delay_ms(uint32_t);

static inline uint32_t __disable_irq_stub(uint32_t basepri)
{
	uint32_t pm=__get_BASEPRI();
	__set_BASEPRI(basepri << (8 - __NVIC_PRIO_BITS));
	return pm;
}

static inline void __restore_irq_stub(uint32_t basepri)
{
	__set_BASEPRI(basepri);
}

#define BLOCK_INT(basepri) for(uint32_t __ctr=1,__int_block=__disable_irq_stub(basepri);__ctr;__restore_irq_stub(__int_block),__ctr=0)


#define STORAGE_PAGE_SIZE 256
#define STORAGE_SIZE (65536*256)

#define EXT_RAM  __attribute__((section(".ext_ram")))

extern void storage_write(uint32_t pageIdx, uint8_t *buf);
extern void storage_read(uint32_t pageIdx, uint8_t *buf);

#endif
