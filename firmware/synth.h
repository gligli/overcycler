#ifndef SYNTH_H
#define SYNTH_H

#include "main.h"

#define SYNTH_VOICE_COUNT 2
#define SYNTH_OSC_COUNT (SYNTH_VOICE_COUNT*2)
#define SYNTH_MASTER_CLOCK CPU_CLOCK


void synth_init(void);
void synth_update(void);

#endif
