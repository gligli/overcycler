////////////////////////////////////////////////////////////////////////////////
// Common clock for sequencer and arpeggiator
////////////////////////////////////////////////////////////////////////////////

#include "clock.h"

#include "scan.h"
#include "storage.h"

#define FRAC_SHIFT 16

struct
{
	int32_t counter,speed;
	uint32_t pendingExtClock;
	int8_t pendingReset;
} clock;

void clock_updateSpeed(void)
{
	if(!settings.seqArpClock)
		clock.speed=INT32_MAX;
	else if(settings.syncMode==symInternal)
		clock.speed=((60UL*TICKER_HZ)<<FRAC_SHIFT)/settings.seqArpClock;
	else
		clock.speed=extClockDividers[((uint32_t)settings.seqArpClock*(sizeof(extClockDividers)/sizeof(uint16_t)-1))/(CLOCK_MAX_BPM-1)]<<FRAC_SHIFT;
}

inline uint16_t clock_getSpeed(void)
{
	if(clock.speed==INT32_MAX)
		return 0;
	else
		return clock.speed>>FRAC_SHIFT;
}

inline uint16_t clock_getCounter(void)
{
	return clock.counter>>FRAC_SHIFT;
}

inline void clock_reset(void)
{
	clock.pendingExtClock=0;
	clock.pendingReset=1;
}

inline void clock_extClockTick(void)
{
	++clock.pendingExtClock;
}

void clock_init(void)
{
	memset(&clock,0,sizeof(clock));
	clock.speed=INT32_MAX;
}

void clock_update(void)
{
	if(clock.speed!=INT32_MAX)
	{
		if(clock.pendingReset)
		{
			clock.counter=0;
			clock.pendingReset=0;
			synth_clockEvent();
		}

		if(settings.syncMode==symInternal)
		{
			clock.counter+=1<<FRAC_SHIFT;
		}
		else
		{
			clock.counter+=clock.pendingExtClock<<FRAC_SHIFT;
		}

		if(clock.counter>=clock.speed)
		{
			clock.counter-=clock.speed;
			synth_clockEvent();
		}
	}

	clock.pendingExtClock=0;
}
