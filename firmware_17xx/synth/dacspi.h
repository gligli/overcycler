#ifndef DACSPI_H
#define DACSPI_H

#include "synth.h"

#define DACSPI_CHANNEL_COUNT 7
#define DACSPI_STEP_COUNT 4
#define DACSPI_TIMER_MATCH 31

#define DACSPI_TICK_RATE ((DACSPI_TIMER_MATCH+1)*(DACSPI_STEP_COUNT+2)*DACSPI_CHANNEL_COUNT)

#define DACSPI_CMD_SET_A 0x7000
#define DACSPI_CMD_SET_B 0xf000
#define DACSPI_CMD_SET_REF 0x3000

void dacspi_init(void);
void dacspi_setVoiceCommand(int32_t buffer, int voice, int ab, uint16_t command);
void dacspi_sendCVCommand(uint16_t command);

#endif
