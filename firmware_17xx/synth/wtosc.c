///////////////////////////////////////////////////////////////////////////////
// Wavetable oscillator
///////////////////////////////////////////////////////////////////////////////

#include "wtosc.h"
#include "dacspi.h"
#include "osc_curves.h"

#define VIRTUAL_CLOCK 3600000000UL
#define VIRTUAL_DAC_TICK_RATE (DACSPI_TICK_RATE*(VIRTUAL_CLOCK/SYNTH_MASTER_CLOCK))

#define MAX_SAMPLERATE(oversampling) (VIRTUAL_CLOCK/((VIRTUAL_DAC_TICK_RATE*(oversampling))/16)) /* oversampling in 1/16th */



static FORCEINLINE uint32_t cvToFrequency(uint32_t cv) // returns the frequency shifted by 8
{
	uint32_t v;
	
	v=cv%(12*WTOSC_CV_SEMITONE); // offset in the octave
	v=(v*20)<<8; // phase for computeShape
	v=(uint32_t)computeShape(v,oscOctaveCurve,1)+32768; // octave frequency in the 12th octave
	v=(v<<8)>>(12-(cv/(12*WTOSC_CV_SEMITONE))); // full frequency shifted by 8
	
	return v;
}

void wtosc_init(struct wtosc_s * o, int8_t voice, int8_t ab)
{
	memset(o,0,sizeof(struct wtosc_s));

	o->voice=voice;
	o->ab=ab;
	
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
	uint32_t sampleRate[2], underSample[2], maxSampleRate, frequency;
	
	if(!o->data)
		return;
	
	aliasing>>=7;
	
	width=MAX(UINT16_MAX/16,width);
	width=MIN((15*UINT16_MAX)/16,width);
	width>>=6;

	cv=MIN(WTOSC_HIGHEST_NOTE*WTOSC_CV_SEMITONE,cv);
	
	if(cv==o->cv && aliasing==o->aliasing && width==o->width)
		return;	
	
	maxSampleRate=MAX_SAMPLERATE(36); // 2.25x oversampling (ensures 20KHz response if DAC samplerate > 90KHz)
	frequency=cvToFrequency(cv)<<10;

	sampleRate[0]=frequency/(1024-width);
	sampleRate[1]=frequency/width;
	
	sampleRate[0]=sampleRate[0]*o->halfSampleCount;
	sampleRate[1]=sampleRate[1]*o->halfSampleCount;

	underSample[0]=1+(sampleRate[0]>>8)/maxSampleRate;
	underSample[1]=1+(sampleRate[1]>>8)/maxSampleRate;

	while(o->halfSampleCount%underSample[0]) ++underSample[0];
	while(o->halfSampleCount%underSample[1]) ++underSample[1];
	
	o->increment[0]=underSample[0]+aliasing;
	o->increment[1]=underSample[1]+aliasing;
	
	o->period[0]=(VIRTUAL_CLOCK/((sampleRate[0]/o->increment[0])>>8));	
	o->period[1]=(VIRTUAL_CLOCK/((sampleRate[1]/o->increment[1])>>8));	

	o->cv=cv;
	o->aliasing=aliasing;
	o->width=width;
	
//	rprintf(0,"inc %d %d cv %x rate % 6d % 6d\n",o->increment[0],o->increment[1],o->cv,sampleRate[0]/(o->increment[0]<<8),sampleRate[1]/(o->increment[1]<<8));
}

static FORCEINLINE int32_t handleCounterUnderflow(struct wtosc_s * o, int32_t bufIdx, oscSyncMode_t syncMode, uint32_t * syncResets)
{
	int32_t curPeriod,curIncrement;
	
	if (o->phase>=o->halfSampleCount)
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

#ifdef WTOSC_HERMITE_INTERP
	o->prevSample3=o->prevSample2;
	o->prevSample2=o->prevSample;
#endif
	o->prevSample=o->curSample;
	o->curSample=o->data[o->phase];
	
	return o->aliasing?INT32_MAX:curPeriod; // we want aliasing, so make alpha fixed to deactivate interpolation !
}

static FORCEINLINE uint16_t interpolate(int32_t alpha, int32_t cur, int32_t prev, int32_t prev2, int32_t prev3)
{
	uint16_t r;
	
#ifdef WTOSC_HERMITE_INTERP
	int32_t v,p0,p1,p2,total;

	// do 4-point, 3rd-order Hermite/Catmull-Rom spline (x-form) interpolation

	v=prev2-prev;
	p0=((prev3-cur-v)>>1)-v;
	p1=cur+v*2-((prev3+prev)>>1);
	p2=(prev2-cur)>>1;

	total=((p0*alpha)>>12)+p1;
	total=((total*alpha)>>12)+p2;
	total=((total*alpha)>>12)+prev;

	r=__USAT(total,16);
#else
	// do linear interpolation

	r=(alpha*prev+(4095-alpha)*cur)>>12;
#endif
	
	return r;
}

FORCEINLINE void wtosc_update(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer, oscSyncMode_t syncMode, uint32_t * syncResets)
{
	uint16_t r;
	int32_t buf;
	int32_t alphaDiv;
	uint32_t slaveSyncResets,slaveSyncResetsMask;
	
	if(!o->data)
		return;
	
	alphaDiv=o->period[o->phase>=o->halfSampleCount?1:0];

	slaveSyncResets=0;
	if(syncMode==osmMaster)
		*syncResets=0; // init
	else if(syncMode==osmSlave)
		slaveSyncResets=*syncResets;
	
	for(buf=startBuffer;buf<=endBuffer;++buf)
	{
		// counter management
		
		o->counter-=VIRTUAL_DAC_TICK_RATE;
		
		// sync (slave side)
		
		slaveSyncResetsMask=-(slaveSyncResets&1);
		o->phase|=slaveSyncResetsMask;
		o->counter|=slaveSyncResetsMask;
		slaveSyncResets>>=1;

		// counter underflow management
		
		if(o->counter<0)
			alphaDiv=handleCounterUnderflow(o,buf-startBuffer,syncMode,syncResets);

		// interpolate
		
		r=interpolate(((uint32_t)o->counter<<12)/alphaDiv,o->curSample,o->prevSample,
#ifdef WTOSC_HERMITE_INTERP
				o->prevSample2,o->prevSample3
#else
				0,0
#endif
		);
		
		// dend value to DACs

		dacspi_setVoiceValue(buf,o->voice,o->ab,r);
	}
}
