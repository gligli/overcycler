///////////////////////////////////////////////////////////////////////////////
// Wavetable oscillator
///////////////////////////////////////////////////////////////////////////////

#include "wtosc.h"
#include "dacspi.h"
#include "PowFast.h"

#define OVERSAMPLING_RATIO 2 // higher -> less aliasing but lower effective sample rate
#define MAX_SAMPLERATE ((double)SYNTH_MASTER_CLOCK/(DACSPI_TICK_RATE*OVERSAMPLING_RATIO))

#define COSINESHAPE_LENGTH 256

static uint16_t cosineShape[COSINESHAPE_LENGTH];
static const double c2fMul=1.0/(12.0*WTOSC_CV_SEMITONE);
static const double c2fAdd=-69.0/12.0;
static int powFastTable[256];

static FORCEINLINE float cvToFrequency(uint16_t cv)
{
	return powFast2_simple(powFastTable,8,c2fMul*cv+c2fAdd)*440.0f;
}

static void globalPreinit(void)
{
	static int8_t done=0;
	
	if(done) return;
	
	// for cosine interpolation
	
	for(int i=0;i<COSINESHAPE_LENGTH;++i)
		cosineShape[i]=(1.0f-cos(M_PI*i/(double)COSINESHAPE_LENGTH))*(65535.0/2.0);
	
	powFastSetTable(powFastTable,8);

	done=1;	
}

void wtosc_init(struct wtosc_s * o, uint16_t controlData)
{
	globalPreinit();

	memset(o,0,sizeof(struct wtosc_s));

	o->controlData=controlData;
	
	wtosc_setSampleData(o,NULL,256);
	wtosc_setParameters(o,69*WTOSC_CV_SEMITONE,0);
}

void wtosc_setSampleData(struct wtosc_s * o, int16_t * data, uint16_t sampleCount)
{
	double f,dummy;
	int i,underSample,sampleRate;

	memset(o->data,0,WTOSC_MAX_SAMPLES*sizeof(uint16_t));

	if(sampleCount>WTOSC_MAX_SAMPLES)
		return;

	if(data)
		for(i=0;i<sampleCount;++i)
			o->data[i]=(int32_t)data[i]-INT16_MIN;

	o->sampleCount=sampleCount;
	
	// recompute undersamples
	
	for(i=WTOSC_LOWEST_NOTE;i<=WTOSC_HIGHEST_NOTE;++i)
	{
		sampleRate=cvToFrequency(i*WTOSC_CV_SEMITONE)*o->sampleCount;
		underSample=0;

		do
		{
			do
			{
				++underSample;
				f=modf(((double)o->sampleCount)/underSample,&dummy);
			}
			while(fabs(f)>0.0001);
		}
		while(MAX_SAMPLERATE<(((double)sampleRate)/underSample));
		
		o->undersample[i-WTOSC_LOWEST_NOTE]=underSample;
	}
}

void wtosc_setParameters(struct wtosc_s * o, uint16_t cv, uint16_t aliasing)
{
	uint32_t underSample,sampleRate;
	
	aliasing>>=7;
	cv=MAX(WTOSC_LOWEST_NOTE*WTOSC_CV_SEMITONE,cv);
	cv=MIN(WTOSC_HIGHEST_NOTE*WTOSC_CV_SEMITONE,cv);
	
	if(cv==o->cv && aliasing==o->aliasing)
		return;	
	
	sampleRate=cvToFrequency(cv)*o->sampleCount;
	underSample=o->undersample[(cv/WTOSC_CV_SEMITONE)-WTOSC_LOWEST_NOTE];

	o->increment=underSample+aliasing;
	o->period=(uint64_t)SYNTH_MASTER_CLOCK*o->increment/sampleRate;	
	o->cv=cv;
	o->aliasing=aliasing;
		
//	rprintf(0,"inc %d cv %x per %d rate %d\n",o->increment,o->cv,o->period,(int)sampleRate/underSample);
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

	// prepare cosine interpolation

	alpha=(o->counter<<8)/o->period;
	alpha=cosineShape[(uint8_t)alpha];
	
	// apply it
	
	r=(alpha*o->prevSample+(UINT16_MAX-alpha)*o->curSample)>>20;
	
	return r|o->controlData;
}
