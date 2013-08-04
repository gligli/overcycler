#ifndef DACSPI_H
#define DACSPI_H

#include "synth.h"
#include "dacspi_fiq.h"

void dacspi_init(void);
void dacspi_setState(int channel, int dsidx, uint32_t value);

#endif
