#ifndef DACSPI_H
#define DACSPI_H

#include "synth.h"

#define DACSPI_CHANNEL_COUNT 8
#define DACSPI_STEP_COUNT 8
#define DACSPI_TIMER_MATCH 15
#define DACSPI_TIME_CONSTANT (7.5*(DACSPI_STEP_COUNT+2)+2.5) // one tick per channel per DMA access

#define DACSPI_TICK_RATE ((uint32_t)((DACSPI_TIMER_MATCH+1)*DACSPI_TIME_CONSTANT))

void dacspi_init(void);
void dacspi_setVoiceValue(int32_t buffer, int voice, int ab, uint16_t value); // 16bit value
void dacspi_setCVValue(uint16_t value, int channel); // 16bit value

#endif
