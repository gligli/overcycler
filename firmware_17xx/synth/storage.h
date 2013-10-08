#ifndef STORAGE_H
#define	STORAGE_H

#include "synth.h"
#include "assigner.h"
#include "tuner.h"

#define MANUAL_PRESET_PAGE ((STORAGE_SIZE/STORAGE_PAGE_SIZE)-5)

typedef enum
{
	cpAFreq=0,cpAVol=1,cpABaseWMod=2,
	cpBFreq=3,cpBVol=4,cpBBaseWMod=5,cpBFineFreq=6,
	cpCutoff=7,cpResonance=8,cpFilEnvAmt=9,cpFilKbdAmt=10,cpWModFilEnv=11,
	cpFilAtt=12,cpFilDec=13,cpFilSus=14,cpFilRel=15,
	cpAmpAtt=16,cpAmpDec=17,cpAmpSus=18,cpAmpRel=19,
	cpLFOFreq=20,cpLFOAmt=21,
	cpLFOPitchAmt=22,cpLFOWModAmt=23,cpLFOFilAmt=24,cpLFOAmpAmt=25,
	cpVibFreq=26,cpVibAmt=27,
	cpModDelay=28,cpGlide=29,
	cpAmpVelocity=30,cpFilVelocity=31,
	cpMasterTune=32,cpUnisonDetune=33,
	cpMasterLeft=34,cpMasterRight=35,
	cpSeqArpClock=36,

	// /!\ this must stay last
	cpCount
} continuousParameter_t;

typedef enum
{
	spABank=0,spAWave=1,spABaseWMod=2,spAFilWMod=3,
	spBBank=4,spBWave=5,spBBaseWMod=6,spBFilWMod=7,
			
	spLFOShape=8,spLFOShift=9,spLFOTargets=10,

	spFilEnvSlow=11,spAmpEnvSlow=12,
			
	spBenderSemitones=13,spBenderTarget=14,
	spModwheelShift=15,spModwheelTarget=16,
			
	spUnison=17,spAssignerPriority=18,spChromaticPitch=19,
			
	// /!\ this must stay last
	spCount
} steppedParameter_t;

struct settings_s
{
	uint16_t tunes[TUNER_OCTAVE_COUNT][TUNER_CV_COUNT];

	uint16_t presetNumber;
	int8_t presetMode;
	
	int8_t midiReceiveChannel; // -1: omni / 0-15: channel 1-16
	int8_t midiSendChannel; // 0-15: channel 1-16
	uint8_t voiceMask;
};

struct preset_s
{
	uint8_t steppedParameters[spCount];
	uint16_t continuousParameters[cpCount];
	
	uint8_t voicePattern[SYNTH_VOICE_COUNT];
};

extern struct settings_s settings;
extern struct preset_s currentPreset;
extern const uint8_t steppedParametersBits[spCount];

int8_t settings_load(void);
void settings_save(void);

int8_t preset_loadCurrent(uint16_t number);
void preset_saveCurrent(uint16_t number);

void preset_loadDefault(int8_t makeSound);
void settings_loadDefault(void);

void storage_export(uint16_t number, uint8_t * buf, int16_t * size);
void storage_import(uint16_t number, uint8_t * buf, int16_t size);

#endif	/* STORAGE_H */
