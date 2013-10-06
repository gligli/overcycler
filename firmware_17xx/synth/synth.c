///////////////////////////////////////////////////////////////////////////////
// Synthesizer
///////////////////////////////////////////////////////////////////////////////

#include "synth.h"
#include "wtosc.h"
#include "dacspi.h"
#include "ff.h"
#include "ui.h"
#include "uart_midi.h"
#include "midi.h"
#include "adsr.h"
#include "lfo.h"
#include "tuner.h"
#include "assigner.h"
#include "arp.h"
#include "storage.h"

#define BANK "test"
#define BANK_DIR "/WAVEDATA/" BANK

#define POT_DEAD_ZONE 512

#define CVMUX_PORT_ABC 0
#define CVMUX_PIN_A 19
#define CVMUX_PIN_B 20
#define CVMUX_PIN_C 21

#define CVMUX_PORT_CARD0 0
#define CVMUX_PIN_CARD0 22

#define CVMUX_PORT_CARD1 0
#define CVMUX_PIN_CARD1 10

#define CVMUX_PORT_CARD2 4
#define CVMUX_PIN_CARD2 29

#define CVMUX_PORT_VCA 4
#define CVMUX_PIN_VCA 28

// soft voice to hard voice (mapped to that the upper the voice, the stronger the panning is)
const int8_t synth_voiceLayout[2][SYNTH_VOICE_COUNT] =
{
	{0,1,3,4,2,5}, //  for DACs
	{2,3,1,4,0,5}, //  for VCAs
};

volatile uint32_t currentTick=0; // 500hz

struct
{
	struct adsr_s filEnvs[SYNTH_VOICE_COUNT];
	struct adsr_s ampEnvs[SYNTH_VOICE_COUNT];

	struct lfo_s lfo,vibrato;
	
	uint16_t oscANoteCV[SYNTH_VOICE_COUNT];
	uint16_t oscBNoteCV[SYNTH_VOICE_COUNT];
	uint16_t filterNoteCV[SYNTH_VOICE_COUNT]; 
	
	uint16_t oscATargetCV[SYNTH_VOICE_COUNT];
	uint16_t oscBTargetCV[SYNTH_VOICE_COUNT];
	uint16_t filterTargetCV[SYNTH_VOICE_COUNT];

	int16_t benderCVs[cvAmp-cvAVol+1];

	int16_t benderAmount;
	uint16_t modwheelAmount;
	int16_t glideAmount;
	int8_t gliding;
	
	uint32_t modulationDelayStart;
	uint16_t modulationDelayTickCount;

	struct wtosc_s osc[SYNTH_VOICE_COUNT][2];
	DIR curDir;
	FILINFO curFile;
	char lfname[_MAX_LFN + 1];
} synth;

static void loadWaveTable(void)
{
	int i;
	FIL f;
	FRESULT res;
	char fn[_MAX_LFN];
	
	strcpy(fn,BANK_DIR "/");
	strcat(fn,synth.curFile.lfname);

	rprintf(0,"loading %s %d... ",fn,synth.curFile.fsize);
	
	if(!f_open(&f,fn,FA_READ))
	{

		if((res=f_lseek(&f,0x2c)))
			rprintf(0,"f_lseek res=%d\n",res);

		int16_t data[WTOSC_MAX_SAMPLES];

		if((res=f_read(&f,data,sizeof(data),&i)))
			rprintf(0,"f_lseek res=%d\n",res);

		f_close(&f);

		for(i=0;i<SYNTH_VOICE_COUNT;++i)
		{
			wtosc_setSampleData(&synth.osc[i][0],data,WTOSC_MAX_SAMPLES);
			wtosc_setSampleData(&synth.osc[i][1],data,WTOSC_MAX_SAMPLES);
		}

		rprintf(0,"loaded\n");
	}
}

static void computeTunedCVs(void)
{
	uint16_t cva,cvb,cvf;
	uint8_t note,baseCutoffNote,baseANote,baseBNote,trackingNote;
	int8_t v;

	uint16_t baseAPitch,baseBPitch,baseCutoff;
	int16_t mTune,fineBFreq,detune;

	uint16_t baseAPitchRaw,baseBPitchRaw,baseCutoffRaw,mTuneRaw,fineBFreqRaw,detuneRaw,trackRaw;
	uint8_t chrom;
	
	mTuneRaw=currentPreset.continuousParameters[cpMasterTune];
	fineBFreqRaw=currentPreset.continuousParameters[cpBFineFreq];
	baseCutoffRaw=currentPreset.continuousParameters[cpCutoff];
	baseAPitchRaw=currentPreset.continuousParameters[cpAFreq];
	baseBPitchRaw=currentPreset.continuousParameters[cpBFreq];
	detuneRaw=currentPreset.continuousParameters[cpUnisonDetune];
	trackRaw=currentPreset.continuousParameters[cpFilKbdAmt];
	chrom=currentPreset.steppedParameters[spChromaticPitch];
	
	// compute for oscs & filters
	
	mTune=(mTuneRaw>>7)+INT8_MIN*2;
	fineBFreq=(fineBFreqRaw>>7)+INT8_MIN*2;
	baseCutoff=baseCutoffRaw;
	baseAPitch=baseAPitchRaw>>2;
	baseBPitch=baseBPitchRaw>>2;
	
	baseCutoffNote=baseCutoff>>8;
	baseANote=baseAPitch>>8; // 64 semitones
	baseBNote=baseBPitch>>8;
	
	baseCutoff&=0xff;
	
	if(chrom>0)
	{
		baseAPitch=0;
		baseBPitch=0;
		
		if(chrom>1)
		{
			baseANote-=baseANote%12;
			baseBNote-=baseBNote%12;
		}
	}
	else
	{
		baseAPitch&=0xff;
		baseBPitch&=0xff;
	}

	for(v=0;v<SYNTH_VOICE_COUNT;++v)
	{
		if (!assigner_getAssignment(v,&note))
			continue;
		
		// oscs
		
		cva=satAddU16S32(tuner_computeCVFromNote(v,baseANote+note,baseAPitch,cvAPitch),(int32_t)synth.benderCVs[cvAPitch]+mTune);
		cvb=satAddU16S32(tuner_computeCVFromNote(v,baseBNote+note,baseBPitch,cvBPitch),(int32_t)synth.benderCVs[cvBPitch]+mTune+fineBFreq);
		
		detune=(1+(v>>1))*(v&1?-1:1)*(detuneRaw>>8);
		cva=satAddU16S16(cva,detune);
		cvb=satAddU16S16(cvb,detune);
		
		// filter
		
		trackingNote=baseCutoffNote+((note*(trackRaw>>8))>>8);
			
		cvf=satAddU16S16(tuner_computeCVFromNote(v,trackingNote,baseCutoff,cvCutoff),synth.benderCVs[cvCutoff]);
		
		// glide
		
		if(synth.gliding)
		{
			synth.oscATargetCV[v]=cva;
			synth.oscBTargetCV[v]=cvb;
			synth.filterTargetCV[v]=cvf;

			if(trackRaw<POT_DEAD_ZONE)
				synth.filterNoteCV[v]=cvf; // no glide if no tracking for filter
		}
		else			
		{
			synth.oscANoteCV[v]=cva;
			synth.oscBNoteCV[v]=cvb;
			synth.filterNoteCV[v]=cvf;
		}
				
	}
}

void computeBenderCVs(void)
{
	int32_t bend;

	bend=synth.benderAmount;

	switch(currentPreset.steppedParameters[spBenderTarget])
	{
	case modPitch:
		bend*=tuner_computeCVFromNote(0,currentPreset.steppedParameters[spBenderSemitones]*2,0,cvAPitch)-tuner_computeCVFromNote(0,0,0,cvAPitch);
		bend/=UINT16_MAX;
		synth.benderCVs[cvAPitch]=bend;
		synth.benderCVs[cvBPitch]=bend;
		break;
	case modFil:
		bend*=tuner_computeCVFromNote(0,currentPreset.steppedParameters[spBenderSemitones]*2,0,cvCutoff)-tuner_computeCVFromNote(0,0,0,cvCutoff);
		bend/=UINT16_MAX;
		synth.benderCVs[cvCutoff]=bend;
		break;
	case modAmp:
		bend=(bend*currentPreset.steppedParameters[spBenderSemitones])/12;
		bend/=UINT16_MAX;
		synth.benderCVs[cvAmp]=bend;
		break;
	}
}

static inline void computeGlide(uint16_t * out, const uint16_t target, const uint16_t amount)
{
	uint16_t diff;
	
	if(*out<target)
	{
		diff=target-*out;
		*out+=MIN(amount,diff);
	}
	else if(*out>target)
	{
		diff=*out-target;
		*out-=MIN(amount,diff);
	}
}

static void refreshModulationDelay(int8_t refreshTickCount)
{
	int8_t anyPressed;
	
	anyPressed=assigner_getAnyPressed();	
	
	if(!anyPressed)
	{
		synth.modulationDelayStart=UINT32_MAX;
	}
	else if (synth.modulationDelayStart==UINT32_MAX)
	{
		synth.modulationDelayStart=currentTick;
	}
	
	if(refreshTickCount)
		synth.modulationDelayTickCount=exponentialCourse(UINT16_MAX-currentPreset.continuousParameters[cpModDelay],12000.0f,2500.0f);
}

static void handleFinishedVoices(void)
{
	int8_t v;
	
	// when amp env finishes, voice is done
	
	for(v=0;v<SYNTH_VOICE_COUNT;++v)
		if(assigner_getAssignment(v,NULL) && adsr_getStage(&synth.ampEnvs[v])==sWait)
			assigner_voiceDone(v);
}

static void refreshAssignerSettings(void)
{
	assigner_setPattern(currentPreset.voicePattern,currentPreset.steppedParameters[spUnison]);
	assigner_setVoiceMask(settings.voiceMask);
	assigner_setPriority(currentPreset.steppedParameters[spAssignerPriority]);
}

static void refreshEnvSettings(int8_t type)
{
	uint8_t slow;
	int8_t i;
	struct adsr_s * a;
		
	slow=currentPreset.steppedParameters[(type)?spFilEnvSlow:spAmpEnvSlow];

	for(i=0;i<SYNTH_VOICE_COUNT;++i)
	{
		if(type)
			a=&synth.filEnvs[i];
		else
			a=&synth.ampEnvs[i];

		adsr_setSpeedShift(a,(slow)?3:1);
	}
}

static void refreshLfoSettings(void)
{
	lfoShape_t shape;
	uint8_t shift;

	shape=currentPreset.steppedParameters[spLFOShape];
	shift=1+currentPreset.steppedParameters[spLFOShift]*3;

	lfo_setShape(&synth.lfo,shape);
	lfo_setSpeedShift(&synth.lfo,shift);
}

void refreshFullState(void)
{
	refreshModulationDelay(1);
	refreshAssignerSettings();
	refreshLfoSettings();
	refreshEnvSettings(0);
	refreshEnvSettings(1);
	computeBenderCVs();
}

void refreshPresetMode(void)
{
	if(!preset_loadCurrent(settings.presetMode?settings.presetNumber:MANUAL_PRESET_PAGE))
		preset_loadDefault(1);

	ui_setPresetModified(0);
}

static void setCVReference(uint16_t value)
{
	uint16_t cmd;
	
	cmd=(value>>4)|DACSPI_CMD_SET_REF;
	
	dacspi_setCommand(DACSPI_CV_CHANNEL,0,cmd);
	dacspi_setCommand(DACSPI_CV_CHANNEL,1,cmd);

	delay_us(6);
}

static void refreshCV(int8_t voice, cv_t cv, uint16_t value)
{
	if(cv==cvCutoff ||cv==cvAmp || cv==cvMasterLeft || cv==cvMasterRight)
		value=UINT16_MAX-value;

	GPIO_SetValue(CVMUX_PORT_CARD0,1<<CVMUX_PIN_CARD0);
	GPIO_SetValue(CVMUX_PORT_CARD1,1<<CVMUX_PIN_CARD1);
	GPIO_SetValue(CVMUX_PORT_CARD2,1<<CVMUX_PIN_CARD2);
	GPIO_SetValue(CVMUX_PORT_VCA,1<<CVMUX_PIN_VCA);
	GPIO_ClearValue(CVMUX_PORT_ABC,7<<CVMUX_PIN_A);

	setCVReference(value);

	if(cv<=cvResonance)
	{
		voice=synth_voiceLayout[0][voice];
		
		if(cv&1)
			GPIO_SetValue(CVMUX_PORT_ABC,1<<CVMUX_PIN_A);
		if(cv&2)
			GPIO_SetValue(CVMUX_PORT_ABC,1<<CVMUX_PIN_B);
		if(voice&1)
			GPIO_SetValue(CVMUX_PORT_ABC,1<<CVMUX_PIN_C);

		delay_us(1);

		switch(voice>>1)
		{
		case 0:
			GPIO_ClearValue(CVMUX_PORT_CARD0,1<<CVMUX_PIN_CARD0);
			break;			
		case 1:
			GPIO_ClearValue(CVMUX_PORT_CARD1,1<<CVMUX_PIN_CARD1);
			break;			
		case 2:
			GPIO_ClearValue(CVMUX_PORT_CARD2,1<<CVMUX_PIN_CARD2);
			break;			
		}
	}
	else if(cv>=cvAmp)
	{
		if(cv==cvAmp)
		{
			voice=synth_voiceLayout[1][voice];

			GPIO_SetValue(CVMUX_PORT_ABC,voice<<CVMUX_PIN_A);
		}
		else
		{
			cv-=cvMasterLeft;
			GPIO_SetValue(CVMUX_PORT_ABC,(cv+SYNTH_VOICE_COUNT)<<CVMUX_PIN_A);
		}

		delay_us(1);

		GPIO_ClearValue(CVMUX_PORT_VCA,1<<CVMUX_PIN_VCA);
	}

	delay_us(3);
}

static FORCEINLINE void refreshVoice(int8_t v,int16_t wmodEnvAmt,int16_t filEnvAmt,int16_t pitchAVal,int16_t pitchBVal,int16_t wmodAVal,int16_t wmodBVal,int16_t filterVal, uint8_t wmodMask)
{
	int32_t vpa,vpb,vma,vmb,vf;
	uint16_t envVal;

	// update envs, compute CVs & apply them

	adsr_update(&synth.filEnvs[v]);
	envVal=synth.filEnvs[v].output;

	// filter

	vf=filterVal;
	vf+=scaleU16S16(envVal,filEnvAmt);
	vf+=synth.filterNoteCV[v];
	refreshCV(v,cvCutoff,vf);

	// oscs
	
	vpa=pitchAVal;
	vpb=pitchBVal;

	vma=wmodAVal;
	if(wmodMask&8)
		vma+=scaleU16S16(envVal,wmodEnvAmt);

	vmb=wmodBVal;
	if(wmodMask&128)
		vmb+=scaleU16S16(envVal,wmodEnvAmt);

	// osc A

	vpa+=synth.oscANoteCV[v];
	wtosc_setParameters(&synth.osc[v][0],vpa,(wmodMask&1)?vma:0);

	// osc B

	vpb+=synth.oscBNoteCV[v];
	wtosc_setParameters(&synth.osc[v][1],vpb,(wmodMask&16)?vmb:0);

	// amplifier

	adsr_update(&synth.ampEnvs[v]);
	refreshCV(v,cvAmp,synth.ampEnvs[v].output);
}

////////////////////////////////////////////////////////////////////////////////
// Synth main code
////////////////////////////////////////////////////////////////////////////////

void synth_init(void)
{
	int i;
	FRESULT res;
	
	memset(&synth,0,sizeof(synth));
	
	// init cv mux

	GPIO_SetDir(CVMUX_PORT_ABC,1<<CVMUX_PIN_A,1); // A
	GPIO_SetDir(CVMUX_PORT_ABC,1<<CVMUX_PIN_B,1); // B
	GPIO_SetDir(CVMUX_PORT_ABC,1<<CVMUX_PIN_C,1); // C
	
	PINSEL_SetOpenDrainMode(CVMUX_PORT_CARD0,CVMUX_PIN_CARD0,PINSEL_PINMODE_NORMAL);
	PINSEL_SetOpenDrainMode(CVMUX_PORT_CARD1,CVMUX_PIN_CARD1,PINSEL_PINMODE_NORMAL);
	PINSEL_SetOpenDrainMode(CVMUX_PORT_CARD2,CVMUX_PIN_CARD2,PINSEL_PINMODE_NORMAL);
	PINSEL_SetOpenDrainMode(CVMUX_PORT_VCA,CVMUX_PIN_VCA,PINSEL_PINMODE_NORMAL);
	
	PINSEL_SetResistorMode(CVMUX_PORT_CARD0,CVMUX_PIN_CARD0,PINSEL_PINMODE_TRISTATE);
	PINSEL_SetResistorMode(CVMUX_PORT_CARD1,CVMUX_PIN_CARD1,PINSEL_PINMODE_TRISTATE);
	PINSEL_SetResistorMode(CVMUX_PORT_CARD2,CVMUX_PIN_CARD2,PINSEL_PINMODE_TRISTATE);
	PINSEL_SetResistorMode(CVMUX_PORT_VCA,CVMUX_PIN_VCA,PINSEL_PINMODE_TRISTATE);
	
	GPIO_SetDir(CVMUX_PORT_CARD0,1<<CVMUX_PIN_CARD0,1); // voice card 0
	GPIO_SetDir(CVMUX_PORT_CARD1,1<<CVMUX_PIN_CARD1,1); // voice card 1
	GPIO_SetDir(CVMUX_PORT_CARD2,1<<CVMUX_PIN_CARD2,1); // voice card 2
	GPIO_SetDir(CVMUX_PORT_VCA,1<<CVMUX_PIN_VCA,1); // VCAs
	
	// init wavetable oscs
	for(i=0;i<SYNTH_VOICE_COUNT;++i)
	{
		wtosc_init(&synth.osc[i][0],DACSPI_CMD_SET_A);
		wtosc_init(&synth.osc[i][1],DACSPI_CMD_SET_B);
	}

	synth.curFile.lfname=synth.lfname;
	synth.curFile.lfsize=sizeof(synth.lfname);
	
	BLOCK_INT
	{
		__enable_irq();
		
		if((res=f_opendir(&synth.curDir,BANK_DIR)))
			rprintf(0,"f_opendir res=%d\n",res);

		if((res=f_readdir(&synth.curDir,&synth.curFile)))
			rprintf(0,"f_readdir res=%d\n",res);

		loadWaveTable();
	}

	// init subsystems
	dacspi_init();
	tuner_init();
	assigner_init();
	uartMidi_init();
	arp_init();
	ui_init();
	midi_init();
	
	for(i=0;i<SYNTH_VOICE_COUNT;++i)
	{
		adsr_init(&synth.ampEnvs[i]);
		adsr_init(&synth.filEnvs[i]);

		adsr_setShape(&synth.ampEnvs[i],1);
		adsr_setShape(&synth.filEnvs[i],1);
	}

	lfo_init(&synth.lfo);
	lfo_init(&synth.vibrato);
	lfo_setShape(&synth.vibrato,lsTri);
	lfo_setSpeedShift(&synth.vibrato,4);

	// manual preset
	
	if(!preset_loadCurrent(MANUAL_PRESET_PAGE))
	{
		preset_loadDefault(1);
		preset_saveCurrent(MANUAL_PRESET_PAGE);
	}
	
	// load settings from storage; tune when they are bad
	
	if(!settings_load())
	{
		settings_loadDefault();
		tuner_tuneSynth();
	}

	// load last preset & do a full refresh
	
	refreshPresetMode();
	refreshFullState();
}


void synth_update(void)
{
	int i;
	static int frc=0;
	int8_t dlyMod;
	uint16_t mwAmt,lfoAmt,vibAmt;
	
print("u");	
	++frc;
	
	ui_update();
	
	// envs

	for(i=0;i<SYNTH_VOICE_COUNT;++i)
	{
		adsr_setCVs(&synth.ampEnvs[i],
				 currentPreset.continuousParameters[cpAmpAtt],
				 currentPreset.continuousParameters[cpAmpDec],
				 currentPreset.continuousParameters[cpAmpSus],
				 currentPreset.continuousParameters[cpAmpRel],
				 0,0x0f);

		adsr_setCVs(&synth.filEnvs[i],
				 currentPreset.continuousParameters[cpFilAtt],
				 currentPreset.continuousParameters[cpFilDec],
				 currentPreset.continuousParameters[cpFilSus],
				 currentPreset.continuousParameters[cpFilRel],
				 0,0x0f);
	}

	// modulations

	dlyMod=currentTick-synth.modulationDelayStart>synth.modulationDelayTickCount;
	mwAmt=synth.modwheelAmount>>currentPreset.steppedParameters[spModwheelShift];

	lfoAmt=currentPreset.continuousParameters[cpLFOAmt];
	lfoAmt=(lfoAmt<POT_DEAD_ZONE)?0:(lfoAmt-POT_DEAD_ZONE);

	vibAmt=currentPreset.continuousParameters[cpVibAmt]>>2;
	vibAmt=(vibAmt<POT_DEAD_ZONE)?0:(vibAmt-POT_DEAD_ZONE);

	if(currentPreset.steppedParameters[spModwheelTarget]==0) // targeting lfo?
	{
		lfo_setCVs(&synth.lfo,
				currentPreset.continuousParameters[cpLFOFreq],
				satAddU16U16(lfoAmt,mwAmt));
		lfo_setCVs(&synth.vibrato,
				 currentPreset.continuousParameters[cpVibFreq],
				 dlyMod?vibAmt:0);
	}
	else
	{
		lfo_setCVs(&synth.lfo,
				currentPreset.continuousParameters[cpLFOFreq],
				dlyMod?lfoAmt:0);
		lfo_setCVs(&synth.vibrato,
				currentPreset.continuousParameters[cpVibFreq],
				satAddU16U16(vibAmt,mwAmt));
	}

	// glide

	synth.glideAmount=exponentialCourse(currentPreset.continuousParameters[cpGlide],11000.0f,2100.0f);
	synth.gliding=synth.glideAmount<2000;

	// arp

	arp_setSpeed(currentPreset.continuousParameters[cpSeqArpClock]);
}

////////////////////////////////////////////////////////////////////////////////
// Synth interrupts
////////////////////////////////////////////////////////////////////////////////

// 2Khz
void synth_timerInterrupt(void)
{
	int32_t v,pitchAVal,pitchBVal,wmodAVal,wmodBVal,filterVal,wmodEnvAmt,filEnvAmt;
	uint8_t wmodEnvMask;

	static int frc=0;

	// lfo
		
		// update
	
	lfo_update(&synth.lfo);
		
		// pitch
	
	pitchAVal=pitchBVal=synth.vibrato.output;
	v=scaleU16S16(currentPreset.continuousParameters[cpLFOPitchAmt],synth.lfo.output>>1);
	
	if(currentPreset.steppedParameters[spLFOTargets]&otA)
		pitchAVal+=v;
	if(currentPreset.steppedParameters[spLFOTargets]&otB)
		pitchBVal+=v;

		// filter

	filterVal=scaleU16S16(currentPreset.continuousParameters[cpLFOFilAmt],synth.lfo.output);
	
	// global computations
	
	v=currentPreset.continuousParameters[cpFilEnvAmt];
	v+=INT16_MIN;
	filEnvAmt=v;
	
	wmodAVal=0;
	if(currentPreset.steppedParameters[spABaseWMod]&otA)
		wmodAVal=currentPreset.continuousParameters[cpABaseWMod];

	wmodBVal=0;
	if(currentPreset.steppedParameters[spBBaseWMod]&otB)
		wmodAVal=currentPreset.continuousParameters[cpBBaseWMod];

	wmodEnvAmt=currentPreset.continuousParameters[cpWModFilEnv];
	
	wmodEnvMask=0;
	if(currentPreset.steppedParameters[spABaseWMod]==wmAliasing)
		wmodEnvMask|=1;
	if(currentPreset.steppedParameters[spABaseWMod]==wmWidth)
		wmodEnvMask|=2;
	if(currentPreset.steppedParameters[spAFilWMod])
		wmodEnvMask|=8;

	if(currentPreset.steppedParameters[spBBaseWMod]==wmAliasing)
		wmodEnvMask|=16;
	if(currentPreset.steppedParameters[spBBaseWMod]==wmWidth)
		wmodEnvMask|=32;
	if(currentPreset.steppedParameters[spBFilWMod])
		wmodEnvMask|=128;

	
	// per voice stuff
	
		// P600_VOICE_COUNT calls
	refreshVoice(0,wmodEnvAmt,filEnvAmt,pitchAVal,pitchBVal,wmodAVal,wmodBVal,filterVal,wmodEnvMask);
	refreshVoice(1,wmodEnvAmt,filEnvAmt,pitchAVal,pitchBVal,wmodAVal,wmodBVal,filterVal,wmodEnvMask);
	refreshVoice(2,wmodEnvAmt,filEnvAmt,pitchAVal,pitchBVal,wmodAVal,wmodBVal,filterVal,wmodEnvMask);
	refreshVoice(3,wmodEnvAmt,filEnvAmt,pitchAVal,pitchBVal,wmodAVal,wmodBVal,filterVal,wmodEnvMask);
	refreshVoice(4,wmodEnvAmt,filEnvAmt,pitchAVal,pitchBVal,wmodAVal,wmodBVal,filterVal,wmodEnvMask);
	refreshVoice(5,wmodEnvAmt,filEnvAmt,pitchAVal,pitchBVal,wmodAVal,wmodBVal,filterVal,wmodEnvMask);

	// slower updates
	
	v=(frc>>2)%(SYNTH_VOICE_COUNT+1);

	switch(frc&0x03) // 4 phases, each 500hz
	{
	case 0:
		midi_update();
		
		++currentTick;

		// CV update
		if(v<SYNTH_VOICE_COUNT)
			refreshCV(v,cvAVol,currentPreset.continuousParameters[cpAVol]);
		else
			refreshCV(SYNTH_VOICE_COUNT,cvMasterLeft,currentPreset.continuousParameters[cpMasterLeft]);
		break;
	case 1:
		if(synth.gliding)
		{
			for(v=0;v<SYNTH_VOICE_COUNT;++v)
			{
				computeGlide(&synth.oscANoteCV[v],synth.oscATargetCV[v],synth.glideAmount);
				computeGlide(&synth.oscBNoteCV[v],synth.oscBTargetCV[v],synth.glideAmount);
				computeGlide(&synth.filterNoteCV[v],synth.filterTargetCV[v],synth.glideAmount);
			}
		}

		// CV update
		if(v<SYNTH_VOICE_COUNT)
			refreshCV(v,cvBVol,currentPreset.continuousParameters[cpBVol]);
		else
			refreshCV(SYNTH_VOICE_COUNT,cvMasterRight,currentPreset.continuousParameters[cpMasterRight]);
		break;
	case 2:
		if(arp_getMode()!=amOff)
		{
			arp_update();
		}

		// CV update
		if(v<SYNTH_VOICE_COUNT)
			refreshCV(v,cvResonance,currentPreset.continuousParameters[cpResonance]);
		else
			handleFinishedVoices(); // 1/7th of 500hz, good enough
		break;
	case 3:
		lfo_update(&synth.vibrato);
		break;
	}

	++frc;
}

////////////////////////////////////////////////////////////////////////////////
// Synth internal events
////////////////////////////////////////////////////////////////////////////////

#define DO_OSC(v,o) dacspi_setCommand(v,o,wtosc_update(&synth.osc[v][o],DACSPI_TICK_RATE))

void synth_updateDACsEvent(void)
{
	DO_OSC(0,0);
	DO_OSC(0,1);
	DO_OSC(1,0);
	DO_OSC(1,1);
	DO_OSC(2,0);
	DO_OSC(2,1);
	DO_OSC(3,0);
	DO_OSC(3,1);
	DO_OSC(4,0);
	DO_OSC(4,1);
	DO_OSC(5,0);
	DO_OSC(5,1);
}

void synth_assignerEvent(uint8_t note, int8_t gate, int8_t voice, uint16_t velocity, int8_t legato)
{
#ifdef DEBUG
	rprintf(0,"assign note %d gate %d voice %d velocity %d legato %d\n",note,gate,voice,velocity,legato);
#endif

	uint16_t velAmt;
	
	// mod delay

	refreshModulationDelay(0);

	// prepare CVs

	computeTunedCVs();

	// set gates (don't retrigger gate, unless we're arpeggiating)

	if(!legato || arp_getMode()!=amOff)
	{
		adsr_setGate(&synth.filEnvs[voice],gate);
		adsr_setGate(&synth.ampEnvs[voice],gate);
	}

	if(gate)
	{
		// handle velocity

		velAmt=currentPreset.continuousParameters[cpFilVelocity];
		adsr_setCVs(&synth.filEnvs[voice],0,0,0,0,(UINT16_MAX-velAmt)+scaleU16U16(velocity,velAmt),0x10);
		velAmt=currentPreset.continuousParameters[cpAmpVelocity];
		adsr_setCVs(&synth.ampEnvs[voice],0,0,0,0,(UINT16_MAX-velAmt)+scaleU16U16(velocity,velAmt),0x10);
	}
}

void synth_uartEvent(uint8_t data)
{
#ifdef DEBUG_
	rprintf(0,"midi %x\n",data);
#endif

	midi_newData(data);
}
