#ifndef WTOSC_H
#define WTOSC_H

#include "synth.h"

#define WTOSC_MAX_SAMPLE_RATE 90000
#define WTOSC_CV_OCTAVE (256*12)
#define WTOSC_MAX_SAMPLES 600 // samples

struct wtosc_s
{
	uint16_t data[WTOSC_MAX_SAMPLES];	
	
	uint16_t cv;

	volatile uint32_t period;
	volatile uint16_t controlData;
	volatile uint16_t increment;
	volatile int16_t phase;
	volatile int16_t sampleCount;
	volatile int8_t channel;
};

void wtosc_init(struct wtosc_s * o, int8_t channel, uint16_t controlData);
void wtosc_setSampleData(struct wtosc_s * o, int16_t * data, uint16_t sampleCount);
void wtosc_setParameters(struct wtosc_s * o, uint16_t cv, uint16_t aliasing);
uint32_t wtosc_update(struct wtosc_s * o);

#endif
