///////////////////////////////////////////////////////////////////////////////
// Wavetable oscillator
///////////////////////////////////////////////////////////////////////////////

#include "wtosc.h"
#include "dacspi.h"
#include "osc_curves.h"

#define VIRTUAL_CLOCK 1800000000
#define VIRTUAL_DAC_TICK_RATE (DACSPI_TICK_RATE*15)

#define MAX_SAMPLERATE(oversampling) (VIRTUAL_CLOCK/(VIRTUAL_DAC_TICK_RATE*(oversampling)))

#define COSINESHAPE_LENGTH 256

static uint8_t cosineShape[COSINESHAPE_LENGTH];

static FORCEINLINE uint32_t cvToFrequency(uint16_t cv) // returns the frequency shifted by 8
{
	uint32_t v;
	
	v=cv%(12*WTOSC_CV_SEMITONE); // offset in the octave
	v=(v*20)<<8; // phase for computeShape
	v=(uint32_t)computeShape(v,oscOctaveCurve,1)+32768; // octave frequency in the 12th octave
	v=(v<<8)>>(12-(cv/(12*WTOSC_CV_SEMITONE))); // full frequency shifted by 8
	
	return v;
}

static void globalPreinit(void)
{
	static int8_t done=0;
	
	if(done) return;
	
	// for cosine interpolation
	
	for(int i=0;i<COSINESHAPE_LENGTH;++i)
		cosineShape[i]=(1.0f-cos(M_PI*i/(double)COSINESHAPE_LENGTH))*(255.0/2.0);
	
	done=1;	
}

void wtosc_init(struct wtosc_s * o, int8_t voice, int8_t ab)
{
	globalPreinit();

	memset(o,0,sizeof(struct wtosc_s));

	o->voice=voice;
	o->ab=ab;
	
	wtosc_setSampleData(o,NULL,256);
	wtosc_setParameters(o,69*WTOSC_CV_SEMITONE,0);
}

void wtosc_setSampleData(struct wtosc_s * o, int16_t * data, uint16_t sampleCount)
{
	double f,dummy,sampleRate;
	int i,underSample;

	memset(o->data,0,WTOSC_MAX_SAMPLES*sizeof(uint16_t));

	if(sampleCount>WTOSC_MAX_SAMPLES)
		return;

	if(data)
		for(i=0;i<sampleCount;++i)
			o->data[i]=(int32_t)data[i]-INT16_MIN;

	o->sampleCount=sampleCount;
	
	// recompute undersamples
	
	for(i=0;i<=WTOSC_HIGHEST_NOTE;++i)
	{
		sampleRate=cvToFrequency(i*WTOSC_CV_SEMITONE)/256.0*o->sampleCount;
		underSample=0;

		do
		{
			do
			{
				++underSample;
				f=modf(((double)o->sampleCount)/underSample,&dummy);
			}
			while(fabs(f));
		}
		while(MAX_SAMPLERATE(underSample>2?2:1)<(((double)sampleRate)/underSample));
		
		o->undersample[i]=underSample;
	}
}

void wtosc_setParameters(struct wtosc_s * o, uint16_t cv, uint16_t aliasing)
{
	uint32_t sampleRate;
	uint8_t underSample;
	
	aliasing>>=7;
	cv=MIN(WTOSC_HIGHEST_NOTE*WTOSC_CV_SEMITONE,cv);
	
	if(cv==o->cv && aliasing==o->aliasing)
		return;	
	
	sampleRate=cvToFrequency(cv)*o->sampleCount;
	underSample=o->undersample[cv/WTOSC_CV_SEMITONE];

	o->increment=underSample+aliasing;
	o->period=(VIRTUAL_CLOCK/((sampleRate/o->increment)>>8));	
	o->cv=cv;
	o->aliasing=aliasing;
		
//	rprintf(0,"inc %d cv %x per %d rate %d\n",o->increment,o->cv,o->period,(int)sampleRate/(o->increment<<8));
}

FORCEINLINE void wtosc_update(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer)
{
	int32_t buf,counter,period,phase,increment,sampleCount;
	uint32_t r;
	uint16_t prevSample,curSample,aliasing,controlData;
	uint8_t alpha,voice,ab;
	
	counter=o->counter;
	period=o->period;
	phase=o->phase;
	increment=o->increment;
	sampleCount=o->sampleCount;
	prevSample=o->prevSample;
	curSample=o->curSample;

	aliasing=o->aliasing;	
	voice=o->voice;
	ab=o->ab;
	controlData=ab?DACSPI_CMD_SET_B:DACSPI_CMD_SET_A;
	
	for(buf=startBuffer;buf<=endBuffer;++buf)
	{
		counter-=VIRTUAL_DAC_TICK_RATE;
		if(counter<0)
		{
			counter+=period;

			phase-=increment;
			if(phase<0)
				phase+=sampleCount;

			prevSample=curSample;
			curSample=o->data[phase];
		}

		if(aliasing)
		{
			// we want aliasing, don't interpolate !

			r=curSample;
		}
		else
		{
			// prepare cosine interpolation

			alpha=(counter<<8)/period;
			alpha=cosineShape[alpha];

			// apply it

			r=(alpha*prevSample+(UINT8_MAX-alpha)*curSample)>>8;
		}

		dacspi_setVoiceCommand(buf,voice,ab,(r>>4)|controlData);
	}
	
	o->counter=counter;
	o->phase=phase;
	o->prevSample=prevSample;
	o->curSample=curSample;
}
