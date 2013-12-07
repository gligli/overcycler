#ifndef WTOSC_H
#define WTOSC_H

#include "synth.h"

#define WTOSC_MAX_SAMPLES 600 // samples
#define WTOSC_CV_SEMITONE 256
#define WTOSC_HIGHEST_NOTE 127

struct wtosc_s
{
	uint16_t data[WTOSC_MAX_SAMPLES];	

	int32_t period[2]; // one per waveform half
	int32_t increment[2];
	
	int32_t counter;
	int32_t phase;
	int32_t sampleCount;
	int32_t halfSampleCount;

	uint32_t curSample;
	uint32_t prevSample;

	uint32_t cv;
	uint32_t aliasing;
	uint32_t width;
	
	int voice;
	int ab;
};

void wtosc_init(struct wtosc_s * o, int8_t voice, int8_t ab);
void wtosc_setSampleData(struct wtosc_s * o, int16_t * data, uint16_t sampleCount);
void wtosc_setParameters(struct wtosc_s * o, uint16_t cv, uint16_t aliasing, uint16_t width);
void wtosc_update(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer);

#endif
