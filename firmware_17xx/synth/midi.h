#ifndef MIDI_H
#define	MIDI_H

#include "synth.h"

#define MIDI_PORT_UART 0
#define MIDI_PORT_USB 1
#define MIDI_PORT_COUNT 2

void midi_init(void);
void midi_update(void);
void midi_processInput(void);
void midi_newData(int8_t port, uint8_t data);

#endif	/* MIDI_H */

