////////////////////////////////////////////////////////////////////////////////
// Tunes CVs using the 8253 timer, by measuring audio period
////////////////////////////////////////////////////////////////////////////////

#include "tuner.h"
#include "storage.h"
#include "synth.h"
#include "storage.h"

#define TUNER_MIDDLE_C_HERTZ 261.63
#define TUNER_LOWEST_HERTZ (TUNER_MIDDLE_C_HERTZ/16)

static struct
{
	cv_t currentCV;
} tuner;

static uint16_t extapolateUpperOctavesTunes(int8_t voice, int8_t oct)
{
	uint32_t v;
	
	v=settings.tunes[TUNER_OCTAVE_COUNT-1][voice]-settings.tunes[TUNER_OCTAVE_COUNT-2][voice];
	v*=oct-TUNER_OCTAVE_COUNT+1;
	v+=settings.tunes[TUNER_OCTAVE_COUNT-1][voice];
	
	return MIN(v,UINT16_MAX);
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
			loVal=extapolateUpperOctavesTunes(voice,loOct);

		if(hiOct<TUNER_OCTAVE_COUNT)
			hiVal=settings.tunes[hiOct][voice];
		else
			hiVal=extapolateUpperOctavesTunes(voice,hiOct);

		semiTone=(((uint32_t)(note%12)<<16)+((uint16_t)nextInterp<<8))/12;

		value=loVal;
		value+=(semiTone*(hiVal-loVal))>>16;
	}

	return value;
}

LOWERCODESIZE void tuner_init(void)
{
	int8_t i,j;
	
	memset(&tuner,0,sizeof(tuner));
	
	// theoretical base tuning
	
	for(j=0;j<TUNER_OCTAVE_COUNT;++j)
		for(i=0;i<TUNER_CV_COUNT;++i)
		{
			settings.tunes[j][i]=TUNER_FIL_INIT_OFFSET+j*TUNER_FIL_INIT_SCALE;
		}
}

LOWERCODESIZE void tuner_tuneSynth(void)
{
	//TODO
}
