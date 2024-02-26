////////////////////////////////////////////////////////////////////////////////
// Presets and settings storage, relies on low level page storage system
////////////////////////////////////////////////////////////////////////////////

#include "storage.h"
#include "lfo.h"
#include "seq.h"
#include "ui.h"
#include "clock.h"
#include "scan.h"
#include "dacspi.h"
#include "main.h"
#include "midi.h"
#include "ff.h"

#include <ctype.h>
#include <alloca.h>

#define SAVE_INT " = %d\n"
#define SAVE_STR " = %s\n"

const struct namedParam_s continuousParametersZeroCentered[cpCount] = 
{
	{"cpAFreq",0},
	{"cpAVol",0},
	{"cpABaseWMod",1},
	{"cpBFreq",0},
	{"cpBVol",0},
	{"cpBBaseWMod",1},
	{"cpDetune",1},
	{"cpCutoff",0},
	{"cpResonance",0},
	{"cpFilEnvAmt",1},
	{"cpFilKbdAmt",0},
	{"cpWModAEnv",1},
	{"cpFilAtt",0},
	{"cpFilDec",0},
	{"cpFilSus",0},
	{"cpFilRel",0},
	{"cpAmpAtt",0},
	{"cpAmpDec",0},
	{"cpAmpSus",0},
	{"cpAmpRel",0},
	{"cpLFOFreq",0},
	{"cpLFOAmt",0},
	{"cpLFOPitchAmt",0},
	{"cpLFOWModAmt",0},
	{"cpLFOFilAmt",0},
	{"cpLFOAmpAmt",0},
	{"cpLFO2Freq",0},
	{"cpLFO2Amt",0},
	{"cpModDelay",0},
	{"cpGlide",0},
	{"cpAmpVelocity",0},
	{"cpFilVelocity",0},
	{"cpMasterTune",1},
	{"cpUnisonDetune",0},
	{NULL,0},
	{NULL,0},
	{NULL,0},
	{"cpNoiseVol",0},
	{"cpLFO2PitchAmt",0},
	{"cpLFO2WModAmt",0},
	{"cpLFO2FilAmt",0},
	{"cpLFO2AmpAmt",0},
	{"cpLFOResAmt",0},
	{"cpLFO2ResAmt",0},
	{"cpWModAtt",0},
	{"cpWModDec",0},
	{"cpWModSus",0},
	{"cpWModRel",0},
	{"cpWModBEnv",1},
	{"cpWModVelocity",0},
};

const struct namedParam_s steppedParametersSteps[spCount] = 
{
	{NULL,128},
	{NULL,128},
	{"spABaseWMod",5},
	{NULL,1},
	{NULL,128},
	{NULL,128},
	{"spBBaseWMod",5},
	{NULL,1},
	{"spLFOShape",7},
	{"spLFOSpeed",4},
	{"spLFOTargets",4},
	{"spFilEnvSlow",2},
	{"spAmpEnvSlow",2},
	{"spBenderRange",3},
	{"spBenderTarget",5},
	{"spModwheelRange",4},
	{"spModwheelTarget",2},
	{"spUnison",2},
	{"spAssignerPriority",3},
	{"spChromaticPitch",3},
	{"spSync",2},
	{NULL,128},
	{NULL,128},
	{"spFilEnvLin",2},
	{"spLFO2Shape",7},
	{"spLFO2Speed",4},
	{"spLFO2Targets",4},
	{"spVoiceCount",SYNTH_VOICE_COUNT},
	{"spPresetType",8},
	{"spPresetStyle",8},
	{"spAmpEnvLin",2},
	{"spFilEnvLoop",2},
	{"spAmpEnvLoop",2},
	{"spWModEnvSlow",2},
	{"spWModEnvLin",2},
	{"spWModEnvLoop",2},
	{"spPressureRange",4},
	{"spPressureTarget",7},
	{NULL,128},
	{NULL,128},
};

struct settings_s settings;
struct preset_s currentPreset;

struct loadLL_s
{
	char *name,*strValue;
	struct loadLL_s *next;
};

typedef void (*parse_callback_t)(struct loadLL_s * ll);

LOWERCODESIZE static FRESULT prepareConfigFileSave(FIL *f, const char * fn)
{
	FRESULT res;
	res=f_open(f,fn,FA_WRITE|FA_CREATE_ALWAYS);
	if(res)
		return res;
		
	f_printf(f,"# %s %s\n", synthName, synthVersion);	
	return 0;
}

LOWERCODESIZE static FRESULT parseConfigFile(const char * fn, parse_callback_t callback)
{
	FIL f;
	FRESULT res;
	struct loadLL_s *ll, *prev_ll=NULL, *first_ll=NULL;
	char line[256];
	char *p, *locName, *locValue;
	
	res=f_open(&f,fn,FA_READ|FA_OPEN_EXISTING);
	if(res)
		return res;

	while(f_gets(line,sizeof(line),&f))
	{
		// alloc on stack, clear
		
		ll=alloca(sizeof(struct loadLL_s));
		memset(ll,0,sizeof(struct loadLL_s));

		// parse line
		
		locName=NULL;
		locValue=NULL;
		
			// remove comments
		p=strchr(line,'#');
		if(p) *p='\n';
				
			// remove lf and trim (value) right
		p=strchr(line,'\n');
		if(p)
		{
			*p--='\0';
			while(isspace(*p)) *p--='\0';
		}
		
			// trim name left
		locName=line;
		while(isspace(*locName)) ++locName;
		
		p=strchr(locName,'=');
		if(p)
		{
			locValue=p+1;
			
			// trim name right
			while(isspace(*p) || *p=='=') *p--='\0';

			// trim value left
			while(isspace(*locValue)) ++locValue;
		}
		
		// alloc strings
		
		if(locName)
		{
			ll->name=alloca(strlen(locName)+1);
			strcpy(ll->name,locName);
		}
		
		if(locValue)
		{
			ll->strValue=alloca(strlen(locValue)+1);
			strcpy(ll->strValue,locValue);
		}
		
		// handle linked list
		
		if(!first_ll)
			first_ll=ll;
		
		if(prev_ll)
			prev_ll->next=ll;
		prev_ll=ll;
	}
	
	if(callback) callback(first_ll);
	
	f_close(&f);
	
	return 0;
}

LOWERCODESIZE static const char * getStrValue(struct loadLL_s *ll, const char * name)
{
	while(ll && strcmp(name, ll->name)) ll=ll->next;
	
	if(ll)
		return ll->strValue;
	else
		return NULL;
}

LOWERCODESIZE static void getIntValue(struct loadLL_s *ll, const char * name, void * default_, int size)
{
	const char *sv = getStrValue(ll,name);
	if(sv && default_)
	{
		int v=atoi(sv);
		memcpy(default_,&v,size);
	}
}

LOWERCODESIZE static void getSafeIntValue(struct loadLL_s *ll, const char * name, void * default_, int size, int min, int max)
{
	const char *sv = getStrValue(ll,name);
	if(sv && default_)
	{
		int v=MAX(min,MIN(max,atoi(sv)));
		memcpy(default_,&v,size);
	}
}

LOWERCODESIZE static void getContinuousValue(struct loadLL_s *ll, continuousParameter_t cp)
{
	const struct namedParam_s *np=&continuousParametersZeroCentered[cp];
	int v=SCAN_POT_FROM_16BITS(currentPreset.continuousParameters[cp]);
	getIntValue(ll,np->name,&v,sizeof(v));

	v=SCAN_POT_TO_16BITS(v);
	if(np->param) v-=INT16_MIN;
	
	currentPreset.continuousParameters[cp]=MAX(0,MIN(UINT16_MAX,v));
}

LOWERCODESIZE static void getSteppedValue(struct loadLL_s *ll, steppedParameter_t sp)
{
	const struct namedParam_s *np=&steppedParametersSteps[sp];
	int v=currentPreset.steppedParameters[sp];
	getIntValue(ll,np->name,&v,sizeof(v));
	
	currentPreset.steppedParameters[sp]=MAX(0,MIN(np->param-1,v));
}

LOWERCODESIZE int8_t settings_load(void)
{
	auto void load(struct loadLL_s * ll)
	{
		char buf[32];

		getSafeIntValue(ll,"presetNumber",&settings.presetNumber,sizeof(settings.presetNumber),0,999);
		getSafeIntValue(ll,"midiReceiveChannel",&settings.midiReceiveChannel,sizeof(settings.midiReceiveChannel),-1,MIDI_CHANMASK);
		getSafeIntValue(ll,"voiceMask",&settings.voiceMask,sizeof(settings.voiceMask),0,(1<<SYNTH_VOICE_COUNT)-1);
		getSafeIntValue(ll,"syncMode",&settings.syncMode,sizeof(settings.syncMode),0,symCount-1);
		getSafeIntValue(ll,"sequencerBank",&settings.sequencerBank,sizeof(settings.sequencerBank),0,SEQ_BANK_COUNT-1);
		getSafeIntValue(ll,"seqArpClock",&settings.seqArpClock,sizeof(settings.seqArpClock),0,CLOCK_MAX_BPM);
		getSafeIntValue(ll,"usbMIDI",&settings.usbMIDI,sizeof(settings.usbMIDI),0,1);
		getSafeIntValue(ll,"lcdContrast",&settings.lcdContrast,sizeof(settings.lcdContrast),0,UI_MAX_LCD_CONTRAST);

		for(int8_t i=0;i<TUNER_CV_COUNT;++i)
			for(int8_t j=0;j<TUNER_OCTAVE_COUNT;++j)
			{
				srprintf(buf,"tune_v%d_o%d",i,j);
				getSafeIntValue(ll,buf,&settings.tunes[j][i],sizeof(settings.tunes[j][i]),0,UINT16_MAX);
			}
	}
	
	settings_loadDefault();

	if(parseConfigFile("/overcycler.conf",load))
		return 0;
	else
		return 1;
}

LOWERCODESIZE void settings_save(void)
{
	FIL f;
	if(prepareConfigFileSave(&f,"/overcycler.conf"))
		return;

	f_printf(&f,"presetNumber" SAVE_INT,settings.presetNumber);
	f_printf(&f,"midiReceiveChannel" SAVE_INT,settings.midiReceiveChannel);
	f_printf(&f,"voiceMask" SAVE_INT,settings.voiceMask);
	f_printf(&f,"syncMode" SAVE_INT,settings.syncMode);
	f_printf(&f,"sequencerBank" SAVE_INT,settings.sequencerBank);
	f_printf(&f,"seqArpClock" SAVE_INT,settings.seqArpClock);
	f_printf(&f,"usbMIDI" SAVE_INT,settings.usbMIDI);
	f_printf(&f,"lcdContrast" SAVE_INT,settings.lcdContrast);
	
	for(int8_t i=0;i<TUNER_CV_COUNT;++i)
		for(int8_t j=0;j<TUNER_OCTAVE_COUNT;++j)
			f_printf(&f,"tune_v%d_o%d" SAVE_INT,i,j,settings.tunes[j][i]);

	f_close(&f);
}

LOWERCODESIZE void settings_loadDefault(void)
{
	memset(&settings,0,sizeof(settings));

	settings.midiReceiveChannel=-1;
	settings.voiceMask=(1<<SYNTH_VOICE_COUNT)-1;
	settings.seqArpClock=CLOCK_MAX_BPM/2;
	settings.lcdContrast=UI_DEFAULT_LCD_CONTRAST;

	tuner_init(); // use theoretical tuning
}

LOWERCODESIZE int8_t storage_loadSequencer(int8_t track, uint8_t * data, uint8_t size)
{
	auto void load(struct loadLL_s * ll)
	{
		char buf[32];
		for(uint8_t i=0;i<size;++i)
		{
			srprintf(buf,"step%02x",i);
			getSafeIntValue(ll,buf,&data[i],sizeof(data[i]),0,UINT8_MAX);
		}
	}

	char fn[256];
	srprintf(fn,SYNTH_SEQUENCES_PATH "/sequence_%02d%c.conf",settings.sequencerBank,'a'+track);
	if(parseConfigFile(fn,load))
		return 0;
	else
		return 1;
}

LOWERCODESIZE void storage_saveSequencer(int8_t track, uint8_t * data, uint8_t size)
{
	FIL f;
	char buf[256];
	
	f_mkdir(SYNTH_SEQUENCES_PATH);
	
	srprintf(buf,SYNTH_SEQUENCES_PATH "/sequence_%02d%c.conf",settings.sequencerBank,'a'+track);
	if(prepareConfigFileSave(&f,buf))
		return;

	for(uint8_t i=0;i<size;++i)
		f_printf(&f,"step%02x" SAVE_INT,i,data[i]);
	
	f_close(&f);
}

LOWERCODESIZE int8_t preset_loadCurrent(uint16_t number)
{
	auto void load(struct loadLL_s * ll)
	{
		char buf[32];

		for(abx_t abx=0;abx<abxCount;++abx)
		{
			srprintf(buf,"bank%d",abx);
			strcpy(currentPreset.oscBank[abx],getStrValue(ll,buf));
			srprintf(buf,"wave%d",abx);
			strcpy(currentPreset.oscWave[abx],getStrValue(ll,buf));
		}

		for(continuousParameter_t cp=0;cp<cpCount;++cp)
			getContinuousValue(ll,cp);

		for(steppedParameter_t sp=0;sp<spCount;++sp)
			getSteppedValue(ll,sp);

		for(int8_t i=0;i<SYNTH_VOICE_COUNT;++i)
		{
			srprintf(buf,"voicePattern%d",i);
			getSafeIntValue(ll,buf,&currentPreset.voicePattern[i],sizeof(currentPreset.voicePattern[i]),0,UINT8_MAX);
		}
	}
	
	preset_loadDefault(1);

	char fn[256];
	srprintf(fn,SYNTH_PRESETS_PATH "/preset_%04d.conf",number);
	if(parseConfigFile(fn,load))
		return 0;
	else
		return 1;
}

LOWERCODESIZE void preset_saveCurrent(uint16_t number)
{
	FIL f;
	char buf[256];
	
	f_mkdir(SYNTH_PRESETS_PATH);
	
	srprintf(buf,SYNTH_PRESETS_PATH "/preset_%04d.conf",number);
	if(prepareConfigFileSave(&f,buf))
		return;

	for(abx_t abx=0;abx<abxCount;++abx)
	{
		f_printf(&f,"bank%d" SAVE_STR,abx,currentPreset.oscBank[abx]);
		f_printf(&f,"wave%d" SAVE_STR,abx,currentPreset.oscWave[abx]);
	}
	
	for(continuousParameter_t cp=0;cp<cpCount;++cp)
		if(continuousParametersZeroCentered[cp].name)
			f_printf(&f,"%s" SAVE_INT,
				continuousParametersZeroCentered[cp].name,
				SCAN_POT_FROM_16BITS(currentPreset.continuousParameters[cp]+(continuousParametersZeroCentered[cp].param?INT16_MIN:0)));

	for(steppedParameter_t sp=0;sp<spCount;++sp)
		if(steppedParametersSteps[sp].name)
			f_printf(&f,"%s" SAVE_INT,
				steppedParametersSteps[sp].name,
				currentPreset.steppedParameters[sp]);

	for(int8_t i=0;i<SYNTH_VOICE_COUNT;++i)
		f_printf(&f,"voicePattern%d" SAVE_INT,i,currentPreset.voicePattern[i]);
	
	f_close(&f);
}

LOWERCODESIZE int8_t preset_fileExists(uint16_t number)
{
	FIL f;
	FRESULT res;
	char fn[256];
	srprintf(fn,SYNTH_PRESETS_PATH "/preset_%04d.conf",number);
	
	res=f_open(&f,fn,FA_READ|FA_OPEN_EXISTING);
	if(res)
		return res!=FR_NO_FILE;

	f_close(&f);
	return 1;
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
	currentPreset.continuousParameters[cpWModAEnv]=HALF_RANGE;
	currentPreset.continuousParameters[cpWModBEnv]=HALF_RANGE;
	currentPreset.continuousParameters[cpCutoff]=UINT16_MAX;
	currentPreset.continuousParameters[cpFilEnvAmt]=HALF_RANGE;
	currentPreset.continuousParameters[cpAmpSus]=UINT16_MAX;
	currentPreset.continuousParameters[cpLFOPitchAmt]=SCAN_POT_TO_16BITS(100);
	currentPreset.continuousParameters[cpLFOFreq]=SCAN_POT_TO_16BITS(5*60);
	currentPreset.continuousParameters[cpLFO2Freq]=SCAN_POT_TO_16BITS(5*60);

	currentPreset.steppedParameters[spBenderTarget]=modPitch;
	currentPreset.steppedParameters[spModwheelRange]=1; // low
	currentPreset.steppedParameters[spChromaticPitch]=2; // octave
	currentPreset.steppedParameters[spAssignerPriority]=apLast;
	currentPreset.steppedParameters[spLFOShape]=lsTri;
	currentPreset.steppedParameters[spLFOTargets]=otBoth;
	currentPreset.steppedParameters[spLFO2Shape]=lsTri;
	currentPreset.steppedParameters[spLFO2Targets]=otBoth;
	currentPreset.steppedParameters[spPressureRange]=1; // low
	currentPreset.steppedParameters[spPressureTarget]=modFilter;

	currentPreset.steppedParameters[spVoiceCount]=SYNTH_VOICE_COUNT-1;
	
	for(i=0;i<SYNTH_VOICE_COUNT;++i)
		currentPreset.voicePattern[i]=(i==0)?0:ASSIGNER_NO_NOTE;	

	for(abx_t abx=0;abx<abxCount;++abx)
	{
		strcpy(currentPreset.oscBank[abx],SYNTH_DEFAULT_WAVE_BANK);
		strcpy(currentPreset.oscWave[abx],SYNTH_DEFAULT_WAVE_NAME);
	}

	if(makeSound)
	{
		currentPreset.continuousParameters[cpAVol]=HALF_RANGE;
	}
}
