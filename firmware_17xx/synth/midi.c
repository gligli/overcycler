////////////////////////////////////////////////////////////////////////////////
// MIDI handling
////////////////////////////////////////////////////////////////////////////////

#include "midi.h"

#include "storage.h"
#include "ui.h"
#include "arp.h"

#include "../xnormidi/midi_device.h"
#include "../xnormidi/midi.h"

#define MIDI_BASE_STEPPED_CC 56
#define MIDI_BASE_COARSE_CC 16
#define MIDI_BASE_FINE_CC 80
#define MIDI_BASE_NOTE 0

static MidiDevice midi;

uint16_t midiCombineBytes(uint8_t first, uint8_t second)
{
   uint16_t _14bit;
   _14bit = (uint16_t)second;
   _14bit <<= 7;
   _14bit |= (uint16_t)first;
   return _14bit;
}

static int8_t midiFilterChannel(uint8_t channel)
{
	return settings.midiReceiveChannel<0 || (channel&MIDI_CHANMASK)==settings.midiReceiveChannel;
}

static void midi_noteOnEvent(MidiDevice * device, uint8_t channel, uint8_t note, uint8_t velocity)
{
	int16_t intNote;
	
	if(!midiFilterChannel(channel))
		return;
	
#ifdef DEBUG_
	print("midi note on  ");
	phex(note);
	print("\n");
#endif

	intNote=note-MIDI_BASE_NOTE;
	intNote=MAX(0,intNote);
	
	if(arp_getMode()==amOff)
	{
		assigner_assignNote(intNote,velocity!=0,(((uint32_t)velocity+1)<<9)-1,0);
	}
	else
	{
		arp_assignNote(intNote,velocity!=0);
	}
}

static void midi_noteOffEvent(MidiDevice * device, uint8_t channel, uint8_t note, uint8_t velocity)
{
	int16_t intNote;
	
	if(!midiFilterChannel(channel))
		return;
	
#ifdef DEBUG_
	print("midi note off ");
	phex(note);
	print("\n");
#endif

	intNote=note-MIDI_BASE_NOTE;
	intNote=MAX(0,intNote);
	
	if(arp_getMode()==amOff)
	{
		assigner_assignNote(intNote,0,0,0);
	}
	else
	{
		arp_assignNote(intNote,0);
	}
}

static void midi_ccEvent(MidiDevice * device, uint8_t channel, uint8_t control, uint8_t value)
{
	int16_t param;
	
	if(!midiFilterChannel(channel))
		return;
	
#ifdef DEBUG_
	print("midi cc ");
	phex(control);
	print(" value ");
	phex(value);
	print("\n");
#endif

	if (control==1) // modwheel
	{
		synth_wheelEvent(0,value<<9,2);
	}
	
	if(control>=MIDI_BASE_COARSE_CC && control<MIDI_BASE_COARSE_CC+cpCount)
	{
		param=control-MIDI_BASE_COARSE_CC;

		currentPreset.continuousParameters[param]&=0x01fc;
		currentPreset.continuousParameters[param]|=(uint16_t)value<<9;
		ui_setPresetModified(1);	
	}
	else if(control>=MIDI_BASE_FINE_CC && control<MIDI_BASE_FINE_CC+cpCount)
	{
		param=control-MIDI_BASE_FINE_CC;

		currentPreset.continuousParameters[param]&=0xfe00;
		currentPreset.continuousParameters[param]|=(uint16_t)value<<2;
		ui_setPresetModified(1);	
	}
	else if(control>=MIDI_BASE_STEPPED_CC && control<MIDI_BASE_STEPPED_CC+spCount)
	{
		param=control-MIDI_BASE_STEPPED_CC;
		
		currentPreset.steppedParameters[param]=value>>(7-steppedParametersBits[param]);
		ui_setPresetModified(1);	
	}

	if(ui_isPresetModified())
		refreshFullState();
}

static void midi_progChangeEvent(MidiDevice * device, uint8_t channel, uint8_t program)
{
	if(!midiFilterChannel(channel))
		return;

	if(program<100  && program!=settings.presetNumber)
	{
		if(preset_loadCurrent(program))
		{
			settings.presetNumber=program;
			ui_setPresetModified(0);	
			settings_save();		
			refreshFullState();
		}
	}
}

static void midi_pitchBendEvent(MidiDevice * device, uint8_t channel, uint8_t v1, uint8_t v2)
{
	if(!midiFilterChannel(channel))
		return;

	int16_t value;
	
	value=midiCombineBytes(v1,v2);
	value-=0x2000;
	value<<=2;
	
	synth_wheelEvent(value,0,1);
}
void midi_init(void)
{
	midi_device_init(&midi);
	midi_register_noteon_callback(&midi,midi_noteOnEvent);
	midi_register_noteoff_callback(&midi,midi_noteOffEvent);
	midi_register_cc_callback(&midi,midi_ccEvent);
	midi_register_progchange_callback(&midi,midi_progChangeEvent);
	midi_register_pitchbend_callback(&midi,midi_pitchBendEvent);
}

void midi_update(void)
{
	midi_device_process(&midi);
}

void midi_newData(uint8_t data)
{
	midi_device_input(&midi,1,&data);
}
