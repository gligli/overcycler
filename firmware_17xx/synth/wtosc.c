///////////////////////////////////////////////////////////////////////////////
// Wavetable oscillator
///////////////////////////////////////////////////////////////////////////////

#include "wtosc.h"
#include "dacspi.h"
#include "osc_curves.h"

#define VIRTUAL_CLOCK 3600000000UL
#define VIRTUAL_DAC_TICK_RATE (DACSPI_TICK_RATE*(VIRTUAL_CLOCK/SYNTH_MASTER_CLOCK))

#define MAX_SAMPLERATE(oversampling) (VIRTUAL_CLOCK/((VIRTUAL_DAC_TICK_RATE*(oversampling))/16)) /* oversampling in 1/16th */

#define USE_HERMITE_INTERP

#define FRAC_SHIFT 12

static FORCEINLINE uint32_t cvToFrequency(uint32_t cv) // returns the frequency shifted by 8
{
	uint32_t v;
	
	v=cv%(12*WTOSC_CV_SEMITONE); // offset in the octave
	v=(v*21)<<8; // phase for computeShape
	v=(uint32_t)computeShape(v,oscOctaveCurve,1)+32768; // octave frequency in the 12th octave
	v=(v<<12)>>(12-(cv/(12*WTOSC_CV_SEMITONE))); // full frequency shifted by 12
	
	return v;
}

void wtosc_init(struct wtosc_s * o, int32_t channel)
{
	memset(o,0,sizeof(struct wtosc_s));

	o->channel=channel;
	
	wtosc_setSampleData(o,NULL,-1);
	wtosc_setParameters(o,69*WTOSC_CV_SEMITONE,0,HALF_RANGE);
}

void wtosc_setSampleData(struct wtosc_s * o, uint16_t * data, uint16_t sampleCount)
{
	o->data=NULL;
	o->sampleCount=0;
	o->halfSampleCount=0;

	if(sampleCount>WTOSC_MAX_SAMPLES)
		return;

	o->data=data;
	o->sampleCount=sampleCount;
	o->halfSampleCount=sampleCount>>1;
}

void wtosc_setParameters(struct wtosc_s * o, uint16_t cv, uint16_t aliasing, uint16_t width)
{
	uint32_t sampleRate[2], maxSampleRate, frequency;
	int32_t increment[2], period[2];
	
	if(!o->data)
		return;
	
	aliasing>>=7;
	
	width=MAX(UINT16_MAX/16,width);
	width=MIN((15*UINT16_MAX)/16,width);
	width>>=4;

	cv=MIN(WTOSC_HIGHEST_NOTE*WTOSC_CV_SEMITONE,cv);
	
	if(cv==o->cv && aliasing==o->aliasing && width==o->width)
		return;	
	
	maxSampleRate=MAX_SAMPLERATE(16);
	frequency=cvToFrequency(cv);

	sampleRate[0]=frequency/(4096-width);
	sampleRate[1]=frequency/width;
	
	sampleRate[0]=sampleRate[0]*o->halfSampleCount;
	sampleRate[1]=sampleRate[1]*o->halfSampleCount;

	increment[0]=1+(sampleRate[0]/maxSampleRate);
	increment[1]=1+(sampleRate[1]/maxSampleRate);

	while(o->halfSampleCount%increment[0]) ++increment[0];
	while(o->halfSampleCount%increment[1]) ++increment[1];
	
	increment[0]+=aliasing;
	increment[1]+=aliasing;
	period[0]=VIRTUAL_CLOCK/(sampleRate[0]/increment[0]);
	period[1]=VIRTUAL_CLOCK/(sampleRate[1]/increment[1]);	
	
	o->pendingPeriod[0]=period[0];	
	o->pendingPeriod[1]=period[1];	
	o->pendingIncrement[0]=increment[0];
	o->pendingIncrement[1]=increment[1];

	o->pendingUpdate=1;
	
	o->cv=cv;
	o->aliasing=aliasing;
	o->width=width;
	
//	if(!o->channel)
//		rprintf(0,"inc %d %d cv %x rate % 6d % 6d\n",increment[0],increment[1],o->cv,VIRTUAL_CLOCK/period[0],VIRTUAL_CLOCK/period[1]);
}

static FORCEINLINE int32_t handleCounterUnderflow(struct wtosc_s * o, int32_t bufIdx, oscSyncMode_t syncMode, uint32_t * syncResets)
{
	int32_t curPeriod,curIncrement;
	
	if(o->phase>=o->halfSampleCount)
	{
		curPeriod=o->period[1];
		curIncrement=o->increment[1];
	}
	else
	{
		curPeriod=o->period[0];
		curIncrement=o->increment[0];
	}

	o->counter+=curPeriod;

	o->phase-=curIncrement;

	if(o->phase<0)
	{
		o->phase+=o->sampleCount;

		// sync (master side)
		if(syncMode==osmMaster)
			*syncResets|=(1<<bufIdx);
	}
	
	if(o->aliasing) // we want aliasing, so make interpolation less effective !
	{
		o->prevSample3=o->prevSample2=o->curSample;
	}
	else
	{
		o->prevSample3=o->prevSample2;
		o->prevSample2=o->prevSample;
	}
	
	o->prevSample=o->curSample;
	o->curSample=o->data[o->phase];
	
	return curPeriod;
}

static FORCEINLINE uint16_t interpolate(int32_t alpha, int32_t cur, int32_t prev, int32_t prev2, int32_t prev3)
{
	uint16_t r;
	
#ifdef USE_HERMITE_INTERP
	int32_t v,p0,p1,p2,total;

	// do 4-point, 3rd-order Hermite/Catmull-Rom spline (x-form) interpolation

	v=prev2-prev;
	p0=((prev3-cur-v)>>1)-v;
	p1=cur+v*2-((prev3+prev)>>1);
	p2=(prev2-cur)>>1;

	total=((p0*alpha)>>FRAC_SHIFT)+p1;
	total=((total*alpha)>>FRAC_SHIFT)+p2;
	total=((total*alpha)>>FRAC_SHIFT)+prev;

	r=__USAT(total,16);
#else
	// do linear interpolation

	r=(alpha*prev+(((1<<FRAC_SHIFT)-1)-alpha)*cur)>>FRAC_SHIFT;
#endif
	
	return r;
}

FORCEINLINE void wtosc_update(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer, oscSyncMode_t syncMode, uint32_t * syncResets)
{
	uint16_t r;
	int32_t buf;
	int32_t alphaDiv,curHalf;
	
	if(!o->data)
		return;
	
	curHalf=o->phase>=o->halfSampleCount?1:0;

	if(o->pendingUpdate)
	{
		// adjust phase to avoid audio glitches on increment changes
		o->phase-=o->pendingIncrement[curHalf]-o->increment[curHalf];
		
		o->period[0]=o->pendingPeriod[0];
		o->period[1]=o->pendingPeriod[1];
		o->increment[0]=o->pendingIncrement[0];
		o->increment[1]=o->pendingIncrement[1];
		o->pendingUpdate=0;
	}

	alphaDiv=o->period[curHalf];

	if(syncMode==osmMaster)
		*syncResets=0; // init
	
	for(buf=startBuffer;buf<=endBuffer;++buf)
	{
		// counter update
		
		o->counter-=VIRTUAL_DAC_TICK_RATE;
		
		// sync (slave side)
		
		if(syncMode==osmSlave)
		{
			if(*syncResets&1)
			{
				o->phase=-1;
				o->counter=-1;
			}
			*syncResets>>=1;
		}

		// counter underflow management
		
		if(o->counter<0)
			alphaDiv=handleCounterUnderflow(o,buf-startBuffer,syncMode,syncResets);

		// interpolate
		
		r=interpolate(((uint32_t)o->counter<<FRAC_SHIFT)/alphaDiv,o->curSample,o->prevSample,o->prevSample2,o->prevSample3);
		
		// send value to DACs

		dacspi_setOscValue(buf,o->channel,r);
	}
}
