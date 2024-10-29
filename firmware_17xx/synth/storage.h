#ifndef STORAGE_H
#define	STORAGE_H

#include "synth.h"
#include "assigner.h"
#include "tuner.h"

typedef enum
{
	cpAFreq=0,cpAVol=1,cpABaseWMod=2,
	cpBFreq=3,cpBVol=4,cpBBaseWMod=5,cpDetune=6,
	cpCutoff=7,cpResonance=8,cpFilEnvAmt=9,cpFilKbdAmt=10,cpWModAEnv=11,
	cpFilAtt=12,cpFilDec=13,cpFilSus=14,cpFilRel=15,
	cpAmpAtt=16,cpAmpDec=17,cpAmpSus=18,cpAmpRel=19,
	cpLFOFreq=20,cpLFOAmt=21,
	cpLFOPitchAmt=22,cpLFOWModAmt=23,cpLFOFilAmt=24,cpLFOAmpAmt=25,
	cpLFO2Freq=26,cpLFO2Amt=27,
	cpModDelay=28,cpGlide=29,
	cpAmpVelocity=30,cpFilVelocity=31,
	cpMasterTune=32,cpUnisonDetune=33,
	cpMasterLeft_Legacy=34,cpMasterRight_Legacy=35,
	cpSeqArpClock_Legacy=36,cpNoiseVol=37,
	cpLFO2PitchAmt=38,cpLFO2WModAmt=39,cpLFO2FilAmt=40,cpLFO2AmpAmt=41,
	cpLFOResAmt=42,cpLFO2ResAmt=43,
	
	cpWModAtt=44,cpWModDec=45,cpWModSus=46,cpWModRel=47,
	cpWModBEnv=48,cpWModVelocity=49,
	cpAmpLevel=50,

	// /!\ this must stay last
	cpCount
} continuousParameter_t;

typedef enum
{
	spABank_Unsaved=0,spAWave_Unsaved=1,spAWModType=2,spAWModEnvEn_Legacy=3,
	spBBank_Unsaved=4,spBWave_Unsaved=5,spBWModType=6,spBWModEnvEn_Legacy=7,
	
	spLFOShape=8,spLFOSpeed=9,spLFOTargets=10,
	
	spFilEnvSlow=11,spAmpEnvSlow=12,
	
	spBenderRange=13,spBenderTarget=14,
	spModwheelRange=15,spModwheelTarget=16,
	
	spUnison=17,spAssignerPriority=18,spChromaticPitch=19,
	
	spOscSync=20,
	
	spAXOvrBank_Unsaved=21,spAXOvrWave_Unsaved=22,
	spFilEnvLin=23,
	
	spLFO2Shape=24,spLFO2Speed=25,spLFO2Targets=26,spVoiceCount=27,
	
	spPresetType=28, spPresetStyle=29,
	
	spAmpEnvLin=30,spFilEnvLoop=31,spAmpEnvLoop=32,
	
	spWModEnvSlow=33,spWModEnvLin=34,spWModEnvLoop=35,
	
	spPressureRange=36,spPressureTarget=37,
	
	spBXOvrBank_Unsaved=38,spBXOvrWave_Unsaved=39,
			
	spLFOTrig=40, spLFO2Trig=41,

	// /!\ this must stay last
	spCount
} steppedParameter_t;

typedef enum
{
	ptOther, ptBass, ptPad, ptStrings, ptBrass, ptKeys, ptLead, ptArpeggios
}presetType_t;

typedef enum
{
	psOther, psNeutral, psClean, psRealistic, psSilky, psRaw, psHeavy, psCrunchy
}presetStyle_t;

struct settings_s
{
	uint16_t tunes[TUNER_OCTAVE_COUNT][TUNER_CV_COUNT];

	uint16_t presetNumber;
	
	int8_t midiReceiveChannel; // -1: omni / 0-15: channel 1-16
	uint8_t voiceMask;
	
	int8_t syncMode;
	int8_t usbMIDI;
	
	uint16_t sequencerBank;
	uint16_t seqArpClock;
	
	uint8_t lcdContrast;
};

struct preset_s
{
	int16_t loadedPresetNumber;
	char presetName[53];

	uint8_t steppedParameters[spCount];
	uint16_t continuousParameters[cpCount];
	
	uint8_t voicePattern[SYNTH_VOICE_COUNT];
	
	char oscBank[abxCount][MAX_FILENAME];
	char oscWave[abxCount][MAX_FILENAME];
};

struct namedParam_s
{
	char *name;
	uint8_t param;
};

extern struct settings_s settings;
extern struct preset_s currentPreset;

extern const struct namedParam_s continuousParametersZeroCentered[cpCount];
extern const struct namedParam_s steppedParametersSteps[spCount];

int8_t settings_load(void);
void settings_save(void);

int8_t preset_loadCurrent(uint16_t number);
void preset_saveCurrent(uint16_t number);
int8_t preset_fileExists(uint16_t number);

void preset_loadDefault(int8_t makeSound);
void settings_loadDefault(void);

int8_t storage_loadSequencer(int8_t track, uint8_t * data, uint8_t size);
void storage_saveSequencer(int8_t track, uint8_t * data, uint8_t size);

#endif	/* STORAGE_H */

