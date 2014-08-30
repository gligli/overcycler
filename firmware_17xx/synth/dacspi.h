#ifndef DACSPI_H
#define DACSPI_H

#include "synth.h"

#define DACSPI_CHANNEL_COUNT 8
#define DACSPI_STEP_COUNT 5
#define DACSPI_TIMER_MATCH 20

#define DACSPI_TICK_RATE ((uint32_t)((DACSPI_TIMER_MATCH+1)*((DACSPI_STEP_COUNT+2)*(DACSPI_CHANNEL_COUNT-1)+3)))

#define DACSPI_CMD_SET_A 0x7000
#define DACSPI_CMD_SET_B 0xf000
#define DACSPI_CMD_SET_REF 0x7000

void dacspi_init(void);
void dacspi_setVoiceCommand(int32_t buffer, int voice, int ab, uint16_t command);
void dacspi_setCVCommand(uint16_t command, int channel);

#endif
