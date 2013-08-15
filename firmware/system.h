#ifndef __SYSTEM_H
#define __SYSTEM_H

/******************************************************************************/
/*  						                                                  */
/*  Copyright Spark Fun Electronics                                           */
/******************************************************************************/
/*                                                                            */
/* Modified system.h for minimal bootloading                                                                            */
/*                                                                            */
/******************************************************************************/



// these are for setting up the LPC clock for 4x PLL
void system_init(void);
void feed(void);

// general purpose
void delay_ms(int);
void delay_us(int);

// calls system_init() to set clock, sets up interrupts, sets up timer, checks voltage and 
// powers down if below threshold, then enables regulator for LCD and GPS
void boot_up(void);

/* RESET the processor */
void reset(void);

unsigned long disableInts(void);
unsigned long enableInts(void);
unsigned long restoreInts(unsigned long oldCPSR);

#define BLOCK_INT for(int __int_block=disableInts();__int_block;restoreInts(__int_block),__int_block=0)

#endif //__SYSTEM_H
