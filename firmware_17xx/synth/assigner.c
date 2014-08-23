////////////////////////////////////////////////////////////////////////////////
// Voice assigner
////////////////////////////////////////////////////////////////////////////////

#include "assigner.h"

struct assigner_s assigner[SYNTH_PART_COUNT];

static const uint8_t bit2mask[8] = {1,2,4,8,16,32,64,128};

static inline void setNoteState(struct assigner_s * a, uint8_t note, int8_t gate)
{
	uint8_t *bf,mask;
	
	bf=&a->noteStates[note>>3];
	mask=bit2mask[note&7];
	
	if(gate)	
		*bf|=mask;
	else
		*bf&=~mask;
}

static inline int8_t getNoteState(struct assigner_s * a, uint8_t note)
{
	uint8_t bf,mask;
	
	bf=a->noteStates[note>>3];
	
	if(!bf)
		return 0;
	
	mask=bit2mask[note&7];
	
	return (bf&mask)!=0;
}

static inline int8_t isVoiceDisabled(struct assigner_s * a, int8_t voice)
{
	return !(a->voiceMask&bit2mask[voice]);
}

static inline int8_t getAvailableVoice(struct assigner_s * a, uint8_t note, uint32_t timestamp)
{
	int8_t v,sameNote=-1,firstFree=-1;

	for(v=0;v<SYNTH_VOICE_COUNT;++v)
	{
		// never assign a disabled voice
		
		if(isVoiceDisabled(a,v))
			continue;
		
		if(a->allocation[v].assigned)
		{
			// triggering a note that is still allocated to a voice should use this voice
		
			if(a->allocation[v].timestamp<timestamp && a->allocation[v].note==note)
			{
				sameNote=v;
				break;
			}
		}
		else
		{
			// else use first free voice, if there's one
			
			if(firstFree<0)
				firstFree=v;
		}
	}
	
	if(sameNote>=0)
		return sameNote;
	else
		return firstFree;
}

static inline int8_t getDispensableVoice(struct assigner_s * a, uint8_t note)
{
	int8_t v,res=-1;
	uint32_t ts;

	// first pass, steal oldest released voice
	
	ts=UINT32_MAX;
		
	for(v=0;v<SYNTH_VOICE_COUNT;++v)
	{
		if(isVoiceDisabled(a,v))
			continue;
		
		if(!getNoteState(a,a->allocation[v].rootNote) && a->allocation[v].timestamp<ts)
		{
			ts=a->allocation[v].timestamp;
			res=v;
		}
	}
	
	if(res>=0)
		return res;
	
	// second pass, use priority rules to steal the less important held note
	
	ts=UINT32_MAX;
		
	for(v=0;v<SYNTH_VOICE_COUNT;++v)
	{
		if(isVoiceDisabled(a,v))
			continue;
		
		switch(a->priority)
		{
		case apLast:
			if(a->allocation[v].timestamp<ts)
			{
				res=v;
				ts=a->allocation[v].timestamp;
			}
			break;
		case apLow:
			if(a->allocation[v].note>note)
			{
				res=v;
				note=a->allocation[v].note;
			}
			break;
		case apHigh:
			if(a->allocation[v].note<note)
			{
				res=v;
				note=a->allocation[v].note;
			}
			break;
		}
	}
	
	return res;
}
	
void assigner_setPriority(struct assigner_s * a, assignerPriority_t prio)
{
	if(prio==a->priority)
		return;
	
	assigner_voiceDone(a,-1);
	
	if(prio>2)
		prio=0;
	
	a->priority=prio;
}

void assigner_setVoiceMask(struct assigner_s * a, uint8_t mask)
{
	if(mask==a->voiceMask)
		return;
	
	assigner_voiceDone(a,-1);
	a->voiceMask=mask;
}

FORCEINLINE int8_t assigner_getAssignment(struct assigner_s * a, int8_t voice, uint8_t * note)
{
	int8_t ad;
	
	ad=a->allocation[voice].assigned;
	
	if(ad && note)
		*note=a->allocation[voice].note;
	
	return ad;
}

int8_t assigner_getAnyPressed(struct assigner_s * a)
{
	int8_t i;
	int8_t v=0;
	
	for(i=0;i<sizeof(a->noteStates);++i)
		v|=a->noteStates[i];
	
	return v!=0;
}

int8_t assigner_getAnyAssigned(struct assigner_s * a)
{
	int8_t i;
	int8_t v=0;
	
	for(i=0;i<SYNTH_VOICE_COUNT;++i)
		v|=a->allocation[i].assigned;
	
	return v!=0;
}

void assigner_assignNote(struct assigner_s * a, uint8_t note, int8_t gate, uint16_t velocity)
{
	uint32_t timestamp;
	uint16_t oldVel;
	uint8_t restoredNote;
	int8_t v,vi,i,legato=0,forceLegato=0;
	int16_t ni,n;
	
	setNoteState(a,note,gate);

	if(gate)
	{
		
reassign:

		timestamp=currentTick;

		if(a->mono)
		{
			// just handle legato & priority
			
			v=0;
			legato=forceLegato;

			if(a->priority!=apLast)
				for(n=0;n<128;++n)
					if(n!=note && getNoteState(a,n))
					{
						if (note>n && a->priority==apLow)
							return;
						if (note<n && a->priority==apHigh)
							return;
						
						legato=1;
					}
		}
		else
		{
			// first, try to get a free voice

			v=getAvailableVoice(a,note,timestamp);

			// no free voice, try to steal one

			if(v<0)
				v=getDispensableVoice(a,note);

			// we might still have no voice

			if(v<0)
				return;
		}

		// try to assign the whole pattern of notes

		for(vi=0;vi<SYNTH_VOICE_COUNT;++vi)
		{
			if(a->patternOffsets[vi]==ASSIGNER_NO_NOTE)
				break;

			n=note+a->patternOffsets[vi];

			a->allocation[v].assigned=1;
			a->allocation[v].velocity=velocity;
			a->allocation[v].rootNote=note;
			a->allocation[v].note=n;
			a->allocation[v].timestamp=timestamp;

			synth_assignerEvent(n,1,v,velocity,legato);

			do
				v=(v+1)%SYNTH_VOICE_COUNT;
			while(isVoiceDisabled(a,v));
		}
	}
	else
	{
		oldVel=UINT16_MAX;
		restoredNote=ASSIGNER_NO_NOTE;

		// some still triggered notes might have been stolen, find them

		for(ni=0;ni<128;++ni)
		{
			if(a->priority==apHigh)
				n=127-ni;
			else
				n=ni;
			
			if(getNoteState(a,n))
			{
				i=0;
				for(v=0;v<SYNTH_VOICE_COUNT;++v)
					if(a->allocation[v].assigned && a->allocation[v].rootNote==n)
					{
						i=1;
						break;
					}

				if(i==0)
				{
					restoredNote=n;
					break;
				}
			}
		}

		// hitting a note spawns a pattern of note, not just one

		for(v=0;v<SYNTH_VOICE_COUNT;++v)
			if(a->allocation[v].assigned && a->allocation[v].rootNote==note)
			{
				if(restoredNote!=ASSIGNER_NO_NOTE)
				{
					oldVel=a->allocation[v].velocity;
				}
				else
				{
					synth_assignerEvent(a->allocation[v].note,0,v,velocity,0);
				}
			}

		// restored notes can be assigned again

		if(restoredNote!=ASSIGNER_NO_NOTE)
		{
			note=restoredNote;
			gate=1;
			velocity=oldVel;
			forceLegato=1;
			
			goto reassign;
		}
	}
}

void assigner_voiceDone(struct assigner_s * a, int8_t voice)
{
	int8_t v;
	for(v=0;v<SYNTH_VOICE_COUNT;++v)
		if(v==voice || voice<0)
		{
			a->allocation[v].assigned=0;
			a->allocation[v].note=ASSIGNER_NO_NOTE;
			a->allocation[v].rootNote=ASSIGNER_NO_NOTE;
			a->allocation[v].timestamp=0;
		}
}

LOWERCODESIZE void assigner_setPattern(struct assigner_s * a, uint8_t * pattern, int8_t mono)
{
	int8_t i,count=0;
	
	if(mono==a->mono && !memcmp(pattern,&a->patternOffsets[0],SYNTH_VOICE_COUNT))
		return;

	if(mono!=a->mono)
		assigner_voiceDone(a,-1);
	
	a->mono=mono;
	memset(&a->patternOffsets[0],ASSIGNER_NO_NOTE,SYNTH_VOICE_COUNT);

	for(i=0;i<SYNTH_VOICE_COUNT;++i)
	{
		if(pattern[i]==ASSIGNER_NO_NOTE)
			break;
		
		a->patternOffsets[i]=pattern[i];
		++count;
	}

	if(count>0)
	{
		a->patternOffsets[0]=0; // root note always has offset 0
	}
	else
	{
		// empty pattern means unison
		memset(a->patternOffsets,0,SYNTH_VOICE_COUNT);
	}
}

void assigner_getPattern(struct assigner_s * a, uint8_t * pattern, int8_t * mono)
{
	memcpy(pattern,a->patternOffsets,SYNTH_VOICE_COUNT);
	
	if(mono!=NULL)
		*mono=a->mono;
}

LOWERCODESIZE void assigner_latchPattern(struct assigner_s * a)
{
	int16_t i;
	int8_t count;
	uint8_t pattern[SYNTH_VOICE_COUNT];	
	count=0;
	
	memset(pattern,ASSIGNER_NO_NOTE,SYNTH_VOICE_COUNT);
	
	for(i=0;i<128;++i)
		if(getNoteState(a,i))
		{
			pattern[count]=i;
			
			if(count>0)
				pattern[count]-=pattern[0]; // it's a list of offsets to the root note
						
			++count;
			
			if(count>=SYNTH_VOICE_COUNT)
				break;
		}

	assigner_setPattern(a,pattern,1);
}

LOWERCODESIZE void assigner_setPoly(struct assigner_s * a)
{
	uint8_t polyPattern[SYNTH_VOICE_COUNT]={0,ASSIGNER_NO_NOTE,ASSIGNER_NO_NOTE,ASSIGNER_NO_NOTE,ASSIGNER_NO_NOTE,ASSIGNER_NO_NOTE};	
	assigner_setPattern(a,polyPattern,0);
}

void assigner_init(struct assigner_s * a)
{
	memset(&assigner,0,sizeof(assigner));

	a->voiceMask=0x3f;
	memset(&a->patternOffsets[0],ASSIGNER_NO_NOTE,SYNTH_VOICE_COUNT);
	a->patternOffsets[0]=0;
}

