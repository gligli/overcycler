#ifndef WTOSC_H
#define WTOSC_H

#include "synth.h"

#define WTOSC_MAX_SAMPLE_RATE 60000
#define WTOSC_CV_OCTAVE (256*12)
#define WTOSC_MAX_SAMPLES 600 // samples

struct wtosc_s
{
	uint16_t data[WTOSC_MAX_SAMPLES];	
	
	uint16_t cv;
	uint16_t output;
	uint16_t controlData;

	uint32_t period;
	uint16_t increment;
	int16_t phase;
	int16_t sampleCount;
};

void wtosc_init(struct wtosc_s * o, uint16_t controlData);
void wtosc_setSampleData(struct wtosc_s * o, int16_t * data, uint16_t sampleCount);
void wtosc_setParameters(struct wtosc_s * o, uint16_t cv, uint16_t aliasing);
uint16_t wtosc_update(struct wtosc_s * o);

#endif
