#ifndef DACSPI_H
#define DACSPI_H

#include "synth.h"

#define DACSPI_BUFFER_COUNT 64
#define DACSPI_CV_COUNT 16
#define DACSPI_CHANNEL_COUNT 7
#define DACSPI_OSC_CHANNEL_WAIT_STATES 8
#define DACSPI_CV_CHANNEL_WAIT_STATES 7
#define DACSPI_TIMER_MATCH 24
#define DACSPI_TIME_CONSTANT ((DACSPI_CHANNEL_COUNT-1)*(2+1+DACSPI_OSC_CHANNEL_WAIT_STATES)+(1+1+DACSPI_CV_CHANNEL_WAIT_STATES)) // one tick per channel per DMA access

#define DACSPI_TICK_RATE ((uint32_t)((DACSPI_TIMER_MATCH+1)*DACSPI_TIME_CONSTANT))

#define DACSPI_UPDATE_HZ (SYNTH_MASTER_CLOCK/(DACSPI_BUFFER_COUNT/2*DACSPI_TICK_RATE))

void dacspi_init(void);
void dacspi_setOscValue(int32_t buffer, int channel, uint16_t value); // 16bit value
void dacspi_setCVValue(int channel, uint16_t value, int8_t noDblBuf); // 16bit value

#endif
