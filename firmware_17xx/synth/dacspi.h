#ifndef DACSPI_H
#define DACSPI_H

#include "synth.h"

#define DACSPI_CHANNEL_COUNT 7
#define DACSPI_STEP_COUNT 6
#define DACSPI_TIMER_MATCH 17

#define DACSPI_TICK_RATE ((uint32_t)((DACSPI_TIMER_MATCH+1)*((DACSPI_STEP_COUNT+2)*SYNTH_VOICE_COUNT+5.5)))

#define DACSPI_CMD_SET_A 0x7000
#define DACSPI_CMD_SET_B 0xf000
#define DACSPI_CMD_SET_REF 0x7000

void dacspi_init(void);
void dacspi_setVoiceCommand(int32_t buffer, int voice, int ab, uint16_t command);
void dacspi_setCVCommand(uint16_t command, int channel);

#endif
