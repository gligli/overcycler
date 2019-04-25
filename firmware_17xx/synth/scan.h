#ifndef SCAN_H
#define SCAN_H

#include "synth.h"

#define POT_COUNT 10

uint16_t scan_getPotValue(int8_t pot);
void scan_resetPotLocking(void);

void scan_init(void);
void scan_update(void);

#endif /* SCAN_H */

