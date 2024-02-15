////////////////////////////////////////////////////////////////////////////////
// MIDI handling
////////////////////////////////////////////////////////////////////////////////

#include "midi.h"

#include "storage.h"
#include "ui.h"
#include "arp.h"
#include "seq.h"

#include "../xnormidi/midi_device.h"
#include "../xnormidi/midi.h"

#define NOTE_TRANSPOSE_OFFSET -12

#define PENDING_UPDATE_TIMEOUT (TICKER_HZ/5)

enum midiCC_e
{
	ccNone=0,ccFree,
	ccContinuousCoarse,ccContinuousFine,ccStepped,
	ccModWheel,ccHoldPedal,ccAllSoundOff,ccAllNotesOff,
	ccDataCoarse,ccDataFine,ccDataIncrement,ccDataDecrement,ccNRPNCoarse,ccNRPNFine,
	ccBankCoarse,ccBankFine,
};

struct midiCC_s
{
	enum midiCC_e type;
	int8_t number;
};

const struct midiCC_s midiCCs[128]={
	/* CC 0-7 */
	{ccBankCoarse},
	{ccModWheel},
	{ccFree},
	{ccNone},
	{ccNone},
	{ccNone},
	{ccDataCoarse},
	{ccNone},
	/* CC 8-15 */
	{ccFree},
	{ccNone},
	{ccNone},
	{ccNone},
	{ccContinuousCoarse,cpAFreq},
	{ccContinuousCoarse,cpAVol},
	{ccContinuousCoarse,cpABaseWMod},
	{ccContinuousCoarse,cpBFreq},
	/* CC 16-23 */
	{ccContinuousCoarse,cpBVol},
	{ccContinuousCoarse,cpBBaseWMod},
	{ccContinuousCoarse,cpDetune},
	{ccContinuousCoarse,cpCutoff},
	{ccContinuousCoarse,cpResonance},
	{ccContinuousCoarse,cpFilEnvAmt},
	{ccContinuousCoarse,cpFilKbdAmt},
	{ccContinuousCoarse,cpWModAEnv},
	/* CC 24-31 */
	{ccContinuousCoarse,cpFilAtt},
	{ccContinuousCoarse,cpFilDec},
	{ccContinuousCoarse,cpFilSus},
	{ccContinuousCoarse,cpFilRel},
	{ccContinuousCoarse,cpAmpAtt},
	{ccContinuousCoarse,cpAmpDec},
	{ccContinuousCoarse,cpAmpSus},
	{ccContinuousCoarse,cpAmpRel},
	/* CC 32-39 */
	{ccBankFine},
	{ccNone},
	{ccNone},
	{ccFree},
	{ccNone},
	{ccNone},
	{ccDataFine},
	{ccNone},
	/* CC 40-47 */
	{ccNone},
	{ccFree},
	{ccNone},
	{ccNone},
	{ccContinuousCoarse,cpLFOFreq},
	{ccContinuousCoarse,cpLFOAmt},
	{ccContinuousCoarse,cpLFOPitchAmt},
	{ccContinuousCoarse,cpLFOWModAmt},
	/* CC 48-55 */
	{ccContinuousCoarse,cpLFOFilAmt},
	{ccContinuousCoarse,cpLFOAmpAmt},
	{ccContinuousCoarse,cpLFO2Freq},
	{ccContinuousCoarse,cpLFO2Amt},
	{ccContinuousCoarse,cpModDelay},
	{ccContinuousCoarse,cpGlide},
	{ccContinuousCoarse,cpAmpVelocity},
	{ccContinuousCoarse,cpFilVelocity},
	/* CC 56-63 */
	{ccContinuousCoarse,cpMasterTune},
	{ccContinuousCoarse,cpUnisonDetune},
	{ccContinuousCoarse,cpNoiseVol},
	{ccContinuousCoarse,cpLFO2PitchAmt},
	{ccContinuousCoarse,cpLFO2WModAmt},
	{ccContinuousCoarse,cpLFO2FilAmt},
	{ccContinuousCoarse,cpLFO2AmpAmt},
	{ccContinuousCoarse,cpLFOResAmt},
	/* CC 64-71 */
	{ccHoldPedal},
	{ccNone},
	{ccNone},
	{ccNone},
	{ccNone},
	{ccNone},
	{ccContinuousCoarse,cpLFO2ResAmt},
	{ccContinuousCoarse,cpWModAtt},
	/* CC 72-79 */
	{ccContinuousCoarse,cpWModDec},
	{ccContinuousCoarse,cpWModSus},
	{ccContinuousCoarse,cpWModRel},
	{ccContinuousCoarse,cpWModBEnv},
	{ccContinuousCoarse,cpWModVelocity},
	{ccFree},
	{ccStepped,spABank_Unsaved},
	{ccStepped,spAWave_Unsaved},
	/* CC 80-87 */
	{ccStepped,spAWModType},
	{ccStepped,spBBank_Unsaved},
	{ccStepped,spBWave_Unsaved},
	{ccStepped,spBWModType},
	{ccStepped,spLFOShape},
	{ccStepped,spLFOTargets},
	{ccStepped,spFilEnvSlow},
	{ccStepped,spAmpEnvSlow},
	/* CC 88-95 */
	{ccStepped,spBenderRange},
	{ccStepped,spBenderTarget},
	{ccStepped,spModwheelRange},
	{ccStepped,spModwheelTarget},
	{ccStepped,spUnison},
	{ccStepped,spAssignerPriority},
	{ccStepped,spChromaticPitch},
	{ccStepped,spOscSync},
	/* CC 96-103 */
	{ccDataIncrement},
	{ccDataDecrement},
	{ccNRPNFine},
	{ccNRPNCoarse},
	{ccNone},
	{ccNone},
	{ccStepped,spAXOvrBank_Unsaved},
	{ccStepped,spAXOvrWave_Unsaved},
	/* CC 104-111 */
	{ccStepped,spFilEnvLin},
	{ccStepped,spLFO2Shape},
	{ccStepped,spLFO2Targets},
	{ccStepped,spVoiceCount},
	{ccStepped,spPresetType},
	{ccStepped,spPresetStyle},
	{ccStepped,spAmpEnvLin},
	{ccStepped,spFilEnvLoop},
	/* CC 112-119 */
	{ccStepped,spAmpEnvLoop},
	{ccStepped,spWModEnvSlow},
	{ccStepped,spWModEnvLin},
	{ccStepped,spWModEnvLoop},
	{ccStepped,spPressureRange},
	{ccStepped,spPressureTarget},
	{ccStepped,spBXOvrBank_Unsaved},
	{ccStepped,spBXOvrWave_Unsaved},
	/* CC 120-127 */
	{ccAllSoundOff},
	{ccNone},
	{ccNone},
	{ccAllNotesOff},
	{ccNone},
	{ccNone},
	{ccNone},
	{ccNone},
};

static struct
{
	MidiDevice device[mpCount];
	int8_t isNrpnStepped[mpCount];
	int8_t currentNrpn[mpCount];
	uint32_t presetTimeout;
	uint32_t pendingBankWaveTimeout[abxCount];
} midi;

static uint16_t combineBytes(uint8_t first, uint8_t second)
{
   uint16_t _14bit;
   _14bit = (uint16_t)second;
   _14bit <<= 7;
   _14bit |= (uint16_t)first;
   return _14bit;
}

static int8_t filterChannel(uint8_t channel)
{
	return settings.midiReceiveChannel<0 || (channel&MIDI_CHANMASK)==settings.midiReceiveChannel;
}

static midiPort_t getPort(MidiDevice * device)
{
	for(midiPort_t port=0;port<mpCount;++port)
		if(device==&midi.device[port])
			return port;
	return -1;
}

static int8_t setContinuousParameterCoarse(continuousParameter_t param, uint8_t value)
{
	if((currentPreset.continuousParameters[param]>>9)!=value)
	{
		currentPreset.continuousParameters[param]&=0x01fc;
		currentPreset.continuousParameters[param]|=(uint16_t)value<<9;
		return 1;	
	}
	return 0;	
}

static int8_t setContinuousParameterFine(continuousParameter_t param, uint8_t value)
{
	if(((currentPreset.continuousParameters[param]>>2)&0x7f)!=value)
	{
		currentPreset.continuousParameters[param]&=0xfe00;
		currentPreset.continuousParameters[param]|=(uint16_t)value<<2;
		return 1;	
	}
	return 0;	
}

static int8_t setSteppedParameter(steppedParameter_t param, uint8_t value, int8_t isRaw)
{
	int8_t changed=0;
	uint16_t v=value;
	
	if(!isRaw)
		v=(v*steppedParametersSteps[param])>>7;

	if(currentPreset.steppedParameters[param]!=v)
	{
		currentPreset.steppedParameters[param]=v;
		changed=1;	
	}
	
	if(changed)
	{
		switch(param)
		{
			case spABank_Unsaved:
			case spBBank_Unsaved:
			case spAXOvrBank_Unsaved:
			case spBXOvrBank_Unsaved:
				synth_getBankName(value,currentPreset.oscBank[sp2abx[param]]);
				midi.pendingBankWaveTimeout[sp2abx[param]]=currentTick+PENDING_UPDATE_TIMEOUT;
				break;
			case spAWave_Unsaved:
			case spBWave_Unsaved:
			case spAXOvrWave_Unsaved:
			case spBXOvrWave_Unsaved:
				synth_getWaveName(value,currentPreset.oscWave[sp2abx[param]]);
				midi.pendingBankWaveTimeout[sp2abx[param]]=currentTick+PENDING_UPDATE_TIMEOUT;
				break;
			default:
				/* nothing */;
		}
	}

	return changed;
}

static int8_t setCurrentNrpn(int8_t port, uint8_t param)
{
	return midi.currentNrpn[port]=MAX(0,MIN((midi.isNrpnStepped[port]?spCount:cpCount)-1,param));
}

static void noteOnEvent(MidiDevice * device, uint8_t channel, uint8_t note, uint8_t velocity)
{
	int16_t intNote;
	
	if(!filterChannel(channel))
		return;
	
#ifdef DEBUG_
	print("midi note on  ");
	phex(note);
	print("\n");
#endif

	note+=NOTE_TRANSPOSE_OFFSET;
	
	if(ui_isTransposing())
	{
		ui_setTranspose(note-MIDDLE_C_NOTE);
		return;
	}
	
	if(arp_getMode()==amOff)
	{
		// sequencer note input		
		if(seq_getMode(0)==smRecording || seq_getMode(1)==smRecording)
			seq_inputNote(note,velocity!=0);

		intNote=note+ui_getTranspose();
		intNote=MAX(0,MIN(127,intNote));
		assigner_assignNote(intNote,velocity!=0,(((uint32_t)velocity+1)<<9)-1,1);
	}
	else
	{
		arp_assignNote(note,velocity!=0);
	}
}

static void noteOffEvent(MidiDevice * device, uint8_t channel, uint8_t note, uint8_t velocity)
{
	int16_t intNote;
	
	if(!filterChannel(channel))
		return;
	
#ifdef DEBUG_
	print("midi note off ");
	phex(note);
	print("\n");
#endif

	note+=NOTE_TRANSPOSE_OFFSET;
	
	if(arp_getMode()==amOff)
	{
		// sequencer note input		
		if(seq_getMode(0)==smRecording || seq_getMode(1)==smRecording)
			seq_inputNote(note,0);

		intNote=note+ui_getTranspose();
		intNote=MAX(0,MIN(127,intNote));
		assigner_assignNote(intNote,0,0,1);
	}
	else
	{
		arp_assignNote(note,0);
	}
}

static void ccEvent(MidiDevice * device, uint8_t channel, uint8_t control, uint8_t value)
{
	int8_t port,change=0;
	struct midiCC_s cc;
	
	if(!filterChannel(channel))
		return;
	
#ifdef DEBUG_
	print("midi cc ");
	phex(control);
	print(" value ");
	phex(value);
	print("\n");
#endif
	
	port=getPort(device);
	if(port<0)
		return;
	
	cc=midiCCs[control&0x7f];
	
	switch(cc.type)
	{
		case ccModWheel:
			synth_wheelEvent(0,value<<9,2);
			break;
		case ccHoldPedal:
			assigner_holdEvent(value);
			break;
		case ccAllSoundOff:
			assigner_panicOff();
			break;
		case ccAllNotesOff:
			assigner_allKeysOff();
			break;
		case ccBankCoarse:
		case ccBankFine:
			settings.presetNumber=(settings.presetNumber%100)+(value%10)*100;
			midi.presetTimeout=currentTick+PENDING_UPDATE_TIMEOUT;
			break;
		case ccContinuousCoarse:
			change=setContinuousParameterCoarse(cc.number,value);
			break;
		case ccContinuousFine:
			change=setContinuousParameterFine(cc.number,value);
			break;
		case ccStepped:
			change=setSteppedParameter(cc.number,value,0);
			break;
		case ccNRPNCoarse:
			midi.isNrpnStepped[port]=value&1;
			setCurrentNrpn(port,midi.currentNrpn[port]);
			break;
		case ccNRPNFine:
			setCurrentNrpn(port,value);
			break;
		case ccDataIncrement:
			if(midi.isNrpnStepped[port])
			{
				steppedParameter_t s=midi.currentNrpn[port];
				change=setSteppedParameter(s,MIN(steppedParametersSteps[s]-1,currentPreset.steppedParameters[s]+1),1);
			}
			else
			{
				uint8_t v=currentPreset.continuousParameters[midi.currentNrpn[port]]>>9;
				change=setContinuousParameterCoarse(midi.currentNrpn[port],MIN(INT8_MAX,v+1));
			}
			break;
		case ccDataDecrement:
			if(midi.isNrpnStepped[port])
			{
				steppedParameter_t s=midi.currentNrpn[port];
				change=setSteppedParameter(s,MAX(0,currentPreset.steppedParameters[s]-1),1);
			}
			else
			{
				uint8_t v=currentPreset.continuousParameters[midi.currentNrpn[port]]>>9;
				change=setContinuousParameterCoarse(midi.currentNrpn[port],MAX(0,v-1));
			}
			break;
		case ccDataCoarse:
			if(midi.isNrpnStepped[port])
			{
				change=setSteppedParameter(midi.currentNrpn[port],value,0);
			}
			else
			{
				change=setContinuousParameterCoarse(midi.currentNrpn[port],value);
			}
			break;
		case ccDataFine:
			if(midi.isNrpnStepped[port])
			{
				/* stepped parameters dont have fine setting */;
			}
			else
			{
				change=setContinuousParameterFine(midi.currentNrpn[port],value);
			}
			break;
		case ccNone:
		case ccFree:
			/* nothing */;
	}

	if(change)
	{
		ui_setPresetModified(1);
		synth_refreshFullState(0);
	}
}

static void progchangeEvent(MidiDevice * device, uint8_t channel, uint8_t program)
{
	if(!filterChannel(channel))
		return;
	
	uint16_t newPresetNumber=(((settings.presetNumber/100)%10)*100)+program;

	if(program<100 && newPresetNumber!=settings.presetNumber)
	{
		settings.presetNumber=newPresetNumber;
		midi.presetTimeout=currentTick+PENDING_UPDATE_TIMEOUT;
	}
}

static void pitchBendEvent(MidiDevice * device, uint8_t channel, uint8_t v1, uint8_t v2)
{
	if(!filterChannel(channel))
		return;

	int16_t value;
	
	value=combineBytes(v1,v2);
	value-=0x2000;
	value<<=2;
	
	synth_wheelEvent(value,0,1);
}

static void chanpressureEvent(MidiDevice * device, uint8_t channel, uint8_t pressure)
{
	if(!filterChannel(channel))
		return;

	synth_pressureEvent(pressure<<9);
}

static void realtimeEvent(MidiDevice * device, uint8_t event)
{
	synth_realtimeEvent(getPort(device),event);
}

void midi_init(void)
{
	memset(&midi,0,sizeof(midi));

	for(abx_t abx=0;abx<abxCount;++abx)
		midi.pendingBankWaveTimeout[abx]=UINT32_MAX;
	midi.presetTimeout=UINT32_MAX;

	for(midiPort_t port=0;port<mpCount;++port)
	{
		MidiDevice * d = &midi.device[port];
		
		midi_device_init(d);
		midi_register_noteon_callback(d,noteOnEvent);
		midi_register_noteoff_callback(d,noteOffEvent);
		midi_register_cc_callback(d,ccEvent);
		midi_register_progchange_callback(d,progchangeEvent);
		midi_register_pitchbend_callback(d,pitchBendEvent);
		midi_register_chanpressure_callback(d,chanpressureEvent);
		midi_register_realtime_callback(d,realtimeEvent);
	}
}

void midi_processInput(void)
{
	for(midiPort_t port=0;port<mpCount;++port)
		midi_device_process(&midi.device[port]);
}

void midi_newData(midiPort_t port, uint8_t data)
{
	midi_device_input(&midi.device[port],1,&data);
}

void midi_update(void)
{
	// pending osc bank/wave updates
	
	for(abx_t abx=0;abx<abxCount;++abx)
	{
		if(currentTick>midi.pendingBankWaveTimeout[abx])
		{
			synth_refreshCurWaveNames(abx,1);
			synth_refreshWaveforms(abx);
			midi.pendingBankWaveTimeout[abx]=UINT32_MAX;
		}
	}	
	
	// pending program change updates
	
	if(currentTick>midi.presetTimeout)
		BLOCK_INT(1)
		{
			// temporarily silence voices
			for(int8_t v=0;v<SYNTH_VOICE_COUNT;++v)
				synth_refreshCV(v,cvAmp, 0);					
			
			settings_save();		

			if(!preset_loadCurrent(settings.presetNumber))
				preset_loadDefault(1);
			ui_setPresetModified(0);	

			synth_refreshFullState(1);

			midi.presetTimeout=UINT32_MAX;
		}
}