////////////////////////////////////////////////////////////////////////////////
// Tunes CVs, by sampling audio though the TLV2556 ADC
////////////////////////////////////////////////////////////////////////////////

#include "tuner.h"
#include "storage.h"
#include "synth.h"
#include "storage.h"
#include "scan.h"
#include "dacspi.h"

#define TUNER_SR_BUF_DIV 4 // divide the sample rate by this and you get the buffer length, this is also the lowest theoretical tunable frequency

#define TUNER_MIDDLE_C_HERTZ 261.63
#define TUNER_LOWEST_HERTZ (TUNER_MIDDLE_C_HERTZ/16)

#define TUNER_FIL_INIT_OFFSET 6500
#define TUNER_FIL_INIT_SCALE (65535/13)
#define TUNER_FIL_NTH_C_LO 5
#define TUNER_FIL_NTH_C_HI 7

static uint16_t extrapolateUpperOctavesTunes(int8_t voice, int8_t oct)
{
	uint32_t v;
	
	v=settings.tunes[TUNER_OCTAVE_COUNT-1][voice]-settings.tunes[TUNER_OCTAVE_COUNT-2][voice];
	v*=oct-TUNER_OCTAVE_COUNT+1;
	v+=settings.tunes[TUNER_OCTAVE_COUNT-1][voice];
	
	return MIN(v,UINT16_MAX);
}

static LOWERCODESIZE void prepareSynth(void)
{
	synth_refreshCV(-1,cvResonance,0,1);
	synth_refreshCV(-1,cvNoiseVol,0,1);
	synth_refreshCV(-1,cvAVol,0,1);
	synth_refreshCV(-1,cvBVol,0,1);
}

static NOINLINE LOWERCODESIZE uint32_t measureThruZeroCount(void)
{
	uint32_t tzCnt=0;
	uint16_t prev;
		
#define MAP_BUF_LEN (SCAN_MASTERMIX_SAMPLERATE/TUNER_SR_BUF_DIV)
#define MAP_BUF_HALF_RANGE (-INT16_MIN)
	uint16_t buf[MAP_BUF_LEN];

	scan_sampleMasterMix(MAP_BUF_LEN,buf);

	prev=buf[0];
	for(uint16_t pos=0;pos<MAP_BUF_LEN;++pos)
	{
		uint16_t sample=buf[pos];
			
		if ((prev<=MAP_BUF_HALF_RANGE&&sample>MAP_BUF_HALF_RANGE) || (prev>=MAP_BUF_HALF_RANGE&&sample<MAP_BUF_HALF_RANGE))
			++tzCnt;

		prev=sample;
	}
	
	return tzCnt;
}

static LOWERCODESIZE void tuneOffset(int8_t voice,uint8_t nthC)
{
	int8_t i;
	uint16_t estimate,bit;
	uint32_t tzCnt,targetTzCnt;

	targetTzCnt=TUNER_LOWEST_HERTZ*(2<<nthC)/TUNER_SR_BUF_DIV; // 2 because 2 thru zero per waveform period
	
	estimate=0;
	bit=0x8000;

	for(i=0;i<12;++i) // 12bit dac
	{
		estimate|=bit;

		synth_refreshCV(voice,cvCutoff,estimate,1);
		delay_ms(25); // wait analog hardware stabilization	

		tzCnt=measureThruZeroCount();

		// adjust estimate
		if (tzCnt>targetTzCnt)
			estimate&=~bit;

		// on to finer changes
		bit>>=1;
	}

	settings.tunes[nthC][voice]=estimate;

#ifdef DEBUG
	rprintf(0, "cv %d tzCnt %d targetTzCnt %d\n",estimate,tzCnt,targetTzCnt);
#endif
}

static LOWERCODESIZE void tuneFilter(int8_t voice)
{
#ifdef DEBUG		
	rprintf(0, "\ntuning %d\n", voice);
#endif
	int8_t nthC;
	
	// display
	
	rprintf(1,"Fil%d ",voice+1);

	// open VCA

	synth_refreshCV(voice,cvAmp,UINT16_MAX,1);
	
	// tune

	for(nthC=TUNER_FIL_NTH_C_LO;nthC<=TUNER_FIL_NTH_C_HI;++nthC)
		tuneOffset(voice,nthC);

	for(nthC=TUNER_FIL_NTH_C_LO-1;nthC>=0;--nthC)
		settings.tunes[nthC][voice]=(uint32_t)2*settings.tunes[nthC+1][voice]-settings.tunes[nthC+2][voice];

	for(nthC=TUNER_FIL_NTH_C_HI+1;nthC<TUNER_OCTAVE_COUNT;++nthC)
		settings.tunes[nthC][voice]=(uint32_t)2*settings.tunes[nthC-1][voice]-settings.tunes[nthC-2][voice];
	
	// close VCA

	synth_refreshCV(voice,cvAmp,0,1);
}

NOINLINE uint16_t tuner_computeCVFromNote(int8_t voice, uint8_t note, uint8_t nextInterp, cv_t cv)
{
	int8_t loOct,hiOct;
	uint16_t value,loVal,hiVal;
	uint32_t semiTone;
	
	if(cv==cvAPitch || cv==cvBPitch)
	{
		// perfect tuning for oscillators
		value=((uint16_t)note<<8)+nextInterp;
	}
	else
	{
		loOct=note/12;
		hiOct=loOct+1;

		if(loOct<TUNER_OCTAVE_COUNT)
			loVal=settings.tunes[loOct][voice];
		else
			loVal=extrapolateUpperOctavesTunes(voice,loOct);

		if(hiOct<TUNER_OCTAVE_COUNT)
			hiVal=settings.tunes[hiOct][voice];
		else
			hiVal=extrapolateUpperOctavesTunes(voice,hiOct);

		semiTone=(((uint32_t)(note%12)<<16)+((uint16_t)nextInterp<<8))/12;

		value=loVal;
		value+=(semiTone*(hiVal-loVal))>>16;
	}

	return value;
}

LOWERCODESIZE void tuner_init(void)
{
	int8_t cv,oct;
	
	// theoretical base tuning
	
	for(oct=0;oct<TUNER_OCTAVE_COUNT;++oct)
		for(cv=0;cv<TUNER_CV_COUNT;++cv)
		{
			settings.tunes[oct][cv]=TUNER_FIL_INIT_OFFSET+oct*TUNER_FIL_INIT_SCALE;
		}
}

LOWERCODESIZE void tuner_tuneSynth(void)
{
	int8_t v;
	
	BLOCK_INT(1)
	{
		// reinit tuner
		
		tuner_init();
		
		// prepare synth for tuning
		
		scan_setMode(1);
		prepareSynth();
		
		// tune filters
			
			// init
		
		synth_refreshCV(-1,cvResonance,UINT16_MAX/2,1);
	
		for(v=0;v<SYNTH_VOICE_COUNT;++v)
			synth_refreshCV(v,cvCutoff,UINT16_MAX/2,1);
	
			// filters
		
		for(v=0;v<SYNTH_VOICE_COUNT;++v)
			tuneFilter(v);

		// finish
		
		synth_refreshCV(-1,cvResonance,0,1);
		for(v=0;v<SYNTH_VOICE_COUNT;++v)
			synth_refreshCV(v,cvAmp,0,1);
		scan_setMode(0);
	}
}