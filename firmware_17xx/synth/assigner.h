#ifndef ASSIGNER_H
#define	ASSIGNER_H

#include "synth.h"

#define ASSIGNER_NO_NOTE UINT8_MAX

typedef enum
{
	apLast=0,apLow=1,apHigh=2
} assignerPriority_t;

struct assignerAllocation_s
{
	uint32_t timestamp;
	uint16_t velocity;
	uint8_t rootNote;
	uint8_t note;
	int8_t assigned;
};

struct assigner_s
{
	uint8_t noteStates[16]; // 1 bit per note, 128 notes
	struct assignerAllocation_s allocation[SYNTH_VOICE_COUNT];
	uint8_t patternOffsets[SYNTH_VOICE_COUNT];
	assignerPriority_t priority;
	uint8_t voiceMask;
	int8_t mono;
};

extern struct assigner_s assigner;

void assigner_setPriority(struct assigner_s * a, assignerPriority_t prio);
void assigner_setVoiceMask(struct assigner_s * a, uint8_t mask);

int8_t assigner_getAssignment(struct assigner_s * a, int8_t voice, uint8_t * note);
int8_t assigner_getAnyPressed(struct assigner_s * a);
int8_t assigner_getAnyAssigned(struct assigner_s * a);

void assigner_assignNote(struct assigner_s * a, uint8_t note, int8_t gate, uint16_t velocity);
void assigner_voiceDone(struct assigner_s * a, int8_t voice); // -1 -> all voices finished

void assigner_setPattern(struct assigner_s * a, uint8_t * pattern, int8_t mono);
void assigner_getPattern(struct assigner_s * a, uint8_t * pattern, int8_t * mono);
void assigner_setPoly(struct assigner_s * a);
void assigner_latchPattern(struct assigner_s * a);

void assigner_init(struct assigner_s * a);

#endif	/* ASSIGNER_H */

