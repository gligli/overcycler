#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

#define CLOCK_MAX_BPM 600

typedef enum
{
	symInternal=0,symMIDI=1,symUSB=2,
			
	// /!\ this must stay last
	symCount
} syncMode_t;

void clock_updateSpeed(void);
uint16_t clock_getSpeed(void); // returns 0 if clock is stalled
uint16_t clock_getCounter(void);
void clock_reset(void);
void clock_extClockTick(void);

void clock_init(void);
void clock_update(void);

#endif /* CLOCK_H */
