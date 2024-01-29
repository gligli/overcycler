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
#define TUNER_FIL_NTH_C_LO 4
#define TUNER_FIL_NTH_C_HI 7

static uint16_t extrapolateUpperOctavesTunes(int8_t voice, int8_t oct)
{
	uint32_t v;
	
	v=settings.tunes[TUNER_OCTAVE_COUNT-1][voice]-settings.tunes[TUNER_OCTAVE_COUNT-2][voice];
	v*=oct-TUNER_OCTAVE_COUNT+1;
	v+=settings.tunes[TUNER_OCTAVE_COUNT-1][voice];
	
	return MIN(v,UINT16_MAX);
}

static void waitCVUpdate(void)
{
	delay_us(500); // enough for dacspi to output CVs to DACs
}

LOWERCODESIZE static void prepareSynth(void)
{
	// display	
	for(int spc=0;spc<40;++spc)
		rprintf(1," ");
	
	synth_refreshCV(-1,cvResonance,0);
	synth_refreshCV(-1,cvNoiseVol,0);
	synth_refreshCV(-1,cvAVol,0);
	synth_refreshCV(-1,cvBVol,0);
	
	waitCVUpdate();
}

static NOINLINE double measureAudioPeriod(uint8_t periods) // in TUNER_TICK ticks
{
	uint32_t tzCnt=0;
		
#define MAP_BUF_LEN 800
	uint16_t buf[MAP_BUF_LEN];
	uint16_t prev;
	
	for(uint8_t p=0;p<periods;++p)
	{
		scan_sampleMasterMix(MAP_BUF_LEN,buf);

		prev=0;
		for(uint16_t pos=0;pos<MAP_BUF_LEN;++pos)
		{
			uint16_t sample=buf[pos];
			
			if (prev<=HALF_RANGE&&sample>=HALF_RANGE)
				++tzCnt;

			prev=sample;
		}
	}
	
	return (double)periods*MAP_BUF_LEN/tzCnt;
}

static LOWERCODESIZE int8_t tuneOffset(int8_t voice,uint8_t nthC)
{
	int8_t i;
	uint16_t estimate,bit;
	double p,tgtp;

	tgtp=TUNER_TICK/(TUNER_LOWEST_HERTZ*pow(2.0,nthC));
	
	estimate=UINT16_MAX;
	bit=0x8000;
	
	for(i=0;i<12;++i) // 12bit dac
	{
		synth_refreshCV(voice,cvCutoff,estimate);
		delay_ms(25); // wait analog hardware stabilization	

		p=measureAudioPeriod(nthC);

		// adjust estimate
		if (p>tgtp)
			estimate+=bit;
		else
			estimate-=bit;

		// on to finer changes
		bit>>=1;
		
	}

	settings.tunes[nthC][voice]=estimate;

#ifdef DEBUG
	rprintf(0, "cv %d per %d %d\n",estimate,(int)p,(int)tgtp);
#endif
	
	return 0;
}

static LOWERCODESIZE void tuneCV(int8_t voice)
{
#ifdef DEBUG		
	print("\ntuning ");phex(voice);print("\n");
#endif
	int8_t i;
	
	// display
	
	rprintf(1,"Fil%d ",voice+1);

	// open VCA

	synth_refreshCV(voice,cvAmp,UINT16_MAX);
	
	// tune

	for(i=TUNER_FIL_NTH_C_LO;i<=TUNER_FIL_NTH_C_HI;++i)
		if (tuneOffset(voice,i))
			break;

	for(i=TUNER_FIL_NTH_C_LO-1;i>=0;--i)
		settings.tunes[i][voice]=(uint32_t)2*settings.tunes[i+1][voice]-settings.tunes[i+2][voice];

	for(i=TUNER_FIL_NTH_C_HI+1;i<TUNER_OCTAVE_COUNT;++i)
		settings.tunes[i][voice]=(uint32_t)2*settings.tunes[i-1][voice]-settings.tunes[i-2][voice];
	
	// close VCA

	synth_refreshCV(voice,cvAmp,0);
	waitCVUpdate();
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
	int8_t i,j;
	
	// theoretical base tuning
	
	for(j=0;j<TUNER_OCTAVE_COUNT;++j)
		for(i=0;i<TUNER_CV_COUNT;++i)
		{
			settings.tunes[j][i]=TUNER_FIL_INIT_OFFSET+j*TUNER_FIL_INIT_SCALE;
		}
}

LOWERCODESIZE void tuner_tuneSynth(void)
{
	int8_t i;
	
	BLOCK_INT(1)
	{
		// reinit tuner
		
		tuner_init();
		scan_init(1);
		
		// prepare synth for tuning
		
		prepareSynth();
		
		// tune filters
			
			// init
		
		synth_refreshCV(-1,cvResonance,UINT16_MAX);
	
		for(i=0;i<SYNTH_VOICE_COUNT;++i)
			synth_refreshCV(i,cvCutoff,0);
	
			// filters
		
		for(i=0;i<SYNTH_VOICE_COUNT;++i)
			tuneCV(i);

		// finish
		
		synth_refreshCV(-1,cvResonance,0);
		for(i=0;i<SYNTH_VOICE_COUNT;++i)
			synth_refreshCV(i,cvAmp,0);
		
		scan_init(0);
		settings_save();
	}
}