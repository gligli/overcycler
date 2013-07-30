#ifndef DACSPI_H
#define DACSPI_H

#include "synth.h"

#define DSIDX_SAMPLE_COUNT 0
#define DSIDX_WAVEFORM_ADDR 1
#define DSIDX_COUNTER 2
#define DSIDX_PERIOD 3
#define DSIDX_PHASE 4
#define DSIDX_INCREMENT 5

void dacspi_init(void);
void dacspi_setState(int channel, int dsidx, uint32_t value);

#endif
