#ifndef SYNTH_H
#define SYNTH_H

#include "main.h"
#include "utils.h"

#define SYNTH_VOICE_COUNT 6
//#define SYNTH_MASTER_CLOCK SystemCoreClock
#define SYNTH_MASTER_CLOCK 120000000

void synth_init(void);
void synth_update(void);
void synth_updateDACs(void);

extern int dbg1,dbg2;

#endif
