////////////////////////////////////////////////////////////////////////////////
// Presets and settings storage, relies on low level page storage system
////////////////////////////////////////////////////////////////////////////////

#include "storage.h"
#include "lfo.h"
#include "wtosc.h"

// increment this each time the binary format is changed
#define STORAGE_VERSION 5

#define STORAGE_MAGIC 0x006116a5
#define STORAGE_MAX_SIZE 512

#define SETTINGS_PAGE_COUNT 2
#define SETTINGS_PAGE ((STORAGE_SIZE/STORAGE_PAGE_SIZE)-4)

#define SEQUENCER_START_PAGE 65000

const uint8_t steppedParametersBits[spCount] = 
{
	/*ABank*/7,
	/*AWave*/7,
	/*ABaseWMod*/2,
	/*AFilWMod*/1,
	/*BBank*/7,
	/*BWave*/7,
	/*BBaseWMod*/2,
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
		print("Error: bad page !\n"); 
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
#ifdef DEBUG
		print("Error: writing too much data to storage !\n"); 
#endif	
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

	if (storage.version<3)
		return 1;
		
	// v3
	
	// ...
	
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
	
	// v3
		
	// ...

	// this must stay last
	storageFinishStore(SETTINGS_PAGE,SETTINGS_PAGE_COUNT);
}

LOWERCODESIZE int8_t preset_loadCurrent(uint16_t number)
{
	int8_t i;
	
	storageLoad(number,1);

	currentPreset.continuousParameters[cpNoiseVol]=0;
	currentPreset.steppedParameters[spOscSync]=0;

	if (storage.version<1)
		return 0;

	// v1

	continuousParameter_t cp;
	for(cp=0;cp<=cpSeqArpClock;++cp)
		currentPreset.continuousParameters[cp]=storageRead16();
	storageRead16(); // bw compat fix

	steppedParameter_t sp;
	for(sp=0;sp<=spChromaticPitch;++sp)
		currentPreset.steppedParameters[sp]=storageRead8();
	storageRead8(); // bw compat fix

	for(i=0;i<SYNTH_VOICE_COUNT;++i)
		currentPreset.voicePattern[i]=storageRead8();
	
	if (storage.version<2)
		return 1;

	// v2
	
	if(storage.version<3)
		currentPreset.continuousParameters[cpBFreq]=(currentPreset.continuousParameters[cpBFreq]/2)+HALF_RANGE;

	if(storage.version<4)
		return 1;

	// v4
	
	currentPreset.continuousParameters[cpNoiseVol]=storageRead16();
	
	if(storage.version<5)
		return 1;
	
	// v5
	
	currentPreset.steppedParameters[spOscSync]=storageRead8();

	return 1;
}

LOWERCODESIZE void preset_saveCurrent(uint16_t number)
{
	int8_t i;
	
	storagePrepareStore();

	// v1

	continuousParameter_t cp;
	for(cp=0;cp<=cpSeqArpClock;++cp)
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
	
	// this must stay last
	storageFinishStore(number,1);
}

LOWERCODESIZE int8_t storage_loadSequencer(int8_t track, uint8_t * data, uint8_t size)
{
	BLOCK_INT
	{
		if (!storageLoad(SEQUENCER_START_PAGE+track,1))
			return 0;
		
		while(size--)
			*data++=storageRead8();
	}
	
	return 1;
}

LOWERCODESIZE void storage_saveSequencer(int8_t track, uint8_t * data, uint8_t size)
{
	BLOCK_INT
	{
		storagePrepareStore();

		while(size--)
			storageWrite8(*data++);
		
		// this must stay last
		storageFinishStore(SEQUENCER_START_PAGE+track,1);
	}
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
	currentPreset.continuousParameters[cpMasterLeft]=HALF_RANGE;
	currentPreset.continuousParameters[cpMasterRight]=HALF_RANGE;

	currentPreset.continuousParameters[cpBFreq]=HALF_RANGE;
	currentPreset.continuousParameters[cpBFineFreq]=HALF_RANGE;
	currentPreset.continuousParameters[cpABaseWMod]=HALF_RANGE;
	currentPreset.continuousParameters[cpBBaseWMod]=HALF_RANGE;
	currentPreset.continuousParameters[cpCutoff]=UINT16_MAX;
	currentPreset.continuousParameters[cpFilEnvAmt]=HALF_RANGE;
	currentPreset.continuousParameters[cpLFOPitchAmt]=UINT16_MAX/16;
	currentPreset.continuousParameters[cpLFOFreq]=HALF_RANGE;
	currentPreset.continuousParameters[cpAmpSus]=UINT16_MAX;
	currentPreset.continuousParameters[cpVibFreq]=8*UINT16_MAX/10+1;

	currentPreset.steppedParameters[spBenderRange]=1;
	currentPreset.steppedParameters[spBenderTarget]=modPitch;
	currentPreset.steppedParameters[spModwheelRange]=2;
	currentPreset.steppedParameters[spChromaticPitch]=2; // octave
	currentPreset.steppedParameters[spAssignerPriority]=apLast;
	currentPreset.steppedParameters[spLFOShape]=lsTri;
	currentPreset.steppedParameters[spLFOTargets]=otBoth;
	currentPreset.steppedParameters[spLFOShift]=1;

	currentPreset.steppedParameters[spABank]=26; // perfectwaves (saw)
	currentPreset.steppedParameters[spBBank]=26;

	for(i=0;i<SYNTH_VOICE_COUNT;++i)
		currentPreset.voicePattern[i]=(i==0)?0:ASSIGNER_NO_NOTE;	

	if(makeSound)
	{
		currentPreset.continuousParameters[cpAVol]=UINT16_MAX;
	}
}

LOWERCODESIZE void settings_loadDefault(void)
{
	memset(&settings,0,sizeof(settings));

	settings.midiReceiveChannel=-1;
	settings.voiceMask=0x3f;

	tuner_init(); // use theoretical tuning
}
