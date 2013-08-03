#ifndef WTOSC_H
#define WTOSC_H

#include "synth.h"

#define WTOSC_MASTER_SAMPLE_RATE 60000
#define WTOSC_MASTER_CLOCK (WTOSC_MASTER_SAMPLE_RATE*SYNTH_OSC_COUNT)
#define WTOSC_CV_OCTAVE (256*12)
#define WTOSC_MAX_SAMPLES 600 // samples

struct wtosc_s
{
	int16_t rawData[WTOSC_MAX_SAMPLES];
	uint16_t resmpData[WTOSC_MAX_SAMPLES];
	
	int16_t rawSampleCount;
	uint16_t increment;

	uint32_t resmpSamplePeriod;
	int16_t resmpSampleCount;

	uint16_t controlData;
	uint16_t cv;
	
	int channel;
};

void wtosc_init(struct wtosc_s * o, int channel, uint16_t controlData);
void wtosc_setSampleData(struct wtosc_s * o, int16_t * data, uint16_t sampleCount);
void wtosc_setParameters(struct wtosc_s * o, uint16_t cv, uint16_t aliasing);
void wtosc_update(struct wtosc_s * o, uint32_t ticks);

#endif
