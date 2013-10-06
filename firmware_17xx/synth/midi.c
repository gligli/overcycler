////////////////////////////////////////////////////////////////////////////////
// MIDI handling
////////////////////////////////////////////////////////////////////////////////

#include "midi.h"

#include "storage.h"
#include "ui.h"

#include "../xnormidi/midi_device.h"
#include "../xnormidi/midi.h"

#define MIDI_BASE_STEPPED_CC 56
#define MIDI_BASE_COARSE_CC 16
#define MIDI_BASE_FINE_CC 80
#define MIDI_BASE_NOTE 24

static MidiDevice midi;

extern void refreshFullState(void);
extern void refreshPresetMode(void);

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
	
	assigner_assignNote(intNote,velocity!=0,(((uint32_t)velocity+1)<<9)-1,0);
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
	
	assigner_assignNote(intNote,0,0,0);
}

static void midi_ccEvent(MidiDevice * device, uint8_t channel, uint8_t control, uint8_t value)
{
	int16_t param;
	
	if(!midiFilterChannel(channel))
		return;
	
#ifdef DEBUG
	print("midi cc ");
	phex(control);
	print(" value ");
	phex(value);
	print("\n");
#endif

	if(control==0 && value<=1 && settings.presetMode!=value) // coarse bank #
	{
		settings.presetMode=value;
		settings_save();
		refreshPresetMode();
		refreshFullState();
	}
	
	if(!settings.presetMode) // in manual mode CC changes would only conflict with pot scans...
		return;
	
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

	if(settings.presetMode && program<100  && program!=settings.presetNumber)
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

void midi_init(void)
{
	midi_device_init(&midi);
	midi_register_noteon_callback(&midi,midi_noteOnEvent);
	midi_register_noteoff_callback(&midi,midi_noteOffEvent);
	midi_register_cc_callback(&midi,midi_ccEvent);
	midi_register_progchange_callback(&midi,midi_progChangeEvent);
}

void midi_update(void)
{
	midi_device_process(&midi);
}

void midi_newData(uint8_t data)
{
	midi_device_input(&midi,1,&data);
}
