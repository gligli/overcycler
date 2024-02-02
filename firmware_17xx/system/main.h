#ifndef MAIN_H
#define MAIN_H

//#define DEBUG
//#define DEBUG_

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <LPC177x_8x.h>
#include <system_LPC177x_8x.h>
#include <core_cm3.h>
#include <core_cmFunc.h>
#include <core_cmInstr.h>
#include <serial.h>
#include <rprintf.h>

#include <lpc177x_8x_clkpwr.h>
#include <lpc177x_8x_pinsel.h>
#include <lpc177x_8x_gpio.h>
#include <lpc177x_8x_timer.h>
#include <lpc177x_8x_ssp.h>
#include <lpc177x_8x_adc.h>
#include <lpc177x_8x_uart.h>
#include <lpc177x_8x_gpdma.h>

#include "ff.h"

//AVR compatibility
#define PROGMEM
#define pgm_read_byte(addr) (*(uint8_t*)(addr))
#define print(s) rprintf(0,s)
#define phex(x) rprintf(0,"%x",(int)x)
#define phex16(x) phex(x)

// assumes 120Mhz clock
#define DELAY_50NS() asm volatile ("nop\nnop\nnop\nnop\nnop\nnop")
#define DELAY_100NS() {DELAY_50NS();DELAY_50NS();}

#define MAX_FILENAME (_MAX_LFN + 1)

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


#define STORAGE_PAGE_SIZE 512
#define STORAGE_PAGE_COUNT 65536

#define EXT_RAM  __attribute__((section(".ext_ram")))

typedef enum
{
	umNone=-1,umPowerOnly=0,umMIDI=1,umMSC=2
} usbMode_t;

typedef int8_t (*usb_MSC_continue_callback_t)(void); // return 0 to quit USB MSC mode

extern void usb_setMode(usbMode_t mode, usb_MSC_continue_callback_t usbMSCContinue);

extern void storage_write(uint32_t pageIdx, uint8_t *buf);
extern void storage_read(uint32_t pageIdx, uint8_t *buf);
extern void storage_delete(uint32_t pageIdx);
extern int8_t storage_pageExists(uint32_t pageIdx);
extern int8_t storage_samePage(uint32_t pageIdx, uint32_t pageIdx2);

#endif
