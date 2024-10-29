#ifndef WTOSC_H
#define WTOSC_H

#include "synth.h"

#define WTOSC_SAMPLE_COUNT 2400 // samples
#define WTOSC_CV_SEMITONE 256
#define WTOSC_HIGHEST_NOTE 108
#define WTOSC_SAMPLES_GUARD_BAND 4600 // about -1.3 decibels

typedef enum
{
	wmOff=0,wmAliasing=1,wmWidth=2,wmFrequency=3,wmCrossOver=4,wmFolder=5,wmBitCrush=6,

	// /!\ this must stay last
	wmCount
} oscWModTarget_t;

struct wtosc_s
{
	uint16_t * mainData;
	uint16_t * crossoverData;
	
	int32_t period[2],pendingPeriod[2]; // one per waveform half
	int32_t increment[2],pendingIncrement[2];
	
	int32_t counter;
	int32_t phase;

	int32_t curSample,prevSample,prevSample2,prevSample3;
	
	int32_t aliasing;
	int32_t folder;
	int32_t bitcrush;
	uint16_t pitch;
	uint16_t width;
	uint16_t crossover;
	
	oscWModTarget_t wmType;
	int8_t channel;
	int8_t pendingUpdate;
};

typedef enum
{
	osmNone, osmMaster, osmSlave
} oscSyncMode_t;

void wtosc_init(struct wtosc_s * o, int8_t channel);
// data must be persistent and be filled with values in the range
// WTOSC_SAMPLES_GUARD_BAND..65535-WTOSC_SAMPLES_GUARD_BAND
// this is because hermite interpolation will overshoot on sharp transitions
void wtosc_setSampleData(struct wtosc_s * o, uint16_t * mainData, uint16_t * xovrData);
void wtosc_setParameters(struct wtosc_s * o, uint16_t pitch, oscWModTarget_t wmType, uint16_t wmAmount);
void wtosc_update(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer, oscSyncMode_t syncMode, int16_t *syncPositions);

#endif
