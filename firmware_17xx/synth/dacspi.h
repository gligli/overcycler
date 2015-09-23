#ifndef DACSPI_H
#define DACSPI_H

#include "synth.h"

#define DACSPI_CHANNEL_COUNT 7
#define DACSPI_WAIT_STATES_COUNT 35
#define DACSPI_TIMER_MATCH 21
#define DACSPI_TIME_CONSTANT (DACSPI_CHANNEL_COUNT*(2+1+1)+DACSPI_WAIT_STATES_COUNT) // one tick per channel per DMA access

#define DACSPI_TICK_RATE ((uint32_t)((DACSPI_TIMER_MATCH+1)*DACSPI_TIME_CONSTANT))

void dacspi_init(void);
void dacspi_setVoiceValue(int32_t buffer, int voice, int ab, uint16_t value); // 16bit value
void dacspi_setCVValue(uint16_t value, int channel); // 16bit value
// dacspi needs to be able to write the whole GPIO port 0, so all port 0 accesses
// must go through dacspi
void dacspi_port0Set(uint32_t value);
void dacspi_port0Clear(uint32_t value);

#endif
