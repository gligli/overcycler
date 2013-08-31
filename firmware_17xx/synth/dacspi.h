#ifndef DACSPI_H
#define DACSPI_H

#include "synth.h"

#define DACSPI_CHANNEL_COUNT 7
#define DACSPI_CV_CHANNEL (DACSPI_CHANNEL_COUNT-1)

#define DACSPI_CMD_SET_A 0x7000
#define DACSPI_CMD_SET_B 0xf000
#define DACSPI_CMD_SET_REF 0x3000

void dacspi_init(void);
void dacspi_sendCommand(uint8_t channel, uint16_t commandA, uint16_t commandB);

#endif
