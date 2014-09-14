#ifndef DACSPI_H
#define DACSPI_H

#include "synth.h"

#define DACSPI_CHANNEL_COUNT 8
#define DACSPI_STEP_COUNT 8
#define DACSPI_TIMER_MATCH 18
#define DACSPI_TIME_CONSTANT 64.5 // ideally should be calculated from other constants; adjust to get proper tune

#define DACSPI_TICK_RATE ((uint32_t)((DACSPI_TIMER_MATCH+1)*DACSPI_TIME_CONSTANT))

#define DACSPI_CMD_SET_A 0x7000
#define DACSPI_CMD_SET_B 0xf000
#define DACSPI_CMD_SET_REF 0x7000

void dacspi_init(void);
void dacspi_setVoiceCommand(int32_t buffer, int voice, int ab, uint16_t command);
void dacspi_setCVValue(uint16_t command, int channel);

#endif
