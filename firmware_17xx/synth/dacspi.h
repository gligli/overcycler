#ifndef DACSPI_H
#define DACSPI_H

#include "synth.h"

#define DACSPI_CHANNEL_COUNT 7
#define DACSPI_STEP_COUNT_OSC 4
#define DACSPI_STEP_COUNT_CV 10
#define DACSPI_TIMER_MATCH 21
#define DACSPI_TIME_CONSTANT (DACSPI_CHANNEL_COUNT*(2+1+1)+(DACSPI_CHANNEL_COUNT-1)*DACSPI_STEP_COUNT_OSC+DACSPI_STEP_COUNT_CV) // one tick per channel per DMA access

#define DACSPI_TICK_RATE ((uint32_t)((DACSPI_TIMER_MATCH+1)*DACSPI_TIME_CONSTANT))

void dacspi_init(void);
void dacspi_setVoiceValue(int32_t buffer, int voice, int ab, uint16_t value); // 16bit value
void dacspi_setCVValue(uint16_t value, int channel); // 16bit value

#endif
