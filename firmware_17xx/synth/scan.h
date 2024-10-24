#ifndef SCAN_H
#define SCAN_H

#include "synth.h"

#define SCAN_POT_COUNT 10
#define SCAN_POT_MAX_VALUE 999
#define SCAN_POT_DEAD_ZONE 512
#define SCAN_MASTERMIX_SAMPLERATE 10000

enum scanKeypadButton_e
{
	kb0=0,kb1,kb2,kb3,kb4,kb5,kb6,kb7,kb8,kb9,
	kbA,kbB,kbC,kbD,
	kbSharp,kbAsterisk,
	
	// /!\ this must stay last
	kbCount
};

typedef void (*scan_event_callback_t)(int8_t source); // source: keypad (kb0..kbAsterisk) / potentiometer (-1..-10)

uint16_t scan_getPotValue(int8_t pot);
void scan_resetPotLocking(void);
void scan_setMode(int8_t isSmpMasterMixMode);
void scan_sampleMasterMix(uint16_t sampleCount, uint16_t * buffer);
void scan_setScanEventCallback(scan_event_callback_t callback);

int scan_potTo16bits(int x);
int scan_potFrom16bits(int x);

void scan_init(void);
void scan_update(void);

#endif /* SCAN_H */

