#ifndef MAIN_H
#define MAIN_H

#include <stddef.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include <core_cm3.h>
#include <serial.h>
#include <rprintf.h>

void delay_ms(uint32_t);
void delay_us(uint32_t);

unsigned long disableInts(void);
unsigned long enableInts(void);
unsigned long restoreInts(unsigned long oldCPSR);

#define BLOCK_INT for(int __int_block=disableInts();__int_block;restoreInts(__int_block),__int_block=0)

#endif
