///////////////////////////////////////////////////////////////////////////////
// Wavetable oscillator
///////////////////////////////////////////////////////////////////////////////

#include "wtosc.h"
#include "dacspi.h"
#include "osc_curves.h"

#define CLOCK SYNTH_MASTER_CLOCK
#define TICK_RATE DACSPI_TICK_RATE

#define MAX_SAMPLERATE (CLOCK/TICK_RATE)

#define WIDTH_MOD_BITS 14
#define FRAC_SHIFT 12

static uint16_t incModLUT[WTOSC_SAMPLE_COUNT/2] EXT_RAM;
static int8_t incModLUT_done=0;

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

static FORCEINLINE int32_t handleCounterUnderflow(struct wtosc_s * o, int32_t bufIdx, oscSyncMode_t syncMode, int16_t * syncPositions)
{
	int32_t curPeriod,curIncrement;
	
	if(o->phase>=WTOSC_SAMPLE_COUNT/2)
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
		o->phase+=WTOSC_SAMPLE_COUNT;

		updatePeriodIncrement(o,1);
	
		// sync (master side)
		if(syncMode==osmMaster)
		{
			syncPositions[bufIdx]=o->counter-curPeriod;
		}
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
	o->curSample=lerp16(o->mainData[o->phase],o->crossoverData[o->phase],o->crossover);
	
	return curPeriod;
}

void wtosc_init(struct wtosc_s * o, int32_t channel)
{
	if(!incModLUT_done)
	{
		uint16_t *p=&incModLUT[0];
		for(uint16_t i=0;i<WTOSC_SAMPLE_COUNT/2;++i)
		{
			uint16_t inc=i+1;
			while((WTOSC_SAMPLE_COUNT/2)%inc) ++inc;
			*p++=inc;
		}
		incModLUT_done=1;
	}	

	memset(o,0,sizeof(struct wtosc_s));

	o->channel=channel;
	
	wtosc_setSampleData(o,NULL,NULL);
	wtosc_setParameters(o,MIDDLE_C_NOTE*WTOSC_CV_SEMITONE,HALF_RANGE,HALF_RANGE,0);
	updatePeriodIncrement(o,1);
}

void wtosc_setSampleData(struct wtosc_s * o, uint16_t * mainData, uint16_t * xovrData)
{
	o->mainData=mainData;
	o->crossoverData=xovrData;
}

void wtosc_setParameters(struct wtosc_s * o, uint16_t pitch, uint16_t aliasing, uint16_t width, uint16_t crossover)
{
	uint64_t frequency;
	uint32_t sampleRate[2];
	int32_t increment[2], period[2], aliasing_s, crossover_s;
	
	pitch=MIN(WTOSC_HIGHEST_NOTE*WTOSC_CV_SEMITONE,pitch);
	
	aliasing_s=aliasing;
	aliasing_s+=INT16_MIN;
	if(aliasing_s>=0)
	{
		aliasing_s>>=8;
	}
	else
	{
		aliasing_s=-aliasing_s;
		aliasing_s>>=4;
	}
	
	width=MAX(UINT16_MAX/32,width);
	width=MIN((31*UINT16_MAX)/32,width);
	width>>=16-WIDTH_MOD_BITS;

	crossover_s=crossover;
	crossover_s+=INT16_MIN;
	crossover_s=abs(crossover_s);
	crossover_s=crossover_s<<1;
	
	if(pitch==o->pitch && aliasing_s==o->aliasing && width==o->width && crossover_s==o->crossover)
		return;	
	
	frequency=(uint64_t)cvToFrequency(pitch)*(WTOSC_SAMPLE_COUNT/2);

	sampleRate[0]=frequency/((1<<WIDTH_MOD_BITS)-width);
	sampleRate[1]=frequency/width;
	
	increment[0]=1+(sampleRate[0]/MAX_SAMPLERATE);
	increment[1]=1+(sampleRate[1]/MAX_SAMPLERATE);

	if(increment[0]<WTOSC_SAMPLE_COUNT/2) increment[0]=incModLUT[increment[0]];
	if(increment[1]<WTOSC_SAMPLE_COUNT/2) increment[1]=incModLUT[increment[1]];
	
	increment[0]=MIN(WTOSC_SAMPLE_COUNT,increment[0]+aliasing_s);
	increment[1]=MIN(WTOSC_SAMPLE_COUNT,increment[1]+aliasing_s);
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
	
//	if(!o->channel)
//		rprintf(0,"inc %d %d cv %x rate % 6d % 6d\n",increment[0],increment[1],o->pitch,sampleRate[0],sampleRate[1]);
}

FORCEINLINE void wtosc_update(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer, oscSyncMode_t syncMode, int16_t *syncPositions)
{
	uint16_t r;
	int32_t buf;
	int32_t alphaDiv,curHalf;
	
	if(!o->mainData)
	{
		for(buf=startBuffer;buf<=endBuffer;++buf)
		{
			int32_t bufIdx=buf-startBuffer;

			// counter update

			o->counter-=TICK_RATE;

			// counter underflow management

			if(o->counter<0)
				alphaDiv=handleCounterUnderflow(o,bufIdx,syncMode,syncPositions);

			// silence DAC
			
			dacspi_setOscValue(buf,o->channel,HALF_RANGE);
		}
		return;
	}
	
	updatePeriodIncrement(o,2);
	
	curHalf=o->phase>=WTOSC_SAMPLE_COUNT/2?1:0;
	alphaDiv=o->period[curHalf];

	if(syncMode==osmSlave)
	{
		for(buf=startBuffer;buf<=endBuffer;++buf)
		{
			int32_t bufIdx=buf-startBuffer;
			
			// counter update

			o->counter-=TICK_RATE;

			// sync (slave side)

			int16_t *sp=&syncPositions[bufIdx];
			if(*sp>INT16_MIN)
			{
				o->phase=0;
				o->counter=*sp;
				*sp=INT16_MIN;
			}

			// counter underflow management

			if(o->counter<0)
				alphaDiv=handleCounterUnderflow(o,bufIdx,osmNone,NULL);

			// interpolate

			r=herp((o->counter<<FRAC_SHIFT)/alphaDiv,o->curSample,o->prevSample,o->prevSample2,o->prevSample3,FRAC_SHIFT);

			// send value to DAC

			dacspi_setOscValue(buf,o->channel,r);
		}
	}
	else
	{
		for(buf=startBuffer;buf<=endBuffer;++buf)
		{
			int32_t bufIdx=buf-startBuffer;

			// counter update

			o->counter-=TICK_RATE;

			// counter underflow management

			if(o->counter<0)
				alphaDiv=handleCounterUnderflow(o,bufIdx,syncMode,syncPositions);

			// interpolate

			r=herp((o->counter<<FRAC_SHIFT)/alphaDiv,o->curSample,o->prevSample,o->prevSample2,o->prevSample3,FRAC_SHIFT);

			// send value to DAC

			dacspi_setOscValue(buf,o->channel,r);
		}
	}
}
