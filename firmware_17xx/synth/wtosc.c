///////////////////////////////////////////////////////////////////////////////
// Wavetable oscillator
///////////////////////////////////////////////////////////////////////////////

#include "wtosc.h"
#include "dacspi.h"
#include "osc_curves.h"

#define CLOCK SYNTH_MASTER_CLOCK
#define TICK_RATE DACSPI_TICK_RATE

#define MAX_SAMPLERATE (CLOCK/TICK_RATE)

#define USE_HERMITE_INTERP

#define WIDTH_MOD_BITS 9
#define FRAC_SHIFT 12

static FORCEINLINE uint32_t cvToFrequency(uint32_t cv) // returns the frequency shifted by 8
{
	uint32_t v;
	
	v=cv%(12*WTOSC_CV_SEMITONE); // offset in the octave
	v=(v*21)<<8; // phase for computeShape
	v=(uint32_t)computeShape(v,oscOctaveCurve,1)+32768; // octave frequency in the 12th octave
	v=(v<<WIDTH_MOD_BITS)>>(12-(cv/(12*WTOSC_CV_SEMITONE))); // full frequency shifted by WIDTH_MOD_BITS
	
	return v;
}

static FORCEINLINE void updatePeriodIncrement(struct wtosc_s * o, int8_t type)
{
	if(o->pendingUpdate>=type)
	{
		o->increment[0]=o->pendingIncrement[0];
		o->increment[1]=o->pendingIncrement[1];
		o->period[0]=o->pendingPeriod[0];
		o->period[1]=o->pendingPeriod[1];
		
		o->pendingUpdate=0;
	}
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

		updatePeriodIncrement(o,1);
	
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
	o->curSample=lerp16(o->data[0][o->phase],o->data[1][o->phase],o->crossover);
	
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

void wtosc_init(struct wtosc_s * o, int32_t channel)
{
	memset(o,0,sizeof(struct wtosc_s));

	o->channel=channel;
	
	wtosc_setSampleData(o,NULL,NULL,WTOSC_MAX_SAMPLES);
	wtosc_setParameters(o,MIDDLE_C_NOTE*WTOSC_CV_SEMITONE,HALF_RANGE,HALF_RANGE,0);
	updatePeriodIncrement(o,1);
}

void wtosc_setSampleData(struct wtosc_s * o, uint16_t * mainData, uint16_t * xovrData, uint16_t sampleCount)
{
	o->sampleCount=sampleCount;
	o->halfSampleCount=sampleCount>>1;

	o->data[0]=NULL;
	o->data[1]=NULL;
	if(sampleCount<=WTOSC_MAX_SAMPLES)
	{
		o->data[0]=mainData;
		o->data[1]=xovrData;
	}
}

void wtosc_setParameters(struct wtosc_s * o, uint16_t pitch, uint16_t aliasing, uint16_t width, uint16_t crossover)
{
	uint32_t sampleRate[2], frequency;
	int32_t increment[2], period[2], aliasing_s, crossover_s;
	
	pitch=MIN(WTOSC_HIGHEST_NOTE*WTOSC_CV_SEMITONE,pitch);
	
	aliasing_s=aliasing;
	aliasing_s+=INT16_MIN;
	if(aliasing_s>=0)
	{
		aliasing_s>>=9;
	}
	else
	{
		aliasing_s=-aliasing_s;
		aliasing_s>>=6;
	}
	
	width=MAX(UINT16_MAX/16,width);
	width=MIN((15*UINT16_MAX)/16,width);
	width>>=16-WIDTH_MOD_BITS;

	crossover_s=crossover;
	crossover_s+=INT16_MIN;
	crossover_s=abs(crossover_s);
	crossover_s=crossover_s<<1;
	
	if(pitch==o->pitch && aliasing_s==o->aliasing && width==o->width && crossover_s==o->crossover)
		return;	
	
	frequency=cvToFrequency(pitch)*o->halfSampleCount;

	sampleRate[0]=frequency/((1<<WIDTH_MOD_BITS)-width);
	sampleRate[1]=frequency/width;
	
	increment[0]=1+(sampleRate[0]/MAX_SAMPLERATE);
	increment[1]=1+(sampleRate[1]/MAX_SAMPLERATE);

	while(o->halfSampleCount%increment[0]) ++increment[0];
	while(o->halfSampleCount%increment[1]) ++increment[1];
	
	increment[0]=MIN(o->sampleCount,increment[0]+aliasing_s);
	increment[1]=MIN(o->sampleCount,increment[1]+aliasing_s);
	period[0]=CLOCK/(sampleRate[0]/increment[0]);
	period[1]=CLOCK/(sampleRate[1]/increment[1]);	
	
	o->pendingIncrement[0]=increment[0];
	o->pendingIncrement[1]=increment[1];
	o->pendingPeriod[0]=period[0];	
	o->pendingPeriod[1]=period[1];	

	o->pendingUpdate=(pitch==o->pitch && crossover_s==o->crossover && aliasing_s==o->aliasing)?1:2; // width change needs delayed update (waiting for phase)
	
	o->pitch=pitch;
	o->crossover=crossover_s;
	o->width=width;
	o->aliasing=aliasing_s;
	
//	 if(!o->channel)
//		 rprintf(0,"inc %d %d cv %x rate % 6d % 6d\n",increment[0],increment[1],o->pitch,CLOCK/period[0],CLOCK/period[1]);
}

FORCEINLINE void wtosc_update(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer, oscSyncMode_t syncMode, uint32_t * syncResets)
{
	uint16_t r;
	int32_t buf;
	int32_t alphaDiv,curHalf;
	
	if(!o->data)
		return;
	
	updatePeriodIncrement(o,2);
	
	curHalf=o->phase>=o->halfSampleCount?1:0;
	alphaDiv=o->period[curHalf];

	for(buf=startBuffer;buf<=endBuffer;++buf)
	{
		// counter update
		
		o->counter-=TICK_RATE;
		
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
