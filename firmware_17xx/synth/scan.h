#ifndef SCAN_H
#define SCAN_H

#include "synth.h"

#define SCAN_ADC_BITS 10
#define SCAN_ADC_QUANTUM (65536 / (1<<(SCAN_ADC_BITS)))
#define SCAN_POT_COUNT 10

uint16_t scan_getPotValue(int8_t pot);
void scan_resetPotLocking(void);

void scan_init(void);
void scan_update(void);

#endif /* SCAN_H */

