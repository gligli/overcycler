#ifndef WTOSC_H
#define WTOSC_H

#include "synth.h"

#define WTOSC_MASTER_SAMPLE_RATE 60000
#define WTOSC_MASTER_CLOCK (WTOSC_MASTER_SAMPLE_RATE*SYNTH_OSC_COUNT)
#define WTOSC_CV_OCTAVE (256*12)
#define WTOSC_MAX_SAMPLES 600 // samples

struct wtosc_s
{
	uint16_t data[WTOSC_MAX_SAMPLES];	
	
	uint32_t samplePeriod;
	uint16_t controlData;
	uint16_t cv;
	uint16_t increment;
	int16_t sampleCount;
	int channel;
};

void wtosc_init(struct wtosc_s * o, int channel, uint16_t controlData);
void wtosc_setSampleData(struct wtosc_s * o, int16_t * data, uint16_t sampleCount);
void wtosc_setParameters(struct wtosc_s * o, uint16_t cv, uint16_t increment);
void wtosc_update(struct wtosc_s * o, uint32_t ticks);

#endif
