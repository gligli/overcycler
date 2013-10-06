///////////////////////////////////////////////////////////////////////////////
// Wavetable oscillator
///////////////////////////////////////////////////////////////////////////////

#include "wtosc.h"
#include "dacspi.h"

#define OVERSAMPLING_RATIO 2 // higher -> less aliasing but lower effective sample rate
#define MAX_SAMPLERATE ((double)SYNTH_MASTER_CLOCK/DACSPI_TICK_RATE/OVERSAMPLING_RATIO)

#define COSINESHAPE_LENGTH 256

static uint16_t cosineShape[COSINESHAPE_LENGTH];

void wtosc_init(struct wtosc_s * o, uint16_t controlData)
{
	memset(o,0,sizeof(struct wtosc_s));
	o->controlData=controlData;
	o->sampleCount=WTOSC_MAX_SAMPLES;

	for(int i=0;i<COSINESHAPE_LENGTH;++i)
		cosineShape[i]=(1.0f-cos(M_PI*i/(double)COSINESHAPE_LENGTH))*(65535.0/2.0);
	
	wtosc_setParameters(o,69*WTOSC_CV_SEMITONE,0);
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
	
	aliasing>>=7;
	
	if(cv==o->cv && aliasing==o->aliasing)
		return;	
	
	freq=pow(2.0,(((double)cv/WTOSC_CV_SEMITONE)-69.0)/12.0)*440.0; // note frequency
	rate=freq*o->sampleCount; // sample rate

	do
	{
		do
		{
			++underSample;
			f=modf(((double)o->sampleCount)/underSample,&dummy);
		}
		while(fabs(f)>0.001);
	}
	while(MAX_SAMPLERATE<(((double)rate)/underSample));

	BLOCK_INT
	{
		o->increment=underSample+aliasing;
		o->period=(uint64_t)SYNTH_MASTER_CLOCK*o->increment/rate;	
		o->cv=cv;
		o->aliasing=aliasing;
	}
		
//	rprintf(0,"inc %d cv %x per %d rate %d\n",o->increment,o->cv,o->period,(int)rate/underSample);
}

FORCEINLINE uint16_t wtosc_update(struct wtosc_s * o, int32_t elapsedTicks)
{
	uint32_t r;
	uint32_t alpha;
	

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

#if 1
	// prepare cosine interpolation

	alpha=(o->counter<<8)/o->period;
	alpha=cosineShape[(uint8_t)alpha];
#else
	// prepare linear interpolation
	
	alpha=(o->counter<<16)/o->period;
#endif
	
	r=(alpha*o->prevSample+(UINT16_MAX-alpha)*o->curSample)>>16;
	
	return (r>>4)|o->controlData;
}
