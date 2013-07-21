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

// calls system_init() to set clock, sets up interrupts, sets up timer, checks voltage and 
// powers down if below threshold, then enables regulator for LCD and GPS
void boot_up(void);

/* RESET the processor */
void reset(void);
