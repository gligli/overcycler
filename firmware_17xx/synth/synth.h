#ifndef SYNTH_H
#define SYNTH_H

#include "main.h"
#include "utils.h"

#define SYNTH_VOICE_COUNT 6
#define SYNTH_OSC_COUNT (SYNTH_VOICE_COUNT*2)
//#define SYNTH_MASTER_CLOCK SystemCoreClock
#define SYNTH_MASTER_CLOCK 120000000

void synth_init(void);
void synth_update(void);

#endif
