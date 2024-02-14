#ifndef MIDI_H
#define	MIDI_H

#include <stdint.h>

typedef enum
{
	mpUART=0,mpUSB=1,
			
	// /!\ this must stay last
	mpCount
} midiPort_t;

void midi_init(void);
void midi_update(void);
void midi_processInput(void);
void midi_newData(midiPort_t port, uint8_t data);

#endif	/* MIDI_H */

