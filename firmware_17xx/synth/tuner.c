////////////////////////////////////////////////////////////////////////////////
// Tunes CVs, by sampling audio though the TLV2556 ADC
////////////////////////////////////////////////////////////////////////////////

#include "tuner.h"
#include "storage.h"
#include "synth.h"
#include "storage.h"
#include "scan.h"
#include "dacspi.h"

#define TUNER_TICK (SYNTH_MASTER_CLOCK/DACSPI_TICK_RATE)

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

static NOINLINE LOWERCODESIZE uint32_t measureAudioPeriod(uint8_t periods) // in TUNER_TICK ticks
{
	uint32_t tzCnt=0;
	uint8_t prev;
		
#define MAP_BUF_LEN 3200
#define MAP_BUF_MIDDLE 128
	uint8_t buf[MAP_BUF_LEN];

	for(uint8_t p=0;p<periods;++p)
	{
		scan_sampleMasterMix(MAP_BUF_LEN,buf);

		prev=buf[0];
		for(uint16_t pos=0;pos<MAP_BUF_LEN;++pos)
		{
			uint8_t sample=buf[pos];
			
			if ((prev<=MAP_BUF_MIDDLE&&sample>MAP_BUF_MIDDLE) || (prev>=MAP_BUF_MIDDLE&&sample<MAP_BUF_MIDDLE))
				++tzCnt;

			prev=sample;
		}
	}
	
	return ((periods*MAP_BUF_LEN*2)<<16)/tzCnt;
}

static LOWERCODESIZE void tuneOffset(int8_t voice,uint8_t nthC)
{
	int8_t i;
	uint16_t estimate,bit;
	uint32_t p,tgtp;

	tgtp=(TUNER_TICK<<(16-nthC))/TUNER_LOWEST_HERTZ;
	
	estimate=0;
	bit=0x8000;

	for(i=0;i<12;++i) // 12bit dac
	{
		estimate|=bit;

		synth_refreshCV(voice,cvCutoff,estimate,1);
		delay_ms(25); // wait analog hardware stabilization	

		p=measureAudioPeriod(2);

		// adjust estimate
		if (p<tgtp)
			estimate&=~bit;

		// on to finer changes
		bit>>=1;
	}

	settings.tunes[nthC][voice]=estimate;

#ifdef DEBUG
	rprintf(0, "cv %d per %d theo %d\n",estimate,p,tgtp);
#endif
}

static LOWERCODESIZE void tuneFilter(int8_t voice)
{
#ifdef DEBUG		
	print("\ntuning ");phex(voice);print("\n");
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
		
		synth_refreshCV(-1,cvResonance,UINT16_MAX,1);
	
		for(v=0;v<SYNTH_VOICE_COUNT;++v)
			synth_refreshCV(v,cvCutoff,UINT16_MAX,1);
	
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