#ifndef WTOSC_H
#define WTOSC_H

#include "synth.h"

#define WTOSC_CV_SEMITONE 256
#define WTOSC_MAX_SAMPLES 600 // samples

struct wtosc_s
{
	uint16_t data[WTOSC_MAX_SAMPLES];	

	int32_t period;
	int32_t phase;
	int32_t increment;
	int32_t sampleCount;

	int32_t counter;

	uint16_t curSample;
	uint16_t prevSample;

	uint16_t cv;
	uint16_t aliasing;
	uint16_t controlData;

};

void wtosc_init(struct wtosc_s * o, uint16_t controlData);
void wtosc_setSampleData(struct wtosc_s * o, int16_t * data, uint16_t sampleCount);
void wtosc_setParameters(struct wtosc_s * o, uint16_t cv, uint16_t aliasing);
uint16_t wtosc_update(struct wtosc_s * o, int32_t elapsedTicks);

#endif
