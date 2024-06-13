#ifndef WTOSC_H
#define WTOSC_H

#include "synth.h"

#define WTOSC_SAMPLE_COUNT 2048 // samples
#define WTOSC_CV_SEMITONE 256
#define WTOSC_HIGHEST_NOTE 120
#define WTOSC_SAMPLES_GUARD_BAND 4600 // about -1.3 decibels

struct wtosc_s
{
	uint16_t * mainData;
	uint16_t * crossoverData;
	
	int32_t period[2],pendingPeriod[2]; // one per waveform half
	int32_t increment[2],pendingIncrement[2];
	
	int32_t counter;
	int32_t phase;

	int32_t curSample,prevSample,prevSample2,prevSample3;
	
	uint32_t pitch;
	uint32_t crossover;
	uint32_t width;
	int32_t aliasing;
	
	int32_t channel;
	int8_t pendingUpdate;
};

typedef enum
{
	osmNone, osmMaster, osmSlave
} oscSyncMode_t;

void wtosc_init(struct wtosc_s * o, int32_t channel);
// data must be persistent and be filled with values in the range
// WTOSC_SAMPLES_GUARD_BAND..65535-WTOSC_SAMPLES_GUARD_BAND
// this is because hermite interpolation will overshoot on sharp transitions
void wtosc_setSampleData(struct wtosc_s * o, uint16_t * mainData, uint16_t * xovrData);
void wtosc_setParameters(struct wtosc_s * o, uint16_t pitch, uint16_t aliasing, uint16_t width, uint16_t crossover);
void wtosc_update(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer, oscSyncMode_t syncMode, uint32_t * syncResets);

#endif
