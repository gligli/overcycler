///////////////////////////////////////////////////////////////////////////////
// Wavetable oscillator
///////////////////////////////////////////////////////////////////////////////

#include "wtosc.h"
#include "dacspi.h"

void wtosc_init(struct wtosc_s * o, int8_t channel, uint16_t controlData)
{
	memset(o,0,sizeof(struct wtosc_s));
	o->period=WTOSC_MAX_SAMPLES;
	o->controlData=controlData;
	o->channel=channel;
	o->increment=1;
	o->sampleCount=WTOSC_MAX_SAMPLES;
}

void wtosc_setSampleData(struct wtosc_s * o, int16_t * data, uint16_t sampleCount)
{
	BLOCK_INT
	{
		memset(o->data,0,WTOSC_MAX_SAMPLES*sizeof(uint16_t));

		if(sampleCount>WTOSC_MAX_SAMPLES)
			return;

		int i;
		for(i=0;i<sampleCount;++i)
			o->data[i]=(((int32_t)data[i]-INT16_MIN)>>4)|o->controlData;

		o->sampleCount=sampleCount;
	}
}

void wtosc_setParameters(struct wtosc_s * o, uint16_t cv, uint16_t aliasing)
{
	double freq,note,rate,f,dummy;
	int underSample=0;
	
	note=cv/256.0; // midi note
	freq=pow(2.0,(note-69.0)/12.0)*440.0; // note frequency
	rate=freq*o->sampleCount; // sample rate

	while((rate/underSample)>(double)WTOSC_MAX_SAMPLE_RATE)
	{
		do
		{
			++underSample;
			f=modf((double)o->sampleCount/underSample,&dummy);
		}
		while(fabs(f)>0.0001);
	}
		
	BLOCK_INT
	{
		o->increment=underSample+aliasing;
		o->period=(double)SYNTH_MASTER_CLOCK*o->increment/rate;	
		o->cv=cv;
	}
		
	rprintf("inc %d cv %x per %d rate %d\n",o->increment,o->cv,o->period,(int)rate/underSample);
}

inline uint32_t wtosc_update(struct wtosc_s * o)
{
	o->phase-=o->increment;
	if(o->phase<0)
		o->phase+=o->sampleCount;
	
	dacspi_sendCommand(o->channel,o->data[o->phase]);

	return o->period;
}
