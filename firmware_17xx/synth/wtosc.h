#ifndef WTOSC_H
#define WTOSC_H

#include "synth.h"

#define WTOSC_MAX_SAMPLES 600 // samples
#define WTOSC_CV_SEMITONE 256
#define WTOSC_HIGHEST_NOTE 127

struct wtosc_s
{
	uint16_t data[WTOSC_MAX_SAMPLES];	

	int32_t period;
	int32_t increment;
	int32_t sampleCount;
	
	int32_t counter;
	int32_t phase;

	uint16_t curSample;
	uint16_t prevSample;

	uint16_t cv;
	uint16_t aliasing;
	
	int8_t voice;
	int8_t ab;
};

void wtosc_init(struct wtosc_s * o, int8_t voice, int8_t ab);
void wtosc_setSampleData(struct wtosc_s * o, int16_t * data, uint16_t sampleCount);
void wtosc_setParameters(struct wtosc_s * o, uint16_t cv, uint16_t aliasing);
void wtosc_update(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer);

#endif
