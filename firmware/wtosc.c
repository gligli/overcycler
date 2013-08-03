///////////////////////////////////////////////////////////////////////////////
// Wavetable oscillator
///////////////////////////////////////////////////////////////////////////////

#include "wtosc.h"
#include "dacspi.h"

void wtosc_init(struct wtosc_s * o, int channel, uint16_t controlData)
{
	memset(o,0,sizeof(o));
	o->samplePeriod=WTOSC_MAX_SAMPLES;
	o->controlData=controlData;
	o->channel=channel;

	dacspi_setState(o->channel,DSIDX_PERIOD,1000);
	dacspi_setState(o->channel,DSIDX_INCREMENT,1);
	dacspi_setState(o->channel,DSIDX_SAMPLE_COUNT,WTOSC_MAX_SAMPLES);

	dacspi_setState(o->channel,DSIDX_WAVEFORM_ADDR,(uint32_t)o->data);
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
		dacspi_setState(o->channel,DSIDX_SAMPLE_COUNT,o->sampleCount);
	}
}

void wtosc_setParameters(struct wtosc_s * o, uint16_t cv, uint16_t increment)
{
	double freq,note,rate;
	int underSample=1;
	
	note=cv/256.0; // midi note
	freq=pow(2.0,(note-69.0)/12.0)*440.0; // note frequency
	rate=freq*o->sampleCount; // sample rate

	while((rate/underSample)>(double)WTOSC_MASTER_SAMPLE_RATE)
		++underSample;
		
	increment+=underSample;
	
	BLOCK_INT
	{
		o->samplePeriod=(double)CPU_FREQ*increment/rate;	
		o->increment=increment;
		o->cv=cv;
		
		dacspi_setState(o->channel,DSIDX_PERIOD,o->samplePeriod);
		dacspi_setState(o->channel,DSIDX_INCREMENT,o->increment);
		dacspi_setState(o->channel,DSIDX_INTERP_RATIO,(o->increment*((uint64_t)1<<32))/o->samplePeriod);
	}

	rprintf("inc %d cv %x per %d rate %d\n",o->increment,o->cv,o->samplePeriod,(int)rate/underSample);
}
