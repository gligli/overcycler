#ifndef WTOSC_H
#define WTOSC_H

#include "synth.h"

#define WTOSC_MAX_SAMPLE_RATE 80000
#define WTOSC_CV_SEMITONE 512
#define WTOSC_MAX_SAMPLES 600 // samples

struct wtosc_s
{
	volatile int16_t phase;
	volatile uint16_t increment;
	volatile uint32_t period;
	volatile int16_t sampleCount;

	uint16_t data[WTOSC_MAX_SAMPLES];	
	
	uint16_t cv;
	uint16_t controlData;
};

void wtosc_init(struct wtosc_s * o, uint16_t controlData);
void wtosc_setSampleData(struct wtosc_s * o, int16_t * data, uint16_t sampleCount);
void wtosc_setParameters(struct wtosc_s * o, uint16_t cv, uint16_t aliasing);
uint16_t wtosc_update(struct wtosc_s * o);

#endif
