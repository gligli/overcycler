////////////////////////////////////////////////////////////////////////////////
// Presets and settings storage, relies on low level page storage system
////////////////////////////////////////////////////////////////////////////////

#include "storage.h"
#include "lfo.h"
#include "wtosc.h"
#include "seq.h"
#include "ui.h"
#include "main.h"

// increment this each time the binary format is changed
#define STORAGE_VERSION 17

#define STORAGE_MAGIC 0x006116a5
#define STORAGE_MAX_SIZE 512

#define SETTINGS_PAGE_COUNT 1
#define SETTINGS_PAGE (STORAGE_PAGE_COUNT-4)

#define SEQUENCER_START_PAGE 65000

const int8_t continuousParametersZeroCentered[cpCount] = 
{
	/*cpAFreq*/0,
	/*cpAVol*/0,
	/*cpABaseWMod*/1,
	/*cpBFreq*/0,
	/*cpBVol*/0,
	/*cpBBaseWMod*/1,
	/*cpDetune*/1,
	/*cpCutoff*/0,
	/*cpResonance*/0,
	/*cpFilEnvAmt*/1,
	/*cpFilKbdAmt*/0,
	/*cpWModAEnv*/1,
	/*cpFilAtt*/0,
	/*cpFilDec*/0,
	/*cpFilSus*/0,
	/*cpFilRel*/0,
	/*cpAmpAtt*/0,
	/*cpAmpDec*/0,
	/*cpAmpSus*/0,
	/*cpAmpRel*/0,
	/*cpLFOFreq*/0,
	/*cpLFOAmt*/0,
	/*cpLFOPitchAmt*/0,
	/*cpLFOWModAmt*/0,
	/*cpLFOFilAmt*/0,
	/*cpLFOAmpAmt*/0,
	/*cpLFO2Freq*/0,
	/*cpLFO2Amt*/0,
	/*cpModDelay*/0,
	/*cpGlide*/0,
	/*cpAmpVelocity*/0,
	/*cpFilVelocity*/0,
	/*cpMasterTune*/1,
	/*cpUnisonDetune*/0,
	/*cpMasterLeft_Legacy*/0,
	/*cpMasterRight_Legacy*/0,
	/*cpSeqArpClock_Legacy*/0,
	/*cpNoiseVol*/0,
	/*cpLFO2PitchAmt*/0,
	/*cpLFO2WModAmt*/0,
	/*cpLFO2FilAmt*/0,
	/*cpLFO2AmpAmt*/0,
	/*cpLFOResAmt*/0,
	/*cpLFO2ResAmt*/0,
	/*cpWModAtt*/0,
	/*cpWModDec*/0,
	/*cpWModSus*/0,
	/*cpWModRel*/0,
	/*cpWModBEnv*/1,
	/*cpWModVelocity*/0,
};

const uint8_t steppedParametersBits[spCount] = 
{
	/*ABank*/7,
	/*AWave*/7,
	/*ABaseWMod*/3,
	/*AFilWMod*/1,
	/*BBank*/7,
	/*BWave*/7,
	/*BBaseWMod*/3,
	/*BWFilMod*/1,
	/*LFOShape*/3,
	/*LFOShift*/2,
	/*LFOTargets*/2,
	/*FilEnvSlow*/1,
	/*AmpEnvSlow*/1,
	/*BenderRange*/2,
	/*BenderTarget*/2,
	/*ModwheelRange*/2,
	/*ModwheelTarget*/1,
	/*Unison*/1,
	/*AssignerPriority*/2,
	/*ChromaticPitch*/2,
	/*Sync*/1,
	/*XOvrBank*/7,
	/*XOvrWave*/7,
	/*FilEnvLin*/1,
	/*LFO2Shape*/3,
	/*LFO2Shift*/2,
	/*LFO2Targets*/2,
	/*VoiceCount*/3,
	/*PresetType*/3,
	/*PresetStyle*/3,
	/*AmpEnvLin*/1,
	/*FilEnvLoop*/1,
	/*AmpEnvLoop*/1,
	/*WModEnvSlow*/1,
	/*WModEnvLin*/1,
	/*WModEnvLoop*/1,
};

struct settings_s settings;
struct preset_s currentPreset;

static struct
{
	uint8_t buffer[STORAGE_MAX_SIZE];
	uint8_t * bufPtr;
	uint8_t version;
} storage;

static uint32_t storageRead32(void)
{
	uint32_t v;
	memcpy(&v,storage.bufPtr,sizeof(v));
	storage.bufPtr+=sizeof(v);
	return v;
}

static uint16_t storageRead16(void)
{
	uint16_t v;
	memcpy(&v,storage.bufPtr,sizeof(v));
	storage.bufPtr+=sizeof(v);
	return v;
}

/*
static int16_t storageReadS16(void)
{
	int16_t v;
	memcpy(&v,storage.bufPtr,sizeof(v));
	storage.bufPtr+=sizeof(v);
	return v;
}
*/

static uint8_t storageRead8(void)
{
	uint8_t v;
	v=*(uint8_t*)storage.bufPtr;
	storage.bufPtr+=sizeof(v);
	return v;
}

static int8_t storageReadS8(void)
{
	int8_t v;
	v=*(int8_t*)storage.bufPtr;
	storage.bufPtr+=sizeof(v);
	return v;
}

static void storageReadStr(char * v)
{
	strcpy(v,(const char *)storage.bufPtr);
	storage.bufPtr+=strlen(v)+1;
}

static void storageWrite32(uint32_t v)
{
	memcpy(storage.bufPtr,&v,sizeof(v));
	storage.bufPtr+=sizeof(v);
}

static void storageWrite16(uint16_t v)
{
	memcpy(storage.bufPtr,&v,sizeof(v));
	storage.bufPtr+=sizeof(v);
}

/*
static void storageWriteS16(int16_t v)
{
	memcpy(storage.bufPtr,&v,sizeof(v));
	storage.bufPtr+=sizeof(v);
}
*/

static void storageWrite8(uint8_t v)
{
	*(uint8_t*)storage.bufPtr=v;
	storage.bufPtr+=sizeof(v);
}

static void storageWriteS8(int8_t v)
{
	*(int8_t*)storage.bufPtr=v;
	storage.bufPtr+=sizeof(v);
}

static void storageWriteStr(const char * v)
{
	strcpy((char *)storage.bufPtr,v);
	storage.bufPtr+=strlen(v)+1;
}

static LOWERCODESIZE int8_t storageLoad(uint16_t pageIdx, uint8_t pageCount)
{
	uint16_t i;
	
	memset(storage.buffer,0,sizeof(storage.buffer));
	
	for (i=0;i<pageCount;++i)
		storage_read(pageIdx+i,&storage.buffer[STORAGE_PAGE_SIZE*i]);
	
	storage.bufPtr=storage.buffer;
	storage.version=0;

	if(storageRead32()!=STORAGE_MAGIC)
	{
#ifdef DEBUG
		print("Warning: empty page !\n"); 
#endif		
		return 0;
	}

	storage.version=storageRead8();
	
	return 1;
}

static LOWERCODESIZE void storagePrepareStore(void)
{
	memset(storage.buffer,0,sizeof(storage.buffer));
	storage.bufPtr=storage.buffer;
	storage.version=STORAGE_VERSION;
	
	storageWrite32(STORAGE_MAGIC);
	storageWrite8(storage.version);
}

static LOWERCODESIZE void storageFinishStore(uint16_t pageIdx, uint8_t pageCount)
{
	if((storage.bufPtr-storage.buffer)>sizeof(storage.buffer))
	{
		print("Error: writing too much data to storage !\n"); 
		return;
	}
	
	uint16_t i;
	
	for (i=0;i<pageCount;++i)
		storage_write(pageIdx+i,&storage.buffer[STORAGE_PAGE_SIZE*i]);
}

LOWERCODESIZE int8_t settings_load(void)
{
	int8_t i,j;
	
	if(!storageLoad(SETTINGS_PAGE,SETTINGS_PAGE_COUNT))
		return 0;

	if (storage.version<1)
		return 0;

	// v1
	for(j=0;j<TUNER_OCTAVE_COUNT;++j)
		for(i=0;i<TUNER_CV_COUNT;++i)
			settings.tunes[j][i]=storageRead16();

	settings.presetNumber=storageRead16();
	settings.midiReceiveChannel=storageReadS8();
	settings.voiceMask=storageRead8();
	settings.midiSendChannel=storageReadS8();

	if (storage.version<2)
		return 1;

	// v2

	settings.syncMode=storageRead8();

	if (storage.version<6)
		return 1;
		
	// v6
	
	settings.sequencerBank=storageRead16();
	
	if (storage.version<7)
		return 1;
		
	// v7
	
	settings.seqArpClock=storageRead16();

	if (storage.version<17)
		return 1;
		
	// v17
	
	settings.usbMIDI=storageReadS8();
	settings.lcdContrast=storageRead8();

	return 1;
}

LOWERCODESIZE void settings_save(void)
{
	int8_t i,j;
	
	storagePrepareStore();

	// v1

	for(j=0;j<TUNER_OCTAVE_COUNT;++j)
		for(i=0;i<TUNER_CV_COUNT;++i)
			storageWrite16(settings.tunes[j][i]);

	storageWrite16(settings.presetNumber);
	storageWriteS8(settings.midiReceiveChannel);
	storageWrite8(settings.voiceMask);
	storageWriteS8(settings.midiSendChannel);

	// v2

	storageWrite8(settings.syncMode);
	
	// v6
		
	storageWrite16(settings.sequencerBank);

	// v7
		
	storageWrite16(settings.seqArpClock);
	
	// v17
	
	storageWriteS8(settings.usbMIDI);
	storageWrite8(settings.lcdContrast);
	
	// this must stay last
	storageFinishStore(SETTINGS_PAGE,SETTINGS_PAGE_COUNT);
}

LOWERCODESIZE int8_t preset_loadCurrent(uint16_t number)
{
	int8_t i;

	storageLoad(number,1);
	
	preset_loadDefault(0);

	if (storage.version<1)
		return 0;

	// v1

	continuousParameter_t cp;
	for(cp=0;cp<=cpSeqArpClock_Legacy;++cp)
		currentPreset.continuousParameters[cp]=storageRead16();
	storageRead16(); // bw compat fix

	steppedParameter_t sp;
	for(sp=0;sp<=spChromaticPitch;++sp)
		currentPreset.steppedParameters[sp]=storageRead8();
	storageRead8(); // bw compat fix

	for(i=0;i<SYNTH_VOICE_COUNT;++i)
		currentPreset.voicePattern[i]=storageRead8();
	
	// v1 - bw compat adjustments
	
	if(storage.version<3)
	{
		currentPreset.continuousParameters[cpBFreq]=(currentPreset.continuousParameters[cpBFreq]>>1)+HALF_RANGE;
	}

	if(storage.version<11)
	{
		synth_reloadLegacyBankWaveIndexes(0,0,0);
		synth_reloadLegacyBankWaveIndexes(1,0,0);
	}
	
	if(storage.version<13)
	{
		for(cp=cpFilAtt;cp<=cpFilRel;++cp)
			currentPreset.continuousParameters[cp+cpWModAtt-cpFilAtt]=currentPreset.continuousParameters[cp];
		if(currentPreset.steppedParameters[spBWModEnvEn_Legacy])
			currentPreset.continuousParameters[cpWModBEnv]=currentPreset.continuousParameters[cpWModAEnv];
		if(!currentPreset.steppedParameters[spAWModEnvEn_Legacy])
			currentPreset.continuousParameters[cpWModAEnv]=HALF_RANGE;
		currentPreset.continuousParameters[cpWModVelocity]=currentPreset.continuousParameters[cpFilVelocity];
		currentPreset.steppedParameters[spWModEnvSlow]=currentPreset.steppedParameters[spFilEnvSlow];
	}
		
	if(storage.version<14)
	{
		if(currentPreset.continuousParameters[cpBFreq]>=HALF_RANGE)
		{
			currentPreset.continuousParameters[cpAFreq]=0;
			currentPreset.continuousParameters[cpBFreq]=(currentPreset.continuousParameters[cpBFreq]-HALF_RANGE)<<1;
		}
		else
		{
			currentPreset.continuousParameters[cpAFreq]=(HALF_RANGE-currentPreset.continuousParameters[cpBFreq])<<1;
			currentPreset.continuousParameters[cpBFreq]=0;
		}
	}
		
	if(storage.version<15)
	{
		if(currentPreset.steppedParameters[spAWModType]==wmAliasing)
			currentPreset.continuousParameters[cpABaseWMod]=HALF_RANGE-(currentPreset.continuousParameters[cpABaseWMod]>>1);
		
		if(currentPreset.steppedParameters[spBWModType]==wmAliasing)
			currentPreset.continuousParameters[cpBBaseWMod]=HALF_RANGE-(currentPreset.continuousParameters[cpBBaseWMod]>>1);
	}
	
	if(storage.version<16)
	{
		// adjust cutoff for base filter tracking note changed to C4
		currentPreset.continuousParameters[cpCutoff]+=scaleU16U16(MIDDLE_C_NOTE<<8,currentPreset.continuousParameters[cpFilKbdAmt]);
	}

	
	if(storage.version<2)
		return 1;

	// v2
	
	if(storage.version<4)
		return 1;

	// v4
	
	currentPreset.continuousParameters[cpNoiseVol]=storageRead16();
	
	if(storage.version<5)
		return 1;
	
	// v5
	
	currentPreset.steppedParameters[spOscSync]=storageRead8();

	if(storage.version<8)
		return 1;
	
	// v8
	
	currentPreset.steppedParameters[spXOvrBank_Legacy]=storageRead8();
	currentPreset.steppedParameters[spXOvrWave_Legacy]=storageRead8();
	currentPreset.steppedParameters[spFilEnvLin]=storageRead8();

	// v8 - bw compat adjustments
	
	currentPreset.steppedParameters[spWModEnvLin]=currentPreset.steppedParameters[spFilEnvLin];
	synth_reloadLegacyBankWaveIndexes(2,0,0);

	if(storage.version<9)
		return 1;
	
	// v9

	currentPreset.continuousParameters[cpLFO2PitchAmt]=storageRead16();
	currentPreset.continuousParameters[cpLFO2WModAmt]=storageRead16();
	currentPreset.continuousParameters[cpLFO2FilAmt]=storageRead16();
	currentPreset.continuousParameters[cpLFO2AmpAmt]=storageRead16();
	currentPreset.continuousParameters[cpLFOResAmt]=storageRead16();
	currentPreset.continuousParameters[cpLFO2ResAmt]=storageRead16();
	
	currentPreset.steppedParameters[spLFO2Shape]=storageRead8();
	currentPreset.steppedParameters[spLFO2Shift]=storageRead8();
	currentPreset.steppedParameters[spLFO2Targets]=storageRead8();
	
	if(storage.version<10)
		return 1;
	
	// v10

	currentPreset.steppedParameters[spVoiceCount]=storageRead8();

	if(storage.version<11)
		return 1;
	
	// v11
	
	for(int8_t abx=0;abx<PRESET_BANKWAVE_ABX_COUNT;++abx)
	{
		storageReadStr(currentPreset.oscBank[abx]);
		storageReadStr(currentPreset.oscWave[abx]);
	}

	currentPreset.steppedParameters[spPresetType]=storageRead8();
	currentPreset.steppedParameters[spPresetStyle]=storageRead8();

	if(storage.version<12)
		return 1;

	// v12
	
	currentPreset.steppedParameters[spAmpEnvLin]=storageRead8();
	currentPreset.steppedParameters[spFilEnvLoop]=storageRead8();
	currentPreset.steppedParameters[spAmpEnvLoop]=storageRead8();

	// v12 - bw compat adjustments
	
	currentPreset.steppedParameters[spWModEnvLoop]=currentPreset.steppedParameters[spFilEnvLoop];

	if(storage.version<13)
		return 1;

	// v13
	
	for(cp=cpWModAtt;cp<=cpWModVelocity;++cp)
		currentPreset.continuousParameters[cp]=storageRead16();

	currentPreset.steppedParameters[spWModEnvSlow]=storageRead8();
	currentPreset.steppedParameters[spWModEnvLin]=storageRead8();
	currentPreset.steppedParameters[spWModEnvLoop]=storageRead8();

	return 1;
}

LOWERCODESIZE void preset_saveCurrent(uint16_t number)
{
	int8_t i;
	
	storagePrepareStore();

	// v1

	continuousParameter_t cp;
	for(cp=0;cp<=cpSeqArpClock_Legacy;++cp)
		storageWrite16(currentPreset.continuousParameters[cp]);
	storageWrite16(0); // bw compat fix

	steppedParameter_t sp;
	for(sp=0;sp<=spChromaticPitch;++sp)
		storageWrite8(currentPreset.steppedParameters[sp]);
	storageWrite8(0); // bw compat fix

	for(i=0;i<SYNTH_VOICE_COUNT;++i)
		storageWrite8(currentPreset.voicePattern[i]);

	// v4
	
	storageWrite16(currentPreset.continuousParameters[cpNoiseVol]);
	
	// v5
	
	storageWrite8(currentPreset.steppedParameters[spOscSync]);
	
	// v8
	
	storageWrite8(currentPreset.steppedParameters[spXOvrBank_Legacy]);
	storageWrite8(currentPreset.steppedParameters[spXOvrWave_Legacy]);
	storageWrite8(currentPreset.steppedParameters[spFilEnvLin]);
	
	// v9

	storageWrite16(currentPreset.continuousParameters[cpLFO2PitchAmt]);
	storageWrite16(currentPreset.continuousParameters[cpLFO2WModAmt]);
	storageWrite16(currentPreset.continuousParameters[cpLFO2FilAmt]);
	storageWrite16(currentPreset.continuousParameters[cpLFO2AmpAmt]);
	storageWrite16(currentPreset.continuousParameters[cpLFOResAmt]);
	storageWrite16(currentPreset.continuousParameters[cpLFO2ResAmt]);
	
	storageWrite8(currentPreset.steppedParameters[spLFO2Shape]);
	storageWrite8(currentPreset.steppedParameters[spLFO2Shift]);
	storageWrite8(currentPreset.steppedParameters[spLFO2Targets]);

	// v10
	
	storageWrite8(currentPreset.steppedParameters[spVoiceCount]);

	// v11
	
	for(int8_t abx=0;abx<PRESET_BANKWAVE_ABX_COUNT;++abx)
	{
		storageWriteStr(currentPreset.oscBank[abx]);
		storageWriteStr(currentPreset.oscWave[abx]);
	}
	
	storageWrite8(currentPreset.steppedParameters[spPresetType]);
	storageWrite8(currentPreset.steppedParameters[spPresetStyle]);
		
	// v12

	storageWrite8(currentPreset.steppedParameters[spAmpEnvLin]);
	storageWrite8(currentPreset.steppedParameters[spFilEnvLoop]);
	storageWrite8(currentPreset.steppedParameters[spAmpEnvLoop]);

	// v13

	for(cp=cpWModAtt;cp<=cpWModVelocity;++cp)
		storageWrite16(currentPreset.continuousParameters[cp]);

	storageWrite8(currentPreset.steppedParameters[spWModEnvSlow]);
	storageWrite8(currentPreset.steppedParameters[spWModEnvLin]);
	storageWrite8(currentPreset.steppedParameters[spWModEnvLoop]);
	
	// /!\ write new version before this
	storageFinishStore(number,1);
}

LOWERCODESIZE int8_t storage_loadSequencer(int8_t track, uint8_t * data, uint8_t size)
{
	if (!storageLoad(SEQUENCER_START_PAGE+track+settings.sequencerBank*SEQ_TRACK_COUNT,1))
		return 0;

	while(size--)
		*data++=storageRead8();
	
	return 1;
}

LOWERCODESIZE void storage_saveSequencer(int8_t track, uint8_t * data, uint8_t size)
{
	storagePrepareStore();

	while(size--)
		storageWrite8(*data++);

	// this must stay last
	storageFinishStore(SEQUENCER_START_PAGE+track+settings.sequencerBank*SEQ_TRACK_COUNT,1);
}

LOWERCODESIZE void storage_export(uint16_t number, uint8_t * buf, int16_t * size)
{
	int16_t actualSize;

	storageLoad(number,1);

	// don't export trailing zeroes		

	actualSize=STORAGE_PAGE_SIZE;
	while(storage.buffer[actualSize-1]==0)
		--actualSize;

	buf[0]=number;		
	memcpy(&buf[1],storage.buffer,actualSize);
	*size=actualSize+1;
}

LOWERCODESIZE void storage_import(uint16_t number, uint8_t * buf, int16_t size)
{
	memset(storage.buffer,0,sizeof(storage.buffer));
	memcpy(storage.buffer,buf,size);
	storage.bufPtr=storage.buffer+size;
	storageFinishStore(number,1);
}

LOWERCODESIZE void preset_loadDefault(int8_t makeSound)
{
	int8_t i;

	memset(&currentPreset,0,sizeof(struct preset_s));

	currentPreset.continuousParameters[cpUnisonDetune]=512;
	currentPreset.continuousParameters[cpMasterTune]=HALF_RANGE;

	currentPreset.continuousParameters[cpDetune]=HALF_RANGE;
	currentPreset.continuousParameters[cpABaseWMod]=HALF_RANGE;
	currentPreset.continuousParameters[cpBBaseWMod]=HALF_RANGE;
	currentPreset.continuousParameters[cpCutoff]=UINT16_MAX;
	currentPreset.continuousParameters[cpFilEnvAmt]=HALF_RANGE;
	currentPreset.continuousParameters[cpLFOPitchAmt]=UINT16_MAX/16;
	currentPreset.continuousParameters[cpLFOFreq]=HALF_RANGE;
	currentPreset.continuousParameters[cpLFO2Freq]=HALF_RANGE;
	currentPreset.continuousParameters[cpAmpSus]=UINT16_MAX;

	currentPreset.continuousParameters[cpWModAEnv]=HALF_RANGE;
	currentPreset.continuousParameters[cpWModBEnv]=HALF_RANGE;

	currentPreset.steppedParameters[spBenderRange]=2; // octave
	currentPreset.steppedParameters[spBenderTarget]=modFilter;
	currentPreset.steppedParameters[spModwheelRange]=2; // high
	currentPreset.steppedParameters[spChromaticPitch]=2; // octave
	currentPreset.steppedParameters[spAssignerPriority]=apLast;
	currentPreset.steppedParameters[spLFOShape]=lsTri;
	currentPreset.steppedParameters[spLFOTargets]=otBoth;
	currentPreset.steppedParameters[spLFOShift]=1;
	currentPreset.steppedParameters[spLFO2Shape]=lsTri;
	currentPreset.steppedParameters[spLFO2Targets]=otBoth;
	currentPreset.steppedParameters[spLFO2Shift]=1;

	currentPreset.steppedParameters[spVoiceCount]=5;
	
	for(i=0;i<SYNTH_VOICE_COUNT;++i)
		currentPreset.voicePattern[i]=(i==0)?0:ASSIGNER_NO_NOTE;	

	if(makeSound)
	{
		// load default waveforms (perfectwaves/sawtooth)
		for(int8_t abx=0;abx<=2;++abx)
			synth_reloadLegacyBankWaveIndexes(abx,1,1);

		currentPreset.continuousParameters[cpAVol]=UINT16_MAX;
	}
}

LOWERCODESIZE void settings_loadDefault(void)
{
	memset(&settings,0,sizeof(settings));

	settings.midiReceiveChannel=-1;
	settings.voiceMask=0x3f;
	settings.seqArpClock=6*UINT16_MAX/10+1;
	settings.lcdContrast=UI_DEFAULT_LCD_CONTRAST;

	tuner_init(); // use theoretical tuning
}

LOWERCODESIZE void preset_upgradeBankWaveStorage(void)
{
	if(settings_load() && storage.version<11)
	{
		rprintf(1,"Upgrading presets...");

		for(uint16_t p=0;p<400;++p)
		{
			rprintf(0,"Upgrading preset %d ",p);
			
			preset_loadDefault(0);
			if(preset_loadCurrent(p))
			{
				if(storage.version<11)
				{
					preset_saveCurrent(p);
					rprintf(0,"Done!\n");
				}
				else
				{
					rprintf(0,"Already upgraded...\n");
				}
			}
			else
			{
				rprintf(0,"Not found...\n",p);
			}
		}
		
		settings_save();
	}
}

LOWERCODESIZE void preset_packAndRemoveDuplicates(void)
{
	int16_t to=0;
	for(int16_t p=0;p<400;++p)
	{
		rprintf(1,"Packing pass 1 (% 3d)...\n               ", p);
	
		if(storage_pageExists(p))
		{
			if(p!=to)
			{
				storage_read(p, storage.buffer);
				storage_write(to, storage.buffer);
				storage_delete(p);
			}
			++to;
		}
	}

	for(int16_t p=0;p<=to;++p)
	{
		rprintf(1,"Removing duplicates (% 3d)...            ", p);

		if(storage_pageExists(p))
			for(int16_t q=p+1;q<=to;++q)
				if(storage_samePage(p, q))
					storage_delete(q);
	}

	to=0;
	for(int16_t p=0;p<400;++p)
	{
		rprintf(1,"Packing pass 2 (% 3d)...\n               ", p);
	
		if(storage_pageExists(p))
		{
			if(p!=to)
			{
				storage_read(p, storage.buffer);
				storage_write(to, storage.buffer);
				storage_delete(p);
			}
			++to;
		}
	}
}
