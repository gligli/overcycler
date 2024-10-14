////////////////////////////////////////////////////////////////////////////////
// Voice assigner
////////////////////////////////////////////////////////////////////////////////

#include "assigner.h"

struct allocation_s
{
	uint32_t timestamp;
	uint16_t velocity;
	uint8_t rootNote;
	uint8_t note;
	int8_t allocated;
	int8_t gated;
	int8_t keyPressed;
	int8_t fromKeyboard;
};

static struct
{
	uint32_t noteTimestamps[ASSIGNER_NOTE_COUNT]; // UINT32_MAX if not gated
	uint16_t noteVelocities[ASSIGNER_NOTE_COUNT];
	struct allocation_s allocation[SYNTH_VOICE_COUNT];
	uint8_t patternOffsets[SYNTH_VOICE_COUNT];
	assignerPriority_t priority;
	uint8_t voiceMask;
	int8_t mono;
	int8_t hold;
} assigner;

static const uint8_t polyPattern[SYNTH_VOICE_COUNT]={0,ASSIGNER_NO_NOTE,ASSIGNER_NO_NOTE,ASSIGNER_NO_NOTE,ASSIGNER_NO_NOTE,ASSIGNER_NO_NOTE};	

static inline void setNoteState(uint8_t note, int8_t gate, uint16_t velocity, uint32_t timestamp)
{
	assigner.noteVelocities[note]=velocity;
	if(gate)
		assigner.noteTimestamps[note]=timestamp;
	else
		assigner.noteTimestamps[note]=UINT32_MAX;
}

static inline int8_t getNoteState(uint8_t note, uint16_t *velocity, uint32_t *timestamp)
{
	int8_t gate=assigner.noteTimestamps[note]!=UINT32_MAX;
	if(gate && timestamp)
		*timestamp=assigner.noteTimestamps[note];
	if(gate && velocity)
		*velocity=assigner.noteVelocities[note];
	return gate;
}

static inline int8_t isVoiceDisabled(int8_t voice)
{
	return !(assigner.voiceMask&(1<<voice));
}

static inline int8_t getNoteAllocation(uint8_t note)
{
	int8_t vi,v=-1;

	for(vi=0;vi<SYNTH_VOICE_COUNT;++vi)
	{
		if(isVoiceDisabled(vi))
			continue;

		if(assigner.allocation[vi].allocated && assigner.allocation[vi].rootNote==note)
		{
			v=vi;
			break;
		}
	}

	return v;
}

static inline int8_t getAvailableVoice(uint8_t note, uint32_t timestamp)
{
	int8_t v,oldestVoice=-1,sameNote=-1;
	uint32_t oldestTimestamp=UINT32_MAX;

	for(v=0;v<SYNTH_VOICE_COUNT;++v)
	{
		// never assign a disabled voice
		
		if(isVoiceDisabled(v))
			continue;
		
		if(assigner.allocation[v].allocated)
		{
			// triggering a note that is still allocated to a voice should use this voice
		
			if(assigner.allocation[v].timestamp<timestamp && assigner.allocation[v].note==note)
			{
				sameNote=v;
				break;
			}
		}
		else
		{
			// else use oldest voice, if there is one

			if (assigner.allocation[v].timestamp<oldestTimestamp)
			{
				oldestTimestamp=assigner.allocation[v].timestamp;
				oldestVoice=v;
			}
		}
	}
	
	if(sameNote>=0)
		return sameNote;
	else
		return oldestVoice;
}

static inline int8_t getDispensableVoice(uint8_t note)
{
	int8_t v,res=-1;
	uint32_t ts;

	// first pass, steal oldest released voice
	
	ts=UINT32_MAX;
		
	for(v=0;v<SYNTH_VOICE_COUNT;++v)
	{
		if(isVoiceDisabled(v))
			continue;
		
		if(!getNoteState(assigner.allocation[v].rootNote,NULL,NULL) && assigner.allocation[v].timestamp<ts)
		{
			ts=assigner.allocation[v].timestamp;
			res=v;
		}
	}
	
	if(res>=0)
		return res;
	
	// second pass, use priority rules to steal the less important held note
	
	ts=UINT32_MAX;
		
	for(v=0;v<SYNTH_VOICE_COUNT;++v)
	{
		if(isVoiceDisabled(v))
			continue;
		
		switch(assigner.priority)
		{
		case apLast:
			if(assigner.allocation[v].timestamp<ts)
			{
				res=v;
				ts=assigner.allocation[v].timestamp;
			}
			break;
		case apLow:
			if(assigner.allocation[v].note>note)
			{
				res=v;
				note=assigner.allocation[v].note;
			}
			break;
		case apHigh:
			if(assigner.allocation[v].note<note)
			{
				res=v;
				note=assigner.allocation[v].note;
			}
			break;
		}
	}
	
	return res;
}
	
void assigner_voiceDone(int8_t voice)
{
	if (voice<0||voice>SYNTH_VOICE_COUNT)
		return;

	assigner.allocation[voice].allocated=0;
	assigner.allocation[voice].keyPressed=0;
	assigner.allocation[voice].note=ASSIGNER_NO_NOTE;
	assigner.allocation[voice].rootNote=ASSIGNER_NO_NOTE;
}

LOWERCODESIZE static void voicesDone(void)
{
	int8_t v;
	for(v=0;v<SYNTH_VOICE_COUNT;++v)
	{
		if(isVoiceDisabled(v))
			continue;
		
		assigner_voiceDone(v);
		assigner.allocation[v].timestamp=0; // reset to voice 0 in case all voices stopped at once
	}
	// Release all keys and future holds too. This avoids potential
	// problems with notes seemingly popping up from nowhere due to
	// reassignment when future keys are released.
    memset(assigner.noteTimestamps,UINT32_MAX,sizeof(assigner.noteTimestamps));
}

// This is different from voicesDone() in that it does not silence
// the voice immediately but lets it go through its release phase as usual.
// Also, only voices corresponding to keys that are down on the keyboard
// are released.
void assigner_allKeysOff(void)
{
	int8_t v;
	for(v=0;v<SYNTH_VOICE_COUNT;++v)
	{
		if (!isVoiceDisabled(v) && assigner.allocation[v].gated &&
		    assigner.allocation[v].fromKeyboard)
		{
			synth_assignerEvent(assigner.allocation[v].note,0,v,assigner.allocation[v].velocity,0);
		    	assigner.allocation[v].gated=0;
		    	assigner.allocation[v].keyPressed=0;
		}
	}
    memset(assigner.noteTimestamps,UINT32_MAX,sizeof(assigner.noteTimestamps));
	assigner.hold=0;
}

void assigner_panicOff(void)
{
    voicesDone();
    assigner_init();
}

void assigner_setPriority(assignerPriority_t prio)
{
	if(prio==assigner.priority)
		return;
	
	voicesDone();
	
	if(prio>2)
		prio=0;
	
	assigner.priority=prio;
}

void assigner_setVoiceMask(uint8_t mask)
{
	if(mask==assigner.voiceMask)
		return;
	
	voicesDone();
	assigner.voiceMask=mask;
}

FORCEINLINE int8_t assigner_getAssignment(int8_t voice, uint8_t * note)
{
	int8_t a;
	
	a=assigner.allocation[voice].allocated;
	
	if(a && note)
		*note=assigner.allocation[voice].note;
	
	return a;
}

int8_t assigner_getAnyPressed(void)
{
	for(uint8_t n=0;n<ASSIGNER_NOTE_COUNT;++n)
		if(getNoteState(n,NULL,NULL))
			return 1;
	
	return 0;
}

int8_t assigner_getAnyAssigned(void)
{
	int8_t v;
	int8_t f=0;
	
	for(v=0;v<SYNTH_VOICE_COUNT;++v)
		if(!isVoiceDisabled(v))
			f|=assigner.allocation[v].allocated;
	
	return f!=0;
}

int8_t FORCEINLINE assigner_getMono(void)
{
	return assigner.mono;
}

void assigner_assignNote(uint8_t note, int8_t gate, uint16_t velocity, int8_t fromKeyboard)
{
	uint32_t timestamp,restoredTimestamp,ts;
	uint16_t restoredVelocity,vel;
	uint8_t restoredNote;
	uint8_t ni,n,flags=0;
	int8_t v,vi;

	timestamp=currentTick;

	setNoteState(note,gate,velocity,timestamp);

reassign:

	if(gate)
	{
		if(assigner.mono)
		{
			// just handle legato & priority
			
			v=0;

			if(assigner.priority!=apLast)
				for(n=0;n<ASSIGNER_NOTE_COUNT;++n)
					if(n!=note && getNoteState(n,NULL,NULL))
					{
						if (note>n && assigner.priority==apLow)
							return;
						if (note<n && assigner.priority==apHigh)
							return;
						
						flags=ASSIGNER_EVENT_FLAG_LEGATO;
					}
		}
		else
		{
			// first, try to get a free voice
			v=getAvailableVoice(note,timestamp);

			// no free voice, try to steal one
			if(v<0)
				v=getDispensableVoice(note);

			// we might still have no voice
			if(v<0)
				return;
		}

		// try to assign the whole pattern of notes

		for(vi=0;vi<SYNTH_VOICE_COUNT;++vi)
		{
			if(assigner.patternOffsets[vi]==ASSIGNER_NO_NOTE)
				break;

			n=note+assigner.patternOffsets[vi];

			assigner.allocation[v].allocated=1;
			assigner.allocation[v].gated=1;
			assigner.allocation[v].keyPressed=1;
			assigner.allocation[v].velocity=velocity;
			assigner.allocation[v].rootNote=note;
			assigner.allocation[v].note=n;
			assigner.allocation[v].timestamp=timestamp;
			assigner.allocation[v].fromKeyboard=fromKeyboard;

			synth_assignerEvent(n,1,v,velocity,flags);

			do
				v=(v+1)%SYNTH_VOICE_COUNT;
			while(isVoiceDisabled(v));
		}
	}
	else if(getNoteAllocation(note)>=0) // note not allocated -> nothing to do
	{
		restoredNote=ASSIGNER_NO_NOTE;

		// some still triggered notes might have been stolen, find them

		if(assigner.priority==apLast)
		{
			restoredTimestamp=0;
			for(n=0;n<ASSIGNER_NOTE_COUNT;++n)
			{
				if(getNoteState(n,&vel,&ts) && ts>restoredTimestamp && getNoteAllocation(n)<0)
				{
					restoredNote=n;
					restoredVelocity=vel;
					restoredTimestamp=ts;
				}
			}
		}
		else
		{
			for(ni=0;ni<ASSIGNER_NOTE_COUNT;++ni)
			{
				if(assigner.priority==apHigh)
					n=127-ni;
				else // apLow
					n=ni;

				if(getNoteState(n,&vel,NULL) && getNoteAllocation(n)<0)
				{
					restoredNote=n;
					restoredVelocity=vel;
					break;
				}
			}
		}
		
		if(restoredNote==ASSIGNER_NO_NOTE)
		{
			// no note to restore, gate off all voices with rootNote=note
			
			for(v=0;v<SYNTH_VOICE_COUNT;++v)
			{
				if(isVoiceDisabled(v))
					continue;
				
				if(assigner.allocation[v].allocated && assigner.allocation[v].rootNote==note)
				{
					assigner.allocation[v].keyPressed=0;
					if(!assigner.hold)
					{
						assigner.allocation[v].gated=0;
						synth_assignerEvent(assigner.allocation[v].note,0,v,velocity,0);
					}
				}
			}
		}
		else
		{
			// restored notes can be assigned again
			
			note=restoredNote;
			velocity=restoredVelocity;
			gate=1;
			flags=ASSIGNER_EVENT_FLAG_LEGATO;
			
			goto reassign;
		}
	}
}

LOWERCODESIZE void assigner_setPattern(const uint8_t * pattern, int8_t mono)
{
	int8_t i,count=0;
	
	if(mono==assigner.mono && !memcmp(pattern,&assigner.patternOffsets[0],SYNTH_VOICE_COUNT))
		return;

        assigner_allKeysOff();
	
	assigner.mono=mono;
	memset(&assigner.patternOffsets[0],ASSIGNER_NO_NOTE,SYNTH_VOICE_COUNT);

	for(i=0;i<SYNTH_VOICE_COUNT;++i)
	{
		if(pattern[i]==ASSIGNER_NO_NOTE)
			break;
		
		assigner.patternOffsets[i]=pattern[i];
		++count;
	}

	if(count>0)
	{
		assigner.patternOffsets[0]=0; // root note always has offset 0
	}
	else
	{
		// empty pattern means unison
		memset(&assigner.patternOffsets[0],0,SYNTH_VOICE_COUNT);
	}
}

void assigner_getPattern(uint8_t * pattern, int8_t * mono)
{
	memcpy(pattern,assigner.patternOffsets,SYNTH_VOICE_COUNT);
	
	if(mono!=NULL)
		*mono=assigner.mono;
}

LOWERCODESIZE void assigner_latchPattern(void)
{
	uint8_t n;
	int8_t count;
	uint8_t pattern[SYNTH_VOICE_COUNT];	
	count=0;
	
	memset(pattern,ASSIGNER_NO_NOTE,SYNTH_VOICE_COUNT);
	
	for(n=0;n<ASSIGNER_NOTE_COUNT;++n)
		if(getNoteState(n,NULL,NULL))
		{
			pattern[count]=n;
			
			if(count>0)
				pattern[count]-=pattern[0]; // it's a list of offsets to the root note
						
			++count;
			
			if(count>=SYNTH_VOICE_COUNT)
				break;
		}

	assigner_setPattern(pattern,1);
}

LOWERCODESIZE void assigner_setPoly(void)
{
	assigner_setPattern(polyPattern,0);
}

void assigner_holdEvent(int8_t hold)
{
	int8_t v;

	if (hold) {
		assigner.hold=1;
		return;
	}

	assigner.hold=0;
	// Send gate off to all voices whose corresponding key is up
	for(v=0;v<SYNTH_VOICE_COUNT;++v) {
		if (!isVoiceDisabled(v) && 
		    assigner.allocation[v].gated &&
		    !assigner.allocation[v].keyPressed) {
			synth_assignerEvent(assigner.allocation[v].note,0,v,assigner.allocation[v].velocity,0);
		    	assigner.allocation[v].gated=0;
		}
	}
}

void assigner_init(void)
{
	memset(&assigner,0,sizeof(assigner));
    memset(assigner.noteTimestamps,UINT32_MAX,sizeof(assigner.noteTimestamps));

	assigner.voiceMask=(1<<SYNTH_VOICE_COUNT)-1;
	memset(&assigner.patternOffsets[0],ASSIGNER_NO_NOTE,SYNTH_VOICE_COUNT);
	assigner.patternOffsets[0]=0;
}

