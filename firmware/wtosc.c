///////////////////////////////////////////////////////////////////////////////
// Wavetable oscillator
///////////////////////////////////////////////////////////////////////////////

#include "wtosc.h"
#include "dacspi.h"

void wtosc_init(struct wtosc_s * o, int channel, uint16_t controlData)
{
	memset(o,0,sizeof(o));

	o->controlData=controlData;
	o->channel=channel;

	dacspi_setState(o->channel,DSIDX_WAVEFORM_ADDR,(uint32_t)o->resmpData);
}

void wtosc_setSampleData(struct wtosc_s * o, int16_t * data, uint16_t sampleCount)
{
	BLOCK_INT
	{
		memset(o->rawData,0,WTOSC_MAX_SAMPLES*sizeof(uint16_t));

		if(sampleCount>WTOSC_MAX_SAMPLES)
			return;

		int i;
		for(i=0;i<sampleCount;++i)
			o->rawData[i]=data[i];

		o->rawSampleCount=sampleCount;
	}
	
	if(o->increment)
		wtosc_setParameters(o,o->cv,o->increment-1);
}

// resamples pSamples (taken from http://pcsx2.googlecode.com/svn/trunk/plugins/zerospu2/zerospu2.cpp)
static void resampleLinear(int16_t* pSamples, int32_t oldsamples, int16_t* pNewSamples, int32_t newsamples)
{
	int32_t newsamp;
	int32_t i;

	for (i = 0; i < newsamples; ++i)
	{
		int32_t io = i * oldsamples;
		int32_t old = io / newsamples;
		int32_t rem = io - old * newsamples;

		if (old==0){
			newsamp = pSamples[oldsamples-1] * (newsamples - rem) + pSamples[0] * rem;
		}else{
			newsamp = pSamples[old-1] * (newsamples - rem) + pSamples[old] * rem;
		}
		
		pNewSamples[i] = newsamp / newsamples;
	}
}

static void makeCommands(uint16_t * data, int count, uint16_t controlData)
{
	int i;
	uint16_t d;
	
	for(i=0;i<count;++i)
	{
		d=*data;
		d=(int32_t)d-INT16_MIN;
		d=(d>>4)|controlData;
		*data++=d;
	}
}

void wtosc_setParameters(struct wtosc_s * o, uint16_t cv, uint16_t aliasing)
{
	double freq,note,rate;
	
	note=cv/256.0; // midi note
	freq=pow(2.0,(note-69.0)/12.0)*440.0; // note frequency
	rate=freq*o->rawSampleCount; // sample rate
	
	o->resmpSampleCount=(WTOSC_MASTER_SAMPLE_RATE/rate)*o->rawSampleCount+0.5;
	
	BLOCK_INT
	{
		resampleLinear(o->rawData,o->rawSampleCount,o->resmpData,o->resmpSampleCount);
		
		makeCommands(o->resmpData,o->resmpSampleCount,o->controlData);
		
		o->increment=aliasing+1;
		o->resmpSamplePeriod=(double)CPU_FREQ*o->increment/WTOSC_MASTER_SAMPLE_RATE;	
		o->cv=cv;
		
		rprintf("inc %d cv %x per %d rsc %d\n",o->increment,o->cv,o->resmpSamplePeriod,o->resmpSampleCount);
		
		dacspi_setState(o->channel,DSIDX_SAMPLE_COUNT,o->resmpSampleCount);
		dacspi_setState(o->channel,DSIDX_PERIOD,o->resmpSamplePeriod);
		dacspi_setState(o->channel,DSIDX_INCREMENT,o->increment);
	}

}
