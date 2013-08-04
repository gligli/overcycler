#ifndef DACSPI_H
#define DACSPI_H

#include "synth.h"

#define DSIDX_WAVEFORM_ADDR 0
#define DSIDX_PHASE 1
#define DSIDX_SAMPLE_COUNT 2
#define DSIDX_INCREMENT 3
#define DSIDX_LAST_SAMPLE 4
#define DSIDX_PERIOD 5
#define DSIDX_DEBUG 7

void dacspi_init(void);
void dacspi_setState(int channel, int dsidx, uint32_t value);

#endif
