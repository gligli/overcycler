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

	dacspi_setState(o->channel,DSIDX_INCREMENT,1);
	dacspi_setState(o->channel,DSIDX_SAMPLE_COUNT,WTOSC_MAX_SAMPLES);

	dacspi_setState(o->channel,DSIDX_WAVEFORM_ADDR,(uint32_t)o->data);
}

void wtosc_setSampleData(struct wtosc_s * o, int16_t * data, uint16_t sampleCount)
{
	BLOCK_INT
	{
		T0TCR=0;

		memset(o->data,0,WTOSC_MAX_SAMPLES*sizeof(uint16_t));

		if(sampleCount>WTOSC_MAX_SAMPLES)
			return;

		int i;
		for(i=0;i<sampleCount;++i)
			o->data[i]=(((int32_t)data[i]-INT16_MIN)>>4)|o->controlData;

		o->sampleCount=sampleCount;
		dacspi_setState(o->channel,DSIDX_SAMPLE_COUNT,o->sampleCount);
		
		T0TCR=1;
	}
}

void wtosc_setParameters(struct wtosc_s * o, uint16_t cv, uint16_t aliasing)
{
	double freq,note,rate,f;
	int underSample=0;
	
	note=cv/256.0; // midi note
	freq=pow(2.0,(note-69.0)/12.0)*440.0; // note frequency
	rate=freq*o->sampleCount; // sample rate

	while((rate/underSample)>(double)WTOSC_MAX_SAMPLE_RATE)
	{
		do
		{
			++underSample;
			f=modf((double)o->sampleCount/underSample,NULL);
		}
		while(fabs(f)>0.0001);
	}
		
	o->increment=underSample+aliasing;
	o->samplePeriod=(double)SYNTH_MASTER_CLOCK*o->increment/rate;	
	o->cv=cv;
		
	BLOCK_INT
	{
		dacspi_setState(o->channel,DSIDX_PERIOD,o->samplePeriod);
		dacspi_setState(o->channel,DSIDX_INCREMENT,o->increment);
	}

	rprintf("inc %d cv %x per %d rate %d\n",o->increment,o->cv,o->samplePeriod,(int)rate/underSample);
}
