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
		o->period[0]=o->pendingPeriod[0];
		o->period[1]=o->pendingPeriod[1];
		o->increment[0]=o->pendingIncrement[0];
		o->increment[1]=o->pendingIncrement[1];
		
		o->pendingUpdate=0;
	}
}

static FORCEINLINE void handlePhaseUnderflow(struct wtosc_s * o, int32_t bufIdx, oscSyncMode_t syncMode, int16_t * syncPositions)
{
	if(o->phase<0)
	{
		o->phase+=WTOSC_SAMPLE_COUNT;

		updatePeriodIncrement(o,1);
	
		// sync (master side)
		if(syncMode==osmMaster)
		{
			syncPositions[bufIdx]=o->counter;
		}
	}
}

static FORCEINLINE void handleSlaveSync(struct wtosc_s * o, int32_t bufIdx, int16_t * syncPositions)
{
	int16_t *sp=&syncPositions[bufIdx];
	if(*sp>INT16_MIN)
	{
		o->phase=0;
		o->counter=*sp;
		*sp=INT16_MIN;
	}
}

static FORCEINLINE int32_t handleCounterUnderflow_wmOff(struct wtosc_s * o, int32_t bufIdx, oscSyncMode_t syncMode, int16_t * syncPositions)
{
	int32_t curPeriod,curIncrement;
	
	curPeriod=o->period[0];
	curIncrement=o->increment[0];
	
	o->phase-=curIncrement;

	handlePhaseUnderflow(o,bufIdx,syncMode,syncPositions);

	o->counter+=curPeriod;

	o->prevSample3=o->prevSample2;
	o->prevSample2=o->prevSample;
	o->prevSample=o->curSample;

	o->curSample=o->mainData[o->phase];

	return (1<<(FRAC_SHIFT*2))/curPeriod;
}

static FORCEINLINE int32_t handleCounterUnderflow_wmAliasing(struct wtosc_s * o, int32_t bufIdx, oscSyncMode_t syncMode, int16_t * syncPositions)
{
	int32_t curPeriod,curIncrement;
	
	curPeriod=o->period[0];
	curIncrement=o->increment[0];
	
	o->phase-=curIncrement;

	handlePhaseUnderflow(o,bufIdx,syncMode,syncPositions);

	o->counter+=curPeriod;

	if(o->aliasing)
	{
		o->prevSample3=o->prevSample2=o->curSample; // we want aliasing, so make interpolation less effective !
	}
	else
	{
		o->prevSample3=o->prevSample2;
		o->prevSample2=o->prevSample;
	}
	o->prevSample=o->curSample;

	o->curSample=o->mainData[o->phase];

	return (1<<(FRAC_SHIFT*2))/curPeriod;
}

static FORCEINLINE int32_t handleCounterUnderflow_wmWidth(struct wtosc_s * o, int32_t bufIdx, oscSyncMode_t syncMode, int16_t * syncPositions)
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

	o->phase-=curIncrement;

	handlePhaseUnderflow(o,bufIdx,syncMode,syncPositions);

	o->counter+=curPeriod;

	o->prevSample3=o->prevSample2;
	o->prevSample2=o->prevSample;
	o->prevSample=o->curSample;

	o->curSample=o->mainData[o->phase];

	return (1<<(FRAC_SHIFT*2))/curPeriod;
}

static FORCEINLINE int32_t handleCounterUnderflow_wmCrossOver(struct wtosc_s * o, int32_t bufIdx, oscSyncMode_t syncMode, int16_t * syncPositions)
{
	int32_t curPeriod,curIncrement;
	
	curPeriod=o->period[0];
	curIncrement=o->increment[0];
	
	o->phase-=curIncrement;

	handlePhaseUnderflow(o,bufIdx,syncMode,syncPositions);

	o->counter+=curPeriod;

	o->prevSample3=o->prevSample2;
	o->prevSample2=o->prevSample;
	o->prevSample=o->curSample;

	o->curSample=lerp16(o->mainData[o->phase],o->crossoverData[o->phase],o->crossover);

	return (1<<(FRAC_SHIFT*2))/curPeriod;
}


static FORCEINLINE int32_t handleCounterUnderflow_wmFolder(struct wtosc_s * o, int32_t bufIdx, oscSyncMode_t syncMode, int16_t * syncPositions)
{
	int32_t curPeriod,curIncrement,smp;
	
	curPeriod=o->period[0];
	curIncrement=o->increment[0];
	
	o->phase-=curIncrement;

	handlePhaseUnderflow(o,bufIdx,syncMode,syncPositions);

	o->counter+=curPeriod;

	o->prevSample3=o->prevSample2;
	o->prevSample2=o->prevSample;
	o->prevSample=o->curSample;

	smp=o->mainData[o->phase];
	
	// wave folder
	smp+=INT16_MIN;
	smp*=o->folder;
	smp=(smp>>2)+(1<<24); // smp = smp * 0.25 + 0.25
	smp-=(smp+(1<<25))&0xfc000000; // smp -= round(smp)
	smp^=smp>>31; // smp = abs(smp)
	smp>>=9;

	o->curSample=smp;

	return (1<<(FRAC_SHIFT*2))/curPeriod;
}

static FORCEINLINE int32_t handleCounterUnderflow_wmBitCrush(struct wtosc_s * o, int32_t bufIdx, oscSyncMode_t syncMode, int16_t * syncPositions)
{
	int32_t curPeriod,curIncrement,smp;
	
	curPeriod=o->period[0];
	curIncrement=o->increment[0];
	
	o->phase-=curIncrement;

	handlePhaseUnderflow(o,bufIdx,syncMode,syncPositions);

	o->counter+=curPeriod;

	o->prevSample3=o->prevSample2;
	o->prevSample2=o->prevSample;
	o->prevSample=o->curSample;

	smp=o->mainData[o->phase];

	// bit crusher
	if(o->bitcrush>=0)
		smp+=INT16_MIN;
	smp=(smp<<1)+1;
	smp/=o->bitcrush;
	smp*=o->bitcrush;
	smp>>=1;
	if(o->bitcrush>=0)
		smp-=INT16_MIN;

	o->curSample=smp;

	return (1<<(FRAC_SHIFT*2))/curPeriod;
}

static FORCEINLINE void update_slaveSync_noData(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer, oscSyncMode_t syncMode, int16_t *syncPositions)
{
	int32_t buf;

	for(buf=startBuffer;buf<=endBuffer;++buf)
	{
		// silence DAC

		dacspi_setOscValue(buf,o->channel,HALF_RANGE);
	}
}

static FORCEINLINE void update_slaveSync_wmOff(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer, oscSyncMode_t syncMode, int16_t *syncPositions)
{
	uint16_t r;
	int32_t buf;
	int32_t alphaDiv;
	
	alphaDiv=(1<<(FRAC_SHIFT*2))/o->period[0];

	for(buf=startBuffer;buf<=endBuffer;++buf)
	{
		int32_t bufIdx=buf-startBuffer;

		// counter update

		o->counter-=TICK_RATE;

		// sync (slave side)

		handleSlaveSync(o,bufIdx,syncPositions);

		// counter underflow management

		if(o->counter<0)
			alphaDiv=handleCounterUnderflow_wmOff(o,bufIdx,osmNone,NULL);

		// interpolate

		r=herp((o->counter*alphaDiv)>>FRAC_SHIFT,o->curSample,o->prevSample,o->prevSample2,o->prevSample3,FRAC_SHIFT);

		// send value to DAC

		dacspi_setOscValue(buf,o->channel,r);
	}
}

static FORCEINLINE void update_slaveSync_wmWidth(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer, oscSyncMode_t syncMode, int16_t *syncPositions)
{
	uint16_t r;
	int32_t buf;
	int32_t alphaDiv,curHalf;
	
	curHalf=o->phase>=WTOSC_SAMPLE_COUNT/2?1:0;
	alphaDiv=(1<<(FRAC_SHIFT*2))/o->period[curHalf];

	for(buf=startBuffer;buf<=endBuffer;++buf)
	{
		int32_t bufIdx=buf-startBuffer;

		// counter update

		o->counter-=TICK_RATE;

		// sync (slave side)

		handleSlaveSync(o,bufIdx,syncPositions);

		// counter underflow management

		if(o->counter<0)
			alphaDiv=handleCounterUnderflow_wmWidth(o,bufIdx,osmNone,NULL);

		// interpolate

		r=herp((o->counter*alphaDiv)>>FRAC_SHIFT,o->curSample,o->prevSample,o->prevSample2,o->prevSample3,FRAC_SHIFT);

		// send value to DAC

		dacspi_setOscValue(buf,o->channel,r);
	}
}

static FORCEINLINE void update_slaveSync_wmAliasing(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer, oscSyncMode_t syncMode, int16_t *syncPositions)
{
	uint16_t r;
	int32_t buf;
	int32_t alphaDiv;

	alphaDiv=(1<<(FRAC_SHIFT*2))/o->period[0];

	for(buf=startBuffer;buf<=endBuffer;++buf)
	{
		int32_t bufIdx=buf-startBuffer;

		// counter update

		o->counter-=TICK_RATE;

		// sync (slave side)

		handleSlaveSync(o,bufIdx,syncPositions);

		// counter underflow management

		if(o->counter<0)
			alphaDiv=handleCounterUnderflow_wmAliasing(o,bufIdx,osmNone,NULL);

		// interpolate

		r=herp((o->counter*alphaDiv)>>FRAC_SHIFT,o->curSample,o->prevSample,o->prevSample2,o->prevSample3,FRAC_SHIFT);

		// send value to DAC

		dacspi_setOscValue(buf,o->channel,r);
	}
}

static FORCEINLINE void update_slaveSync_wmCrossOver(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer, oscSyncMode_t syncMode, int16_t *syncPositions)
{
	uint16_t r;
	int32_t buf;
	int32_t alphaDiv;

	alphaDiv=(1<<(FRAC_SHIFT*2))/o->period[0];

	for(buf=startBuffer;buf<=endBuffer;++buf)
	{
		int32_t bufIdx=buf-startBuffer;

		// counter update

		o->counter-=TICK_RATE;

		// sync (slave side)

		handleSlaveSync(o,bufIdx,syncPositions);

		// counter underflow management

		if(o->counter<0)
			alphaDiv=handleCounterUnderflow_wmCrossOver(o,bufIdx,osmNone,NULL);

		// interpolate

		r=herp((o->counter*alphaDiv)>>FRAC_SHIFT,o->curSample,o->prevSample,o->prevSample2,o->prevSample3,FRAC_SHIFT);

		// send value to DAC

		dacspi_setOscValue(buf,o->channel,r);
	}
}

static FORCEINLINE void update_slaveSync_wmFolder(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer, oscSyncMode_t syncMode, int16_t *syncPositions)
{
	uint16_t r;
	int32_t buf;
	int32_t alphaDiv;

	alphaDiv=(1<<(FRAC_SHIFT*2))/o->period[0];

	for(buf=startBuffer;buf<=endBuffer;++buf)
	{
		int32_t bufIdx=buf-startBuffer;

		// counter update

		o->counter-=TICK_RATE;

		// sync (slave side)

		handleSlaveSync(o,bufIdx,syncPositions);

		// counter underflow management

		if(o->counter<0)
			alphaDiv=handleCounterUnderflow_wmFolder(o,bufIdx,osmNone,NULL);

		// interpolate

		r=herp((o->counter*alphaDiv)>>FRAC_SHIFT,o->curSample,o->prevSample,o->prevSample2,o->prevSample3,FRAC_SHIFT);

		// send value to DAC

		dacspi_setOscValue(buf,o->channel,r);
	}
}

static FORCEINLINE void update_slaveSync_wmBitCrush(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer, oscSyncMode_t syncMode, int16_t *syncPositions)
{
	uint16_t r;
	int32_t buf;
	int32_t alphaDiv;

	alphaDiv=(1<<(FRAC_SHIFT*2))/o->period[0];

	for(buf=startBuffer;buf<=endBuffer;++buf)
	{
		int32_t bufIdx=buf-startBuffer;

		// counter update

		o->counter-=TICK_RATE;

		// sync (slave side)

		handleSlaveSync(o,bufIdx,syncPositions);

		// counter underflow management

		if(o->counter<0)
			alphaDiv=handleCounterUnderflow_wmBitCrush(o,bufIdx,osmNone,NULL);

		// interpolate

		r=herp((o->counter*alphaDiv)>>FRAC_SHIFT,o->curSample,o->prevSample,o->prevSample2,o->prevSample3,FRAC_SHIFT);

		// send value to DAC

		dacspi_setOscValue(buf,o->channel,r);
	}
}

static FORCEINLINE void update_masterSync_noData(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer, oscSyncMode_t syncMode, int16_t *syncPositions)
{
	int32_t buf;

	for(buf=startBuffer;buf<=endBuffer;++buf)
	{
		int32_t bufIdx=buf-startBuffer;

		// counter update

		o->counter-=TICK_RATE;

		// counter underflow management

		if(o->counter<0)
			handleCounterUnderflow_wmWidth(o,bufIdx,syncMode,syncPositions); // needs wmWidth to handle all cases

		// silence DAC

		dacspi_setOscValue(buf,o->channel,HALF_RANGE);
	}
}

static FORCEINLINE void update_masterSync_wmOff(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer, oscSyncMode_t syncMode, int16_t *syncPositions)
{
	uint16_t r;
	int32_t buf;
	int32_t alphaDiv;

	alphaDiv=(1<<(FRAC_SHIFT*2))/o->period[0];

	for(buf=startBuffer;buf<=endBuffer;++buf)
	{
		int32_t bufIdx=buf-startBuffer;

		// counter update

		o->counter-=TICK_RATE;

		// counter underflow management

		if(o->counter<0)
			alphaDiv=handleCounterUnderflow_wmOff(o,bufIdx,syncMode,syncPositions);

		// interpolate

		r=herp((o->counter*alphaDiv)>>FRAC_SHIFT,o->curSample,o->prevSample,o->prevSample2,o->prevSample3,FRAC_SHIFT);

		// send value to DAC

		dacspi_setOscValue(buf,o->channel,r);
	}
}

static FORCEINLINE void update_masterSync_wmWidth(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer, oscSyncMode_t syncMode, int16_t *syncPositions)
{
	uint16_t r;
	int32_t buf;
	int32_t alphaDiv,curHalf;
	
	curHalf=o->phase>=WTOSC_SAMPLE_COUNT/2?1:0;
	alphaDiv=(1<<(FRAC_SHIFT*2))/o->period[curHalf];

	for(buf=startBuffer;buf<=endBuffer;++buf)
	{
		int32_t bufIdx=buf-startBuffer;

		// counter update

		o->counter-=TICK_RATE;

		// sync (slave side)

		handleSlaveSync(o,bufIdx,syncPositions);

		// counter underflow management

		if(o->counter<0)
			alphaDiv=handleCounterUnderflow_wmWidth(o,bufIdx,syncMode,syncPositions);

		// interpolate

		r=herp((o->counter*alphaDiv)>>FRAC_SHIFT,o->curSample,o->prevSample,o->prevSample2,o->prevSample3,FRAC_SHIFT);

		// send value to DAC

		dacspi_setOscValue(buf,o->channel,r);
	}
}

static FORCEINLINE void update_masterSync_wmAliasing(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer, oscSyncMode_t syncMode, int16_t *syncPositions)
{
	uint16_t r;
	int32_t buf;
	int32_t alphaDiv;

	alphaDiv=(1<<(FRAC_SHIFT*2))/o->period[0];

	for(buf=startBuffer;buf<=endBuffer;++buf)
	{
		int32_t bufIdx=buf-startBuffer;

		// counter update

		o->counter-=TICK_RATE;

		// sync (slave side)

		handleSlaveSync(o,bufIdx,syncPositions);

		// counter underflow management

		if(o->counter<0)
			alphaDiv=handleCounterUnderflow_wmAliasing(o,bufIdx,syncMode,syncPositions);

		// interpolate

		r=herp((o->counter*alphaDiv)>>FRAC_SHIFT,o->curSample,o->prevSample,o->prevSample2,o->prevSample3,FRAC_SHIFT);

		// send value to DAC

		dacspi_setOscValue(buf,o->channel,r);
	}
}

static FORCEINLINE void update_masterSync_wmCrossOver(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer, oscSyncMode_t syncMode, int16_t *syncPositions)
{
	uint16_t r;
	int32_t buf;
	int32_t alphaDiv;

	alphaDiv=(1<<(FRAC_SHIFT*2))/o->period[0];

	for(buf=startBuffer;buf<=endBuffer;++buf)
	{
		int32_t bufIdx=buf-startBuffer;

		// counter update

		o->counter-=TICK_RATE;

		// sync (slave side)

		handleSlaveSync(o,bufIdx,syncPositions);

		// counter underflow management

		if(o->counter<0)
			alphaDiv=handleCounterUnderflow_wmCrossOver(o,bufIdx,syncMode,syncPositions);

		// interpolate

		r=herp((o->counter*alphaDiv)>>FRAC_SHIFT,o->curSample,o->prevSample,o->prevSample2,o->prevSample3,FRAC_SHIFT);

		// send value to DAC

		dacspi_setOscValue(buf,o->channel,r);
	}
}

static FORCEINLINE void update_masterSync_wmFolder(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer, oscSyncMode_t syncMode, int16_t *syncPositions)
{
	uint16_t r;
	int32_t buf;
	int32_t alphaDiv;

	alphaDiv=(1<<(FRAC_SHIFT*2))/o->period[0];

	for(buf=startBuffer;buf<=endBuffer;++buf)
	{
		int32_t bufIdx=buf-startBuffer;

		// counter update

		o->counter-=TICK_RATE;

		// sync (slave side)

		handleSlaveSync(o,bufIdx,syncPositions);

		// counter underflow management

		if(o->counter<0)
			alphaDiv=handleCounterUnderflow_wmFolder(o,bufIdx,syncMode,syncPositions);

		// interpolate

		r=herp((o->counter*alphaDiv)>>FRAC_SHIFT,o->curSample,o->prevSample,o->prevSample2,o->prevSample3,FRAC_SHIFT);

		// send value to DAC

		dacspi_setOscValue(buf,o->channel,r);
	}
}

static FORCEINLINE void update_masterSync_wmBitCrush(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer, oscSyncMode_t syncMode, int16_t *syncPositions)
{
	uint16_t r;
	int32_t buf;
	int32_t alphaDiv;

	alphaDiv=(1<<(FRAC_SHIFT*2))/o->period[0];

	for(buf=startBuffer;buf<=endBuffer;++buf)
	{
		int32_t bufIdx=buf-startBuffer;

		// counter update

		o->counter-=TICK_RATE;

		// sync (slave side)

		handleSlaveSync(o,bufIdx,syncPositions);

		// counter underflow management

		if(o->counter<0)
			alphaDiv=handleCounterUnderflow_wmBitCrush(o,bufIdx,syncMode,syncPositions);

		// interpolate

		r=herp((o->counter*alphaDiv)>>FRAC_SHIFT,o->curSample,o->prevSample,o->prevSample2,o->prevSample3,FRAC_SHIFT);

		// send value to DAC

		dacspi_setOscValue(buf,o->channel,r);
	}
}

void wtosc_init(struct wtosc_s * o, int8_t channel)
{
	memset(o,0,sizeof(struct wtosc_s));

	o->channel=channel;
	
	wtosc_setSampleData(o,NULL,NULL);
	wtosc_setParameters(o,MIDDLE_C_NOTE*WTOSC_CV_SEMITONE,wmOff,HALF_RANGE);
	updatePeriodIncrement(o,1);
}

FORCEINLINE void wtosc_setSampleData(struct wtosc_s * o, uint16_t * mainData, uint16_t * xovrData)
{
	o->mainData=mainData;
	o->crossoverData=xovrData;
}

FORCEINLINE void wtosc_setParameters(struct wtosc_s * o, uint16_t pitch, oscWModTarget_t wmType, uint16_t wmAmount)
{
	uint64_t frequency;
	uint32_t sampleRate[2];
	int32_t increment[2], period[2], aliasing_s, crossover_s, folder_s, bitcrush_s;
	uint16_t width, wlim;
	
	pitch=MIN(WTOSC_HIGHEST_NOTE*WTOSC_CV_SEMITONE,pitch);
	
	width=HALF_RANGE>>(16-WIDTH_MOD_BITS);
	aliasing_s=0;
	crossover_s=0;
	folder_s=UINT16_MAX/32;
	bitcrush_s=1;
	
	switch(wmType)
	{
	case wmWidth:
		wlim=pitch>>2;
		width=wmAmount;
		width=MAX(wlim,width);
		width=MIN(UINT16_MAX-wlim,width);
		width>>=16-WIDTH_MOD_BITS;
		break;
	case wmAliasing:
		aliasing_s=wmAmount;
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
		break;
	case wmCrossOver:
		crossover_s=wmAmount;
		crossover_s=abs(crossover_s+INT16_MIN);
		crossover_s=__USAT(crossover_s<<1,16);
		break;
	case wmBitCrush:
		bitcrush_s=wmAmount;
		bitcrush_s=bitcrush_s+INT16_MIN;
		if(!bitcrush_s)
			bitcrush_s=1;
		break;
	case wmFolder:
		folder_s=wmAmount;
		folder_s=abs(folder_s+INT16_MIN);
		folder_s=__USAT((folder_s<<1)+UINT16_MAX/32,16);
		break;
	default:
		/* nothing */
		break;
	}
	
	if(pitch!=o->pitch || width!=o->width || aliasing_s!=o->aliasing)
	{
		frequency=(uint64_t)cvToFrequency(pitch)*(WTOSC_SAMPLE_COUNT/2);

		sampleRate[0]=frequency/((1<<WIDTH_MOD_BITS)-width);
		sampleRate[1]=frequency/width;

		increment[0]=sampleRate[0]/MAX_SAMPLERATE;
		increment[1]=sampleRate[1]/MAX_SAMPLERATE;

		increment[0]=oscIncModLUT[increment[0]];
		increment[1]=oscIncModLUT[increment[1]];

		increment[0]=MIN(WTOSC_SAMPLE_COUNT,increment[0]+aliasing_s);
		increment[1]=MIN(WTOSC_SAMPLE_COUNT,increment[1]+aliasing_s);
		period[0]=CLOCK/(sampleRate[0]/increment[0]);
		period[1]=CLOCK/(sampleRate[1]/increment[1]);	

		o->pendingPeriod[0]=period[0];	
		o->pendingPeriod[1]=period[1];	
		o->pendingIncrement[0]=increment[0];
		o->pendingIncrement[1]=increment[1];

		o->pendingUpdate=(pitch==o->pitch && aliasing_s==o->aliasing)?1:2; // width change alone needs delayed update (waiting for phase)

		o->pitch=pitch;
		o->width=width;
		o->aliasing=aliasing_s;

//		if(!o->channel)
//			rprintf(0,"inc %d %d cv %x rate % 6d % 6d per % 6d % 6d\n",increment[0],increment[1],o->pitch,sampleRate[0],sampleRate[1],period[0],period[1]);
	}
	
	o->crossover=crossover_s;
	o->folder=folder_s;
	o->bitcrush=bitcrush_s;
	
	o->wmType=wmType;
}

void wtosc_update(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer, oscSyncMode_t syncMode, int16_t *syncPositions)
{
	typedef void(*update_t)(struct wtosc_s *, int32_t, int32_t, oscSyncMode_t, int16_t *);	

	static const update_t update[wmCount*2*2] = {
		// (noData; masterSync), (data; masterSync), (noData; slaveSync), (data; slaveSync), 
		update_masterSync_noData,	update_masterSync_wmOff,		update_slaveSync_noData,	update_slaveSync_wmOff,
		update_masterSync_noData,	update_masterSync_wmAliasing,	update_slaveSync_noData,	update_slaveSync_wmAliasing,
		update_masterSync_noData,	update_masterSync_wmWidth,		update_slaveSync_noData,	update_slaveSync_wmWidth,
		update_masterSync_noData,	update_masterSync_wmOff,		update_slaveSync_noData,	update_slaveSync_wmOff,
		update_masterSync_noData,	update_masterSync_wmCrossOver,	update_slaveSync_noData,	update_slaveSync_wmCrossOver,
		update_masterSync_noData,	update_masterSync_wmFolder,		update_slaveSync_noData,	update_slaveSync_wmFolder,
		update_masterSync_noData,	update_masterSync_wmBitCrush,	update_slaveSync_noData,	update_slaveSync_wmBitCrush,
	};
	
	updatePeriodIncrement(o,2);
	
	uint8_t mode=(o->wmType<<2)|((syncMode==osmSlave?1:0)<<1)|(o->mainData?1:0);

	update[mode](o,startBuffer,endBuffer,syncMode,syncPositions);
}
