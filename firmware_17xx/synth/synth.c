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
#include "vca_curves.h"
#include "../xnormidi/midi.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"

#define BIT_INTPUT_FOOTSWITCH (1<<26)

#define WAVEDATA_PATH "/WAVEDATA"

#define MAX_BANKS 80
#define MAX_BANK_WAVES 100
#define MAX_FILENAME _MAX_LFN

#define POT_DEAD_ZONE 512

// soft voice to hard voice (mapped so that the upper the voice, the stronger panning it gets)
const int8_t synth_voiceLayout[2][SYNTH_VOICE_COUNT] =
{
	{0,1,3,4,2,5}, //  for DACs
	{2,3,1,4,0,5}, //  for VCAs
};

volatile uint32_t currentTick=0; // 500hz

__attribute__ ((section(".ext_ram"))) static struct
{
	int bankCount;
	int curWaveCount[2];
	char bankNames[MAX_BANKS][MAX_FILENAME];
	char waveNames[2][MAX_BANK_WAVES][MAX_FILENAME];

	DIR curDir;
	FILINFO curFile;
	char lfname[_MAX_LFN + 1];
} waveData;

static struct
{
	struct wtosc_s osc[SYNTH_VOICE_COUNT][2];
	struct adsr_s filEnvs[SYNTH_VOICE_COUNT];
	struct adsr_s ampEnvs[SYNTH_VOICE_COUNT];
	struct lfo_s lfo[SYNTH_PART_COUNT];
	struct lfo_s vibrato[SYNTH_PART_COUNT];
	
	uint16_t oscANoteCV[SYNTH_VOICE_COUNT];
	uint16_t oscBNoteCV[SYNTH_VOICE_COUNT];
	uint16_t filterNoteCV[SYNTH_VOICE_COUNT]; 
	
	uint16_t oscATargetCV[SYNTH_VOICE_COUNT];
	uint16_t oscBTargetCV[SYNTH_VOICE_COUNT];
	uint16_t filterTargetCV[SYNTH_VOICE_COUNT];

	struct
	{
		int16_t benderCVs[cvAmp-cvAVol+1];

		int16_t benderAmount;
		uint16_t modwheelAmount;
		int16_t glideAmount;
		int8_t gliding;

		uint32_t modulationDelayStart;
		uint16_t modulationDelayTickCount;

		uint8_t wmodMask;
	} partState[SYNTH_PART_COUNT];
	
	uint8_t pendingExtClock;
	
	int8_t voicePart[SYNTH_VOICE_COUNT];
} synth;

extern const uint16_t attackCurveLookup[]; // for modulation delay

////////////////////////////////////////////////////////////////////////////////
// Non speed critical internal code
////////////////////////////////////////////////////////////////////////////////

static void computeTunedCVs(void)
{
	uint16_t cva,cvb,cvf;
	uint8_t note,baseCutoffNote,baseANote,baseBNote,trackingNote;
	int8_t v;

	uint16_t baseAPitch,baseBPitch,baseCutoff;
	int16_t mTune,fineBFreq,detune;

	uint16_t baseBPitchRaw,baseCutoffRaw,mTuneRaw,fineBFreqRaw,detuneRaw,trackRaw;
	uint8_t chrom;
	
	for(v=0;v<SYNTH_VOICE_COUNT;++v)
	{
		int8_t part=synth.voicePart[v];
		
		if (!assigner_getAssignment(&assigner[part],v,&note))
			continue;
		
		// get raw values

		mTuneRaw=currentPreset[part].continuousParameters[cpMasterTune];
		fineBFreqRaw=currentPreset[part].continuousParameters[cpBFineFreq];
		baseCutoffRaw=currentPreset[part].continuousParameters[cpCutoff];
		baseBPitchRaw=currentPreset[part].continuousParameters[cpBFreq];
		detuneRaw=currentPreset[part].continuousParameters[cpUnisonDetune];
		trackRaw=currentPreset[part].continuousParameters[cpFilKbdAmt];
		chrom=currentPreset[part].steppedParameters[spChromaticPitch];

		// compute for oscs & filters

		mTune=(mTuneRaw>>7)+INT8_MIN*2;
		fineBFreq=(fineBFreqRaw>>8)+INT8_MIN;
		baseCutoff=((uint32_t)baseCutoffRaw*3)>>2; // 75% of raw cutoff

		if(baseBPitchRaw>=UINT16_MAX/2)
		{
			baseAPitch=0;
			baseBPitch=(baseBPitchRaw-(UINT16_MAX/2))>>1;
		}
		else
		{
			baseAPitch=((UINT16_MAX/2)-baseBPitchRaw)>>1;
			baseBPitch=0;
		}

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

		// oscs
		
		cva=satAddU16S32(tuner_computeCVFromNote(v,baseANote+note,baseAPitch,cvAPitch),(int32_t)synth.partState[part].benderCVs[cvAPitch]+mTune);
		cvb=satAddU16S32(tuner_computeCVFromNote(v,baseBNote+note,baseBPitch,cvBPitch),(int32_t)synth.partState[part].benderCVs[cvBPitch]+mTune+fineBFreq);
		
		detune=(1+(v>>1))*(v&1?-1:1)*(detuneRaw>>9);
		cva=satAddU16S16(cva,detune);
		cvb=satAddU16S16(cvb,detune);
		
		// filter
		
		trackingNote=baseCutoffNote+((note*(trackRaw>>8))>>8);
			
		cvf=satAddU16S16(tuner_computeCVFromNote(v,trackingNote,baseCutoff,cvCutoff),synth.partState[part].benderCVs[cvCutoff]);
		
		// glide
		
		if(synth.partState[part].gliding)
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
	static const int8_t br[]={3,5,12,0};
	
	int32_t bend;

	for(int8_t part=0;part<SYNTH_PART_COUNT;++part)
	{
		bend=synth.partState[part].benderAmount;

		switch(currentPreset[part].steppedParameters[spBenderTarget])
		{
		case modPitch:
			bend*=tuner_computeCVFromNote(0,br[currentPreset[part].steppedParameters[spBenderRange]]*2,0,cvAPitch)-tuner_computeCVFromNote(0,0,0,cvAPitch);
			bend/=UINT16_MAX;
			synth.partState[part].benderCVs[cvAPitch]=bend;
			synth.partState[part].benderCVs[cvBPitch]=bend;
			break;
		case modFil:
			bend*=tuner_computeCVFromNote(0,br[currentPreset[part].steppedParameters[spBenderRange]]*4,0,cvCutoff)-tuner_computeCVFromNote(0,0,0,cvCutoff);
			bend/=UINT16_MAX;
			synth.partState[part].benderCVs[cvCutoff]=bend;
			break;
		case modAmp:
			bend=(bend*br[currentPreset[part].steppedParameters[spBenderRange]])/12;
			synth.partState[part].benderCVs[cvAmp]=bend;
			break;
		}
		}
}

static void handleFinishedVoices(void)
{
	int8_t v;
	
	for(v=0;v<SYNTH_VOICE_COUNT;++v)
	{
		// when amp env finishes, voice is done
		if(assigner_getAssignment(&assigner[synth.voicePart[v]],v,NULL) && adsr_getStage(&synth.ampEnvs[v])==sWait)
			assigner_voiceDone(&assigner[synth.voicePart[v]],v);
	
		// if voice isn't assigned, silence it
		if(!assigner_getAssignment(&assigner[synth.voicePart[v]],v,NULL) && adsr_getStage(&synth.ampEnvs[v])!=sWait)
		{
			adsr_reset(&synth.ampEnvs[v]);
			adsr_reset(&synth.filEnvs[v]);
		}
	}
}

static void refreshAssignerSettings(void)
{
	for(int8_t part=0;part<SYNTH_PART_COUNT;++part)
	{
		assigner_setPattern(&assigner[part],currentPreset[part].voicePattern,currentPreset[part].steppedParameters[spUnison]);
		assigner_setVoiceMask(&assigner[part],settings.voiceMask);
		assigner_setPriority(&assigner[part],currentPreset[part].steppedParameters[spAssignerPriority]);
	}
}

static void refreshEnvSettings(int8_t type)
{
	uint8_t slow;
	int8_t i;
	struct adsr_s * a;
		
	for(i=0;i<SYNTH_VOICE_COUNT;++i)
	{
		int8_t part=synth.voicePart[i];
		
		slow=currentPreset[part].steppedParameters[(type)?spFilEnvSlow:spAmpEnvSlow];

		if(type)
			a=&synth.filEnvs[i];
		else
			a=&synth.ampEnvs[i];

		adsr_setSpeedShift(a,(slow)?3:1);

		adsr_setCVs(&synth.ampEnvs[i],
				 currentPreset[part].continuousParameters[cpAmpAtt],
				 currentPreset[part].continuousParameters[cpAmpDec],
				 currentPreset[part].continuousParameters[cpAmpSus],
				 currentPreset[part].continuousParameters[cpAmpRel],
				 0,0x0f);

		adsr_setCVs(&synth.filEnvs[i],
				 currentPreset[part].continuousParameters[cpFilAtt],
				 currentPreset[part].continuousParameters[cpFilDec],
				 currentPreset[part].continuousParameters[cpFilSus],
				 currentPreset[part].continuousParameters[cpFilRel],
				 0,0x0f);
	}
}

static void refreshLfoSettings(void)
{
	static const int8_t mr[]={5,3,1,0};
	lfoShape_t shape;
	uint8_t shift;
	uint16_t mwAmt,lfoAmt,vibAmt,dlyAmt;
	uint32_t elapsed;

	for(int8_t part=0;part<SYNTH_PART_COUNT;++part)
	{
		shape=currentPreset[part].steppedParameters[spLFOShape];
		shift=1+currentPreset[part].steppedParameters[spLFOShift]*3;

		lfo_setShape(&synth.lfo[part],shape);
		lfo_setSpeedShift(&synth.lfo[part],shift);

		// wait modulationDelayTickCount then progressively increase over
		// modulationDelayTickCount time, following an exponential curve
		dlyAmt=0;
		if(synth.partState[part].modulationDelayStart!=UINT32_MAX)
		{
			if(currentPreset[part].continuousParameters[cpModDelay]<POT_DEAD_ZONE)
			{
				dlyAmt=UINT16_MAX;
			}
			else if(currentTick>=synth.partState[part].modulationDelayStart+synth.partState[part].modulationDelayTickCount)
			{
				elapsed=currentTick-(synth.partState[part].modulationDelayStart+synth.partState[part].modulationDelayTickCount);
				if(elapsed>=synth.partState[part].modulationDelayTickCount)
					dlyAmt=UINT16_MAX;
				else
					dlyAmt=attackCurveLookup[(elapsed<<8)/synth.partState[part].modulationDelayTickCount];
			}
		}

		mwAmt=synth.partState[part].modwheelAmount>>mr[currentPreset[part].steppedParameters[spModwheelRange]];
		lfoAmt=currentPreset[part].continuousParameters[cpLFOAmt];
		lfoAmt=(lfoAmt<POT_DEAD_ZONE)?0:(lfoAmt-POT_DEAD_ZONE);

		vibAmt=currentPreset[part].continuousParameters[cpVibAmt]>>2;
		vibAmt=(vibAmt<POT_DEAD_ZONE)?0:(vibAmt-POT_DEAD_ZONE);

		if(currentPreset[part].steppedParameters[spModwheelTarget]==0) // targeting lfo?
		{
			lfo_setCVs(&synth.lfo[part],
					currentPreset[part].continuousParameters[cpLFOFreq],
					satAddU16U16(lfoAmt,mwAmt));
			lfo_setCVs(&synth.vibrato[part],
					 currentPreset[part].continuousParameters[cpVibFreq],
					 scaleU16U16(vibAmt,dlyAmt));
		}
		else
		{
			lfo_setCVs(&synth.lfo[part],
					currentPreset[part].continuousParameters[cpLFOFreq],
					scaleU16U16(lfoAmt,dlyAmt));
			lfo_setCVs(&synth.vibrato[part],
					currentPreset[part].continuousParameters[cpVibFreq],
					satAddU16U16(vibAmt,mwAmt));
		}
	}
}

static void refreshModulationDelay(int8_t refreshTickCount)
{
	int8_t anyPressed, anyAssigned;
	static int8_t prevAnyPressed[SYNTH_PART_COUNT]={0};
	
	for(int8_t part=0;part<SYNTH_PART_COUNT;++part)
	{
		anyPressed=assigner_getAnyPressed(&assigner[part]);	
		anyAssigned=assigner_getAnyAssigned(&assigner[part]);	

		if(!anyAssigned)
		{
			synth.partState[part].modulationDelayStart=UINT32_MAX;
		}

		if(anyPressed && !prevAnyPressed[part])
		{
			synth.partState[part].modulationDelayStart=currentTick;
		}

		prevAnyPressed[part]=anyPressed;

		if(refreshTickCount)
			synth.partState[part].modulationDelayTickCount=exponentialCourse(UINT16_MAX-currentPreset[part].continuousParameters[cpModDelay],12000.0f,2500.0f);
	}
}

static void refreshMisc(void)
{
	// arp

	arp_setSpeed(currentPreset[currentPart].continuousParameters[cpSeqArpClock]);

	for(int8_t part=0;part<SYNTH_PART_COUNT;++part)
	{
		// glide

		synth.partState[part].glideAmount=exponentialCourse(currentPreset[part].continuousParameters[cpGlide],11000.0f,2100.0f);
		synth.partState[part].gliding=synth.partState[part].glideAmount<2000;

		// WaveMod mask

		synth.partState[part].wmodMask=0;
		if(currentPreset[part].steppedParameters[spAWModType]==wmAliasing)
			synth.partState[part].wmodMask|=1;
		if(currentPreset[part].steppedParameters[spAWModType]==wmWidth)
			synth.partState[part].wmodMask|=2;
		if(currentPreset[part].steppedParameters[spAWModType]==wmFrequency)
			synth.partState[part].wmodMask|=4;
		if(currentPreset[part].steppedParameters[spAWModEnvEn])
			synth.partState[part].wmodMask|=8;

		if(currentPreset[part].steppedParameters[spBWModType]==wmAliasing)
			synth.partState[part].wmodMask|=16;
		if(currentPreset[part].steppedParameters[spBWModType]==wmWidth)
			synth.partState[part].wmodMask|=32;
		if(currentPreset[part].steppedParameters[spBWModType]==wmFrequency)
			synth.partState[part].wmodMask|=64;
		if(currentPreset[part].steppedParameters[spBWModEnvEn])
			synth.partState[part].wmodMask|=128;
	}
}

void refreshFullState(void)
{
	refreshModulationDelay(1);
	refreshAssignerSettings();
	refreshLfoSettings();
	refreshEnvSettings(0);
	refreshEnvSettings(1);
	refreshMisc();
	computeBenderCVs();
	computeTunedCVs();
}

int8_t appendBankName(int8_t ab, char * path)
{
	uint8_t bankNum;

	bankNum=currentPreset[currentPart].steppedParameters[ab?spBBank:spABank];

	if(bankNum>=waveData.bankCount)
		return 0;

	strcat(path,waveData.bankNames[bankNum]);
	
	return 1;
}

int8_t appendWaveName(int8_t ab, char * path)
{
	uint8_t waveNum;

	waveNum=currentPreset[currentPart].steppedParameters[ab?spBWave:spAWave];

	if(waveNum>=waveData.curWaveCount[ab])
		return 0;

	strcat(path,waveData.waveNames[ab][waveNum]);
	
	return 1;
}

int getBankCount(void)
{
	return waveData.bankCount;
}

int getWaveCount(int8_t ab)
{
	return waveData.curWaveCount[ab];
}

static void refreshBankNames(void)
{
	FRESULT res;
	
	waveData.bankCount=0;

	if((res=f_opendir(&waveData.curDir,WAVEDATA_PATH)))
	{
		rprintf(0,"f_opendir res=%d\n",res);
		return;
	}

	if((res=f_readdir(&waveData.curDir,&waveData.curFile)))
		rprintf(0,"f_readdir res=%d\n",res);

	while(!res)
	{
		if(strcmp(waveData.curFile.fname,".") && strcmp(waveData.curFile.fname,".."))
		{
			strncpy(waveData.bankNames[waveData.bankCount],waveData.curFile.lfname,waveData.curFile.lfsize);
			++waveData.bankCount;
		}
		
		res=f_readdir(&waveData.curDir,&waveData.curFile);
		if (!waveData.curFile.fname[0] || waveData.bankCount>=MAX_BANKS)
			break;
	}
	
	rprintf(0,"bankCount %d\n",waveData.bankCount);
}

void refreshWaveNames(int8_t ab)
{
	FRESULT res;
	char fn[256];
	
	waveData.curWaveCount[ab]=0;

	strcpy(fn,WAVEDATA_PATH "/");
	if(!appendBankName(ab,fn)) return;

	rprintf(0,"loading %s\n",fn);
	
	if((res=f_opendir(&waveData.curDir,fn)))
	{
		rprintf(0,"f_opendir res=%d\n",res);
		return;
	}

	if((res=f_readdir(&waveData.curDir,&waveData.curFile)))
		rprintf(0,"f_readdir res=%d\n",res);

	while(!res)
	{
		if(strstr(waveData.curFile.fname,".WAV"))
		{
			strncpy(waveData.waveNames[ab][waveData.curWaveCount[ab]],waveData.curFile.lfname,waveData.curFile.lfsize);
			++waveData.curWaveCount[ab];
		}
		
		res=f_readdir(&waveData.curDir,&waveData.curFile);
		if (!waveData.curFile.fname[0] || waveData.curWaveCount[ab]>=MAX_BANK_WAVES)
			break;
	}

	rprintf(0,"curWaveCount %d %d\n",ab,waveData.curWaveCount[ab]);
}

void refreshWaveforms(int8_t ab)
{
	int i;
	FIL f;
	FRESULT res;
	char fn[256];
	
	strcpy(fn,WAVEDATA_PATH "/");
	if(!appendBankName(ab,fn)) return;
	strcat(fn,"/");
	if(!appendWaveName(ab,fn)) return;

	rprintf(0,"loading %s\n",fn);
	
	if(!f_open(&f,fn,FA_READ))
	{

		if((res=f_lseek(&f,0x2c)))
			rprintf(0,"f_lseek res=%d\n",res);

		int16_t data[WTOSC_MAX_SAMPLES];

		if((res=f_read(&f,data,sizeof(data),&i)))
			rprintf(0,"f_lseek res=%d\n",res);

		f_close(&f);

		for(i=0;i<SYNTH_VOICE_COUNT;++i)
			wtosc_setSampleData(&synth.osc[i][ab],data,WTOSC_MAX_SAMPLES);
	}
}

static void handleBitInputs(void)
{
	uint32_t cur;
	static uint32_t last=0;
	
	cur=GPIO_ReadValue(3);
	
	// control footswitch 
	 
	if(currentPreset[currentPart].steppedParameters[spUnison] && !(cur&BIT_INTPUT_FOOTSWITCH) && last&BIT_INTPUT_FOOTSWITCH)
	{
		assigner_latchPattern(&assigner[currentPart]);
		assigner_getPattern(&assigner[currentPart], currentPreset[currentPart].voicePattern,NULL);
	}
	else if(arp_getMode()!=amOff && (cur&BIT_INTPUT_FOOTSWITCH)!=(last&BIT_INTPUT_FOOTSWITCH))
	{
		arp_setMode(arp_getMode(),(cur&BIT_INTPUT_FOOTSWITCH)?0:2);
	}

	// this must stay last
	
	last=cur;
}

////////////////////////////////////////////////////////////////////////////////
// Speed critical internal code
////////////////////////////////////////////////////////////////////////////////

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

static FORCEINLINE uint16_t adjustCV(cv_t cv, uint32_t value)
{
	switch(cv)
	{
	case cvCutoff:
		value=UINT16_MAX-value;
		break;
	case cvAmp:
	case cvMasterLeft:
	case cvMasterRight:
		value=computeShape((uint32_t)value<<8,vcaLinearizationCurve,1);		
		break;
	default:
		;
	}
	
	return value;
}

static FORCEINLINE void refreshCV(int8_t voice, cv_t cv, uint32_t v)
{
	uint16_t value,channel,cmd;
	
	v=MIN(v,UINT16_MAX);
	value=adjustCV(cv,v);

	if(cv<=cvResonance)
	{
		channel=(synth_voiceLayout[0][voice]<<2)|cv;
	}
	else if(cv>=cvAmp)
	{
		if(cv==cvAmp)
		{
			channel=24|synth_voiceLayout[1][voice];
		}
		else
		{
			channel=30+(cv-cvMasterLeft);
		}
	}
	
	cmd=(value>>4)|DACSPI_CMD_SET_REF;
	
	dacspi_setCVCommand(cmd,channel);
}

static FORCEINLINE void refreshVoice(int8_t v,int32_t wmodEnvAmt,int32_t filEnvAmt,int32_t pitchAVal,int32_t pitchBVal,int32_t wmodAVal,int32_t wmodBVal,int32_t filterVal,int32_t ampVal,uint8_t wmodMask, int32_t resoFactor)
{
	int32_t vpa,vpb,vma,vmb,vf,vamp,envVal,envValScale;

	// envs

	adsr_update(&synth.ampEnvs[v]);
	adsr_update(&synth.filEnvs[v]);
	envVal=synth.filEnvs[v].output;

	// filter

	vf=filterVal;
	vf+=scaleU16S16(envVal,filEnvAmt);
	vf+=synth.filterNoteCV[v];
	refreshCV(v,cvCutoff,vf);

	// oscs
	
	envValScale=scaleU16S16(envVal,wmodEnvAmt);

	vma=wmodAVal;
	if(wmodMask&8)
	{
		vma+=envValScale;
		vma=MAX(0,MIN(UINT16_MAX,vma));
	}

	vmb=wmodBVal;
	if(wmodMask&128)
	{
		vmb+=envValScale;
		vmb=MAX(0,MIN(UINT16_MAX,vmb));
	}

	vpa=pitchAVal;
	if(wmodMask&4)
		vpa+=vma-(UINT16_MAX/2);

	vpb=pitchBVal;
	if(wmodMask&64)
		vpb+=vmb-(UINT16_MAX/2);

	// osc A

	vpa+=synth.oscANoteCV[v];
	vpa=MAX(0,MIN(UINT16_MAX,vpa));
	wtosc_setParameters(&synth.osc[v][0],vpa,(wmodMask&1)?vma:0,(wmodMask&2)?vma:32768);

	// osc B

	vpb+=synth.oscBNoteCV[v];
	vpb=MAX(0,MIN(UINT16_MAX,vpb));
	wtosc_setParameters(&synth.osc[v][1],vpb,(wmodMask&16)?vmb:0,(wmodMask&32)?vmb:32768);

	// amplifier
	
	vamp=scaleU16U16(synth.ampEnvs[v].output,ampVal);
	vamp=(vamp*resoFactor)>>8;
	refreshCV(v,cvAmp,vamp);
}

////////////////////////////////////////////////////////////////////////////////
// Synth main code
////////////////////////////////////////////////////////////////////////////////

void synth_init(void)
{
	int i;
	
	memset(&synth,0,sizeof(synth));
	memset(&waveData,0,sizeof(waveData));

	// init footswitch in

	GPIO_SetDir(3,1<<26,0);
	PINSEL_SetResistorMode(3,26,PINSEL_PINMODE_TRISTATE);
	PINSEL_SetOpenDrainMode(3,26,PINSEL_PINMODE_NORMAL);
	
	// init cv mux

	GPIO_SetDir(CVMUX_PORT_ABC,1<<CVMUX_PIN_A,1); // A
	GPIO_SetDir(CVMUX_PORT_ABC,1<<CVMUX_PIN_B,1); // B
	GPIO_SetDir(CVMUX_PORT_ABC,1<<CVMUX_PIN_C,1); // C
	GPIO_SetDir(CVMUX_PORT_CARD0,1<<CVMUX_PIN_CARD0,1); // voice card 0
	GPIO_SetDir(CVMUX_PORT_CARD1,1<<CVMUX_PIN_CARD1,1); // voice card 1
	GPIO_SetDir(CVMUX_PORT_CARD2,1<<CVMUX_PIN_CARD2,1); // voice card 2
	GPIO_SetDir(CVMUX_PORT_VCA,1<<CVMUX_PIN_VCA,1); // VCAs
	
	// init wavetable oscs
	for(i=0;i<SYNTH_VOICE_COUNT;++i)
	{
		wtosc_init(&synth.osc[i][0],i,0);
		wtosc_init(&synth.osc[i][1],i,1);
	}

	// give it some memory
	waveData.curFile.lfname=waveData.lfname;
	waveData.curFile.lfsize=sizeof(waveData.lfname);
	
	// init subsystems
	dacspi_init();
	tuner_init();
	for(int8_t part=0;part<SYNTH_PART_COUNT;++part)
		assigner_init(&assigner[part]);
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

	for(int8_t part=0;part<SYNTH_PART_COUNT;++part)
	{
		lfo_init(&synth.lfo[part]);
		lfo_init(&synth.vibrato[part]);
		lfo_setShape(&synth.vibrato[part],lsTri);
		lfo_setSpeedShift(&synth.vibrato[part],4);
	}

	// load settings from storage; tune when they are bad
	
	if(!settings_load())
	{
		settings_loadDefault();
		tuner_tuneSynth();
	}

	// load last preset & do a full refresh
	
	if(!preset_loadCurrent(currentPart, settings.presetNumber))
		preset_loadDefault(currentPart, 1);
	ui_setPresetModified(0);

	refreshFullState();
	
	__enable_irq();
	refreshBankNames();

	refreshWaveNames(0);
	refreshWaveNames(1);
	refreshWaveforms(0);
	refreshWaveforms(1);
}


void synth_update(void)
{
#ifdef DEBUG
#if 0
	putc_serial0('.');	
#else
	static int32_t frc=0,prevTick=0;
	++frc;
	if(currentTick-prevTick>2*TICKER_1S)
	{
		rprintf(0,"%d u/s\n",frc/2);
		frc=0;
		prevTick=currentTick;
	}
#endif
#endif
	
	ui_update();
	
	refreshLfoSettings();
}

////////////////////////////////////////////////////////////////////////////////
// Synth interrupts
////////////////////////////////////////////////////////////////////////////////

// 2Khz
void synth_timerInterrupt(void)
{
	int32_t val,pitchAVal,pitchBVal,wmodAVal,wmodBVal,filterVal,ampVal,wmodEnvAmt,filEnvAmt,resoFactor;

	static int frc=0;

	// lfo
		
	for(int8_t part=0;part<SYNTH_PART_COUNT;++part)
		lfo_update(&synth.lfo[part]);
		
	// per voice stuff
	
	for(int8_t v=0;v<SYNTH_VOICE_COUNT;++v)
	{
		int8_t part=synth.voicePart[v];

		// pitch

		pitchAVal=pitchBVal=synth.vibrato[part].output;
		val=scaleU16S16(currentPreset[part].continuousParameters[cpLFOPitchAmt],synth.lfo[part].output>>1);

		if(currentPreset[part].steppedParameters[spLFOTargets]&otA)
			pitchAVal+=val;
		if(currentPreset[part].steppedParameters[spLFOTargets]&otB)
			pitchBVal+=val;

		// filter

		filterVal=scaleU16S16(currentPreset[part].continuousParameters[cpLFOFilAmt],synth.lfo[part].output);

		// amplifier

		ampVal=synth.partState[part].benderCVs[cvAmp];
		ampVal+=UINT16_MAX-scaleU16U16(currentPreset[part].continuousParameters[cpLFOAmpAmt],synth.lfo[part].levelCV);
		ampVal+=scaleU16S16(currentPreset[part].continuousParameters[cpLFOAmpAmt],synth.lfo[part].output);

		// part computations

		filEnvAmt=currentPreset[part].continuousParameters[cpFilEnvAmt];
		filEnvAmt+=INT16_MIN;

		wmodAVal=currentPreset[part].continuousParameters[cpABaseWMod];
		if(currentPreset[part].steppedParameters[spLFOTargets]&otA)
			wmodAVal+=scaleU16S16(currentPreset[part].continuousParameters[cpLFOWModAmt],synth.lfo[part].output);

		wmodBVal=currentPreset[part].continuousParameters[cpBBaseWMod];
		if(currentPreset[part].steppedParameters[spLFOTargets]&otB)
			wmodBVal+=scaleU16S16(currentPreset[part].continuousParameters[cpLFOWModAmt],synth.lfo[part].output);

		wmodEnvAmt=currentPreset[part].continuousParameters[cpWModFilEnv];
		wmodEnvAmt+=INT16_MIN;
		
		// restrict range

		pitchAVal=MAX(INT16_MIN,MIN(INT16_MAX,pitchAVal));
		pitchBVal=MAX(INT16_MIN,MIN(INT16_MAX,pitchBVal));
		wmodAVal=MAX(0,MIN(UINT16_MAX,wmodAVal));
		wmodBVal=MAX(0,MIN(UINT16_MAX,wmodBVal));
		filterVal=MAX(INT16_MIN,MIN(INT16_MAX,filterVal));
		ampVal=MAX(0,MIN(UINT16_MAX,ampVal));
		
		// compensate vca level for resonance
		
		resoFactor=(UINT16_MAX+2*(int32_t)currentPreset[part].continuousParameters[cpResonance])/(3*256);

		// actual voice refresh
		
		refreshVoice(v,wmodEnvAmt,filEnvAmt,pitchAVal,pitchBVal,wmodAVal,wmodBVal,filterVal,ampVal,synth.partState[part].wmodMask,resoFactor);
	}

	// slower updates
	
	switch(frc&0x03) // 4 phases, each 500hz
	{
	case 0:
		// CV update
		
		for(int8_t v=0;v<SYNTH_VOICE_COUNT;++v)
		{
			int8_t part=synth.voicePart[v];

			refreshCV(v,cvAVol,currentPreset[part].continuousParameters[cpAVol]);
			refreshCV(v,cvBVol,currentPreset[part].continuousParameters[cpBVol]);
			refreshCV(v,cvResonance,currentPreset[part].continuousParameters[cpResonance]);
		}

		refreshCV(SYNTH_VOICE_COUNT,cvMasterRight,currentPreset[currentPart].continuousParameters[cpMasterRight]);
		refreshCV(SYNTH_VOICE_COUNT,cvMasterLeft,currentPreset[currentPart].continuousParameters[cpMasterLeft]);

		break;
	case 1:
		// midi

		midi_update();

		break;
	case 2:
		// vibrato

		for(int8_t part=0;part<SYNTH_PART_COUNT;++part)
			lfo_update(&synth.vibrato[part]);

		break;
	case 3:
		// bit inputs (footswitch / tape in)
	
		handleBitInputs();
		
		// arpeggiator

		if(arp_getMode()!=amOff && (settings.syncMode==smInternal || synth.pendingExtClock))
		{
			if(synth.pendingExtClock)
				--synth.pendingExtClock;
			
			arp_update();
		}

		// glide
		
		for(int8_t v=0;v<SYNTH_VOICE_COUNT;++v)
		{
			int8_t part=synth.voicePart[v];
			int16_t amt=synth.partState[part].glideAmount;
			
			if(synth.partState[part].gliding)
			{
				computeGlide(&synth.oscANoteCV[v],synth.oscATargetCV[v],amt);
				computeGlide(&synth.oscBNoteCV[v],synth.oscBTargetCV[v],amt);
				computeGlide(&synth.filterNoteCV[v],synth.filterTargetCV[v],amt);
			}
		}

		// misc
		
		++currentTick;
		handleFinishedVoices();

		break;
	}

	++frc;
}

////////////////////////////////////////////////////////////////////////////////
// Synth internal events
////////////////////////////////////////////////////////////////////////////////

void synth_updateDACsEvent(int32_t start, int32_t count)
{
	int32_t end=start+count-1;
	
	wtosc_update(&synth.osc[0][0],start,end);
	wtosc_update(&synth.osc[0][1],start,end);
	wtosc_update(&synth.osc[1][0],start,end);
	wtosc_update(&synth.osc[1][1],start,end);
	wtosc_update(&synth.osc[3][0],start,end);
	wtosc_update(&synth.osc[3][1],start,end);
	wtosc_update(&synth.osc[4][0],start,end);
	wtosc_update(&synth.osc[4][1],start,end);
	wtosc_update(&synth.osc[2][0],start,end);
	wtosc_update(&synth.osc[2][1],start,end);
	wtosc_update(&synth.osc[5][0],start,end);
	wtosc_update(&synth.osc[5][1],start,end);
}

void synth_assignerEvent(uint8_t note, int8_t gate, int8_t voice, uint16_t velocity, int8_t legato)
{
#ifdef DEBUG_
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

		velAmt=currentPreset[synth.voicePart[voice]].continuousParameters[cpFilVelocity];
		adsr_setCVs(&synth.filEnvs[voice],0,0,0,0,(UINT16_MAX-velAmt)+scaleU16U16(velocity,velAmt),0x10);
		velAmt=currentPreset[synth.voicePart[voice]].continuousParameters[cpAmpVelocity];
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

void synth_wheelEvent(int16_t bend, uint16_t modulation, uint8_t mask)
{
#ifdef DEBUG_
	rprintf(0,"wheel bend %d mod %d mask %d\n",bend,modulation,mask);
#endif

	if(mask&1)
	{
		synth.partState[currentPart].benderAmount=bend;
		computeBenderCVs();
		computeTunedCVs();
		
	}
	
	if(mask&2)
	{
		synth.partState[currentPart].modwheelAmount=modulation;
		refreshLfoSettings();
	}
}

void synth_realtimeEvent(uint8_t midiEvent)
{
	if(settings.syncMode!=smMIDI)
		return;
	
	switch(midiEvent)
	{
		case MIDI_CLOCK:
			++synth.pendingExtClock;
			break;
		case MIDI_START:
			arp_resetCounter();
			synth.pendingExtClock=0;
			break;
	}
}
