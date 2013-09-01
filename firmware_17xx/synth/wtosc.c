///////////////////////////////////////////////////////////////////////////////
// Wavetable oscillator
///////////////////////////////////////////////////////////////////////////////

#include "wtosc.h"
#include "dacspi.h"

#define MAX_SAMPLERATE ((double)SYNTH_MASTER_CLOCK/DACSPI_TICK_RATE)

void wtosc_init(struct wtosc_s * o, uint16_t controlData)
{
	memset(o,0,sizeof(struct wtosc_s));
	o->period=WTOSC_MAX_SAMPLES;
	o->controlData=controlData;
	o->increment=1;
	o->sampleCount=WTOSC_MAX_SAMPLES;
}

void wtosc_setSampleData(struct wtosc_s * o, int16_t * data, uint16_t sampleCount)
{
	memset(o->data,0,WTOSC_MAX_SAMPLES*sizeof(uint16_t));

	if(sampleCount>WTOSC_MAX_SAMPLES)
		return;

	int i;
	for(i=0;i<sampleCount;++i)
		o->data[i]=(int32_t)data[i]-INT16_MIN;

	o->sampleCount=sampleCount;
}

void wtosc_setParameters(struct wtosc_s * o, uint16_t cv, uint16_t aliasing)
{
	double freq,f,dummy;
	int underSample=0,rate;
	
	freq=pow(2.0,(((double)cv/WTOSC_CV_SEMITONE)-69.0)/12.0)*440.0; // note frequency
	rate=freq*o->sampleCount; // sample rate

	do
	{
		do
		{
			++underSample;
			f=modf(((double)o->sampleCount)/underSample,&dummy);
		}
		while(f);
	}
	while(MAX_SAMPLERATE<(((double)rate)/underSample));

	BLOCK_INT
	{
		o->increment=underSample+aliasing;
		o->period=(uint64_t)SYNTH_MASTER_CLOCK*o->increment/rate;	
		o->cv=cv;
	}
		
//	rprintf(0,"inc %d cv %x per %d rate %d\n",o->increment,o->cv,o->period,(int)rate/underSample);
}

FORCEINLINE uint16_t wtosc_update(struct wtosc_s * o, int32_t elapsedTicks)
{
	int32_t r;

	o->counter-=elapsedTicks;
	if(o->counter<0)
	{
		o->counter+=o->period;
		
		o->phase-=o->increment;
		if(o->phase<0)
			o->phase+=o->sampleCount;

		o->prevSample=o->curSample;
		o->curSample=o->data[o->phase];
	}
		
	r=(o->counter*o->prevSample+(o->period-o->counter)*o->curSample)/o->period;
	
	return (r>>4)|o->controlData;
}
