///////////////////////////////////////////////////////////////////////////////
// Synthesizer
///////////////////////////////////////////////////////////////////////////////

#include "synth.h"
#include "wtosc.h"
#include "dacspi.h"
#include "ff.h"
#include "scan.h"
#include "ui.h"
#include "uart_midi.h"
#include "midi.h"
#include "adsr.h"
#include "lfo.h"
#include "tuner.h"
#include "assigner.h"
#include "arp.h"
#include "seq.h"
#include "clock.h"
#include "storage.h"
#include "vca_curves.h"
#include "vcnoise_curves.h"
#include "../xnormidi/midi.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"

#define BIT_INPUT_FOOTSWITCH (1<<26)

#define WAVEDATA_PATH "/WAVEDATA"

#define MAX_BANKS 75
#define MAX_BANK_WAVES 150
#define MAX_FILENAME _MAX_LFN

#define POT_DEAD_ZONE 512

volatile uint32_t currentTick=0; // 500hz

static struct
{
	int bankCount;
	char bankNames[MAX_BANKS][MAX_FILENAME];
	int curWaveCount[3];
	char waveNames[2][MAX_BANK_WAVES][MAX_FILENAME];

	uint16_t sampleData[4][WTOSC_MAX_SAMPLES]; // 0: OscA, 1: OscB, 2: XOvr source, 3: XOvr mix
	uint16_t sampleCount[4];

	DIR curDir;
	FILINFO curFile;
	char lfname[_MAX_LFN + 1];
} waveData;

static struct
{
	struct wtosc_s osc[SYNTH_VOICE_COUNT][2];
	struct adsr_s filEnvs[SYNTH_VOICE_COUNT];
	struct adsr_s ampEnvs[SYNTH_VOICE_COUNT];
	struct lfo_s lfo[2];
	
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
		uint32_t syncResetsMask;
		int32_t oldCrossOver;
	} partState;
	
	uint8_t pendingExtClock;
} synth;

extern const uint16_t attackCurveLookup[]; // for modulation delay

const uint16_t extClockDividers[16] = {192,168,144,128,96,72,48,36,24,18,12,9,6,4,3,2};

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
		if (!assigner_getAssignment(v,&note))
			continue;
		
		// get raw values

		mTuneRaw=currentPreset.continuousParameters[cpMasterTune];
		fineBFreqRaw=currentPreset.continuousParameters[cpBFineFreq];
		baseCutoffRaw=currentPreset.continuousParameters[cpCutoff];
		baseBPitchRaw=currentPreset.continuousParameters[cpBFreq];
		detuneRaw=currentPreset.continuousParameters[cpUnisonDetune];
		trackRaw=currentPreset.continuousParameters[cpFilKbdAmt];
		chrom=currentPreset.steppedParameters[spChromaticPitch];

		// compute for oscs & filters

		mTune=(mTuneRaw>>7)+INT8_MIN*2;
		fineBFreq=(fineBFreqRaw>>8)+INT8_MIN;
		baseCutoff=((uint32_t)baseCutoffRaw*3)>>2; // 75% of raw cutoff

		if(baseBPitchRaw>=HALF_RANGE)
		{
			baseAPitch=0;
			baseBPitch=(baseBPitchRaw-HALF_RANGE)>>1;
		}
		else
		{
			baseAPitch=(HALF_RANGE-baseBPitchRaw)>>1;
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
		
		cva=satAddU16S32(tuner_computeCVFromNote(v,baseANote+note,baseAPitch,cvAPitch),(int32_t)synth.partState.benderCVs[cvAPitch]+mTune);
		cvb=satAddU16S32(tuner_computeCVFromNote(v,baseBNote+note,baseBPitch,cvBPitch),(int32_t)synth.partState.benderCVs[cvBPitch]+mTune+fineBFreq);
		
		detune=(1+(v>>1))*(v&1?-1:1)*(detuneRaw>>9);
		cva=satAddU16S16(cva,detune);
		cvb=satAddU16S16(cvb,detune);
		
		// filter
		
		trackingNote=baseCutoffNote+((note*(trackRaw>>8))>>8);
			
		cvf=satAddU16S16(tuner_computeCVFromNote(v,trackingNote,baseCutoff,cvCutoff),synth.partState.benderCVs[cvCutoff]);
		
		// glide
		
		if(synth.partState.gliding)
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

	bend=synth.partState.benderAmount;

	switch(currentPreset.steppedParameters[spBenderTarget])
	{
	case modPitch:
		bend*=tuner_computeCVFromNote(0,br[currentPreset.steppedParameters[spBenderRange]]*2,0,cvAPitch)-tuner_computeCVFromNote(0,0,0,cvAPitch);
		bend/=UINT16_MAX;
		synth.partState.benderCVs[cvAPitch]=bend;
		synth.partState.benderCVs[cvBPitch]=bend;
		break;
	case modFil:
		bend*=tuner_computeCVFromNote(0,br[currentPreset.steppedParameters[spBenderRange]]*8,0,cvCutoff)-tuner_computeCVFromNote(0,0,0,cvCutoff);
		bend/=UINT16_MAX;
		synth.partState.benderCVs[cvCutoff]=bend;
		break;
	case modAmp:
		bend=(bend*br[currentPreset.steppedParameters[spBenderRange]])/12;
		synth.partState.benderCVs[cvAmp]=bend;
		break;
	}
}

static void handleFinishedVoices(void)
{
	int8_t v;
	
	for(v=0;v<SYNTH_VOICE_COUNT;++v)
	{
		// when amp env finishes, voice is done
		if(assigner_getAssignment(v,NULL) && adsr_getStage(&synth.ampEnvs[v])==sWait)
			assigner_voiceDone(v);
	
		// if voice isn't assigned, silence it
		if(!assigner_getAssignment(v,NULL) && adsr_getStage(&synth.ampEnvs[v])!=sWait)
		{
			adsr_reset(&synth.ampEnvs[v]);
			adsr_reset(&synth.filEnvs[v]);
		}
	}
}

static void refreshAssignerSettings(void)
{
	static const uint8_t vc2msk[7] = {1, 3, 7, 15, 31, 63};
 
	if(currentPreset.steppedParameters[spUnison]!=assigner_getMono())
	{
		if(currentPreset.steppedParameters[spUnison])
			assigner_latchPattern();
		else
			assigner_setPoly();
		
		assigner_getPattern(currentPreset.voicePattern,NULL);
	}

	assigner_setPattern(currentPreset.voicePattern,currentPreset.steppedParameters[spUnison]);
	assigner_setPriority(currentPreset.steppedParameters[spAssignerPriority]);
	assigner_setVoiceMask(vc2msk[currentPreset.steppedParameters[spVoiceCount]]);
}

static void refreshEnvSettings(int8_t type)
{
	int8_t slow;
	int8_t i;
	struct adsr_s * a;
		
	for(i=0;i<SYNTH_VOICE_COUNT;++i)
	{
		slow=currentPreset.steppedParameters[(type)?spFilEnvSlow:spAmpEnvSlow];

		if(type)
		{
			a=&synth.filEnvs[i];
			adsr_setShape(a,currentPreset.steppedParameters[spFilEnvLin]?0:1);
		}
		else
		{
			a=&synth.ampEnvs[i];
		}

		adsr_setSpeedShift(a,(slow)?4:2);

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
}

static void refreshLfoSettings(void)
{
	static const int8_t mr[]={5,3,1,0};
	uint16_t mwAmt,lfoAmt,lfo2Amt,dlyAmt;
	uint32_t elapsed;

	lfo_setShape(&synth.lfo[0],currentPreset.steppedParameters[spLFOShape]);
	lfo_setShape(&synth.lfo[1],currentPreset.steppedParameters[spLFO2Shape]);
	
	lfo_setSpeedShift(&synth.lfo[0],currentPreset.steppedParameters[spLFOShift]*3);
	lfo_setSpeedShift(&synth.lfo[1],currentPreset.steppedParameters[spLFO2Shift]*3);

	// wait modulationDelayTickCount then progressively increase over
	// modulationDelayTickCount time, following an exponential curve
	dlyAmt=0;
	if(synth.partState.modulationDelayStart!=UINT32_MAX)
	{
		if(currentPreset.continuousParameters[cpModDelay]<POT_DEAD_ZONE)
		{
			dlyAmt=UINT16_MAX;
		}
		else if(currentTick>=synth.partState.modulationDelayStart+synth.partState.modulationDelayTickCount)
		{
			elapsed=currentTick-(synth.partState.modulationDelayStart+synth.partState.modulationDelayTickCount);
			if(elapsed>=synth.partState.modulationDelayTickCount)
				dlyAmt=UINT16_MAX;
			else
				dlyAmt=attackCurveLookup[(elapsed<<8)/synth.partState.modulationDelayTickCount];
		}
	}

	mwAmt=synth.partState.modwheelAmount>>mr[currentPreset.steppedParameters[spModwheelRange]];
	lfoAmt=currentPreset.continuousParameters[cpLFOAmt];
	lfoAmt=(lfoAmt<POT_DEAD_ZONE)?0:(lfoAmt-POT_DEAD_ZONE);

	lfo2Amt=currentPreset.continuousParameters[cpLFO2Amt];
	lfo2Amt=(lfo2Amt<POT_DEAD_ZONE)?0:(lfo2Amt-POT_DEAD_ZONE);

	if(currentPreset.steppedParameters[spModwheelTarget]==0) // targeting lfo1?
	{
		lfo_setCVs(&synth.lfo[0],
				currentPreset.continuousParameters[cpLFOFreq],
				satAddU16U16(lfoAmt,mwAmt));
		lfo_setCVs(&synth.lfo[1],
				 currentPreset.continuousParameters[cpLFO2Freq],
				 scaleU16U16(lfo2Amt,dlyAmt));
	}
	else
	{
		lfo_setCVs(&synth.lfo[0],
				currentPreset.continuousParameters[cpLFOFreq],
				scaleU16U16(lfoAmt,dlyAmt));
		lfo_setCVs(&synth.lfo[1],
				currentPreset.continuousParameters[cpLFO2Freq],
				satAddU16U16(lfo2Amt,mwAmt));
	}
}

static void refreshModulationDelay(int8_t refreshTickCount)
{
	int8_t anyPressed, anyAssigned;
	static int8_t prevAnyPressed=0;
	
	anyPressed=assigner_getAnyPressed();	
	anyAssigned=assigner_getAnyAssigned();	

	if(!anyAssigned)
	{
		synth.partState.modulationDelayStart=UINT32_MAX;
	}

	if(anyPressed && !prevAnyPressed)
	{
		synth.partState.modulationDelayStart=currentTick;
	}

	prevAnyPressed=anyPressed;

	if(refreshTickCount)
		synth.partState.modulationDelayTickCount=exponentialCourse(UINT16_MAX-currentPreset.continuousParameters[cpModDelay],12000.0f,2500.0f);
}

static void refreshMisc(void)
{
	// arp

	clock_setSpeed(settings.seqArpClock);

	// glide

	synth.partState.glideAmount=exponentialCourse(currentPreset.continuousParameters[cpGlide],11000.0f,2100.0f);
	synth.partState.gliding=synth.partState.glideAmount<2000;

	// WaveMod mask

	synth.partState.wmodMask=0;
	if(currentPreset.steppedParameters[spAWModType]==wmAliasing)
		synth.partState.wmodMask|=1;
	if(currentPreset.steppedParameters[spAWModType]==wmWidth)
		synth.partState.wmodMask|=2;
	if(currentPreset.steppedParameters[spAWModType]==wmFrequency)
		synth.partState.wmodMask|=4;
	if(currentPreset.steppedParameters[spAWModEnvEn])
		synth.partState.wmodMask|=8;

	if(currentPreset.steppedParameters[spBWModType]==wmAliasing)
		synth.partState.wmodMask|=16;
	if(currentPreset.steppedParameters[spBWModType]==wmWidth)
		synth.partState.wmodMask|=32;
	if(currentPreset.steppedParameters[spBWModType]==wmFrequency)
		synth.partState.wmodMask|=64;
	if(currentPreset.steppedParameters[spBWModEnvEn])
		synth.partState.wmodMask|=128;
	
	// waveforms
	
	for(int i=0;i<SYNTH_VOICE_COUNT;++i)
	{
		int p=currentPreset.steppedParameters[spAWModType]==wmCrossOver?3:0;
		wtosc_setSampleData(&synth.osc[i][0],waveData.sampleData[p],waveData.sampleCount[p]);
		wtosc_setSampleData(&synth.osc[i][1],waveData.sampleData[1],waveData.sampleCount[1]);
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

int8_t appendBankName(int8_t abx, char * path)
{
	uint8_t bankNum;
	static const uint8_t abx2sp[3] = {spABank, spBBank, spXOvrBank};

	bankNum=currentPreset.steppedParameters[abx2sp[abx]];

	if(bankNum>=waveData.bankCount)
		return 0;

	strcat(path,waveData.bankNames[bankNum]);
	
	return 1;
}

int8_t appendWaveName(int8_t abx, char * path)
{
	uint8_t waveNum;
	static const uint8_t abx2sp[3] = {spAWave, spBWave, spXOvrWave};

	waveNum=currentPreset.steppedParameters[abx2sp[abx]];

	if(abx==2)
	{
		refreshWaveNames(2);
		if(waveNum>=waveData.curWaveCount[1])
			return 0;
		strcat(path,waveData.waveNames[1][waveNum]);
		refreshWaveNames(1);
	}
	else
	{
		if(waveNum>=waveData.curWaveCount[abx])
			return 0;
		strcat(path,waveData.waveNames[abx][waveNum]);
	}

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

void refreshWaveNames(int8_t abx)
{
	FRESULT res;
	char fn[256];
	int8_t ab=MIN(1,abx); // CrossMod waveNames are temporarily stored in B waveNames to save RAM
	
	waveData.curWaveCount[ab]=0;

	strcpy(fn,WAVEDATA_PATH "/");
	if(!appendBankName(abx,fn)) return;

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

void refreshWaveforms(int8_t abx)
{
	int i;
	FIL f;
	FRESULT res;
	char fn[256];
	int16_t data[WTOSC_MAX_SAMPLES];
	int32_t d;
	int32_t smpSize = 0;
	
	strcpy(fn,WAVEDATA_PATH "/");
	if(!appendBankName(abx,fn)) return;
	strcat(fn,"/");
	if(!appendWaveName(abx,fn)) return;

	rprintf(0,"loading %s\n",fn);
	
	if(!f_open(&f,fn,FA_READ))
	{

		if((res=f_lseek(&f,0x28)))
			rprintf(0,"f_lseek res=%d\n",res);

		if((res=f_read(&f,&smpSize,sizeof(smpSize),&i)))
			rprintf(0,"f_read res=%d\n",res);
		
		smpSize=MIN(smpSize,WTOSC_MAX_SAMPLES*2);
		
		rprintf(0, "smpSize %d\n", smpSize);

		if((res=f_read(&f,data,smpSize,&i)))
			rprintf(0,"f_read res=%d\n",res);

		f_close(&f);
		
		for(i=0;i<WTOSC_MAX_SAMPLES;++i)
		{
			d=data[i];
			d=(d*(INT16_MAX-WTOSC_SAMPLES_GUARD_BAND))>>15;
			d-=INT16_MIN;
			waveData.sampleData[abx][i]=d;
		}
		
		waveData.sampleCount[abx]=smpSize>>1; 
	}
	
	// XOvr mix waveform
	waveData.sampleCount[3]=MAX(waveData.sampleCount[0], waveData.sampleCount[2]); 
	
	synth.partState.oldCrossOver=-1;
}

static void handleBitInputs(void)
{
	uint32_t cur;
	static uint32_t last=0;
	
	cur=GPIO_ReadValue(3);
	
	// control footswitch 
	 
	if(arp_getMode()==amOff && currentPreset.steppedParameters[spUnison] && !(cur&BIT_INPUT_FOOTSWITCH) && last&BIT_INPUT_FOOTSWITCH)
	{
		assigner_latchPattern();
		assigner_getPattern( currentPreset.voicePattern,NULL);
	}
	else if((cur&BIT_INPUT_FOOTSWITCH)!=(last&BIT_INPUT_FOOTSWITCH))
	{
		if(arp_getMode()!=amOff)
		{
			arp_setMode(arp_getMode(),(cur&BIT_INPUT_FOOTSWITCH)?0:1);
		}
		else
		{
			assigner_holdEvent((cur&BIT_INPUT_FOOTSWITCH)?0:1);
		}
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
		value=(6*value)/16; // limit VCA output level to 4Vpp
		value=computeShape(value<<8,vcaLinearizationCurve,1);		
		break;
	case cvNoiseVol:
		value=computeShape(value<<8,vcNoiseLinearizationCurve,1);		
		break;
	default:
		;
	}
	
	return value;
}

static FORCEINLINE void refreshCV(int8_t voice, cv_t cv, uint32_t v)
{
	uint16_t value,channel;
	
	v=__USAT(v,16);
	value=adjustCV(cv,v);

	switch(cv)
	{
	case cvAmp:
		channel=voice;
		break;
	case cvNoiseVol:
		channel=6;
		break;
	case cvResonance:
		channel=7;
		break;
	case cvCutoff:
		channel=voice+8;
		break;
	case cvAVol:
		channel=14;
		break;
	case cvBVol:
		channel=15;
		break;
	default:
		return;
	}
	
	dacspi_setCVValue(channel,value);
}

static void refreshCrossOver(int32_t wmod, int32_t wmodEnvAmt)
{
	int i;
	int8_t v;
	uint32_t xovr;
	uint16_t *p0,*p2,*p3;
	
	xovr=wmod;

	// paraphonic filter envelope mod
	if(currentPreset.steppedParameters[spAWModEnvEn])
	{
		v=assigner_getLatestAssigned(NULL);
		if(v>=0)
		{
			xovr+=scaleU16S16(synth.filEnvs[v].output,wmodEnvAmt)<<1;
			xovr=__USAT(xovr,16);
		}
	}
	
	if(xovr==synth.partState.oldCrossOver)
		return;

	p0=waveData.sampleData[0];
	p2=waveData.sampleData[2];
	p3=waveData.sampleData[3];
	
	for(i=0;i<WTOSC_MAX_SAMPLES/4;++i)
	{
		*p3++=lerp16(*p0++,*p2++,xovr);
		*p3++=lerp16(*p0++,*p2++,xovr);
		*p3++=lerp16(*p0++,*p2++,xovr);
		*p3++=lerp16(*p0++,*p2++,xovr);
	}
	
	synth.partState.oldCrossOver=xovr;
}

static FORCEINLINE void refreshVoice(int8_t v,int32_t wmodEnvAmt,int32_t filEnvAmt,int32_t pitchAVal,int32_t pitchBVal,int32_t wmodAVal,int32_t wmodBVal,int32_t filterVal,int32_t ampVal,uint8_t wmodMask)
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
		vma=__USAT(vma,16);
	}

	vmb=wmodBVal;
	if(wmodMask&128)
	{
		vmb+=envValScale;
		vmb=__USAT(vmb,16);
	}

	vpa=pitchAVal;
	if(wmodMask&4)
		vpa+=vma-HALF_RANGE;

	vpb=pitchBVal;
	if(wmodMask&64)
		vpb+=vmb-HALF_RANGE;

	// osc A

	vpa+=synth.oscANoteCV[v];
	vpa=__USAT(vpa,16);
	wtosc_setParameters(&synth.osc[v][0],vpa,(wmodMask&1)?vma:0,(wmodMask&2)?vma:HALF_RANGE);

	// osc B

	vpb+=synth.oscBNoteCV[v];
	vpb=__USAT(vpb,16);
	wtosc_setParameters(&synth.osc[v][1],vpb,(wmodMask&16)?vmb:0,(wmodMask&32)?vmb:HALF_RANGE);

	// amplifier
	
	vamp=scaleU16U16(synth.ampEnvs[v].output,ampVal);
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
	
	// init wavetable oscs
	for(i=0;i<SYNTH_VOICE_COUNT;++i)
	{
		wtosc_init(&synth.osc[i][0],i*2);
		wtosc_init(&synth.osc[i][1],i*2+1);
	}

	// give it some memory
	waveData.curFile.lfname=waveData.lfname;
	waveData.curFile.lfsize=sizeof(waveData.lfname);

	for(i=0;i<4;++i)
		waveData.sampleCount[i]=WTOSC_MAX_SAMPLES;
	
	// init subsystems
	// ui_init() done in main.c
	dacspi_init();
	scan_init();
	tuner_init();
	assigner_init();
	uartMidi_init();
	seq_init();
	arp_init();
	midi_init();
	
	for(i=0;i<SYNTH_VOICE_COUNT;++i)
	{
		adsr_init(&synth.ampEnvs[i]);
		adsr_init(&synth.filEnvs[i]);

		adsr_setShape(&synth.ampEnvs[i],1);
		adsr_setShape(&synth.filEnvs[i],1);
	}

	lfo_init(&synth.lfo[0]);
	lfo_init(&synth.lfo[1]);

	// load settings from storage; tune when they are bad
	
	if(!settings_load())
	{
		settings_loadDefault();
		tuner_tuneSynth();
	}

	// load last preset & do a full refresh
	
	if(!preset_loadCurrent(settings.presetNumber))
		preset_loadDefault(1);
	ui_setPresetModified(0);

	refreshFullState();
	
	__enable_irq();
	refreshBankNames();

	refreshWaveNames(0);
	refreshWaveNames(1);
	refreshWaveforms(0);
	refreshWaveforms(1);
	refreshWaveforms(2);
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
	
	scan_update();
	ui_update();
	
	refreshLfoSettings();
	
	synth.partState.syncResetsMask=currentPreset.steppedParameters[spOscSync]?UINT32_MAX:0;
}

////////////////////////////////////////////////////////////////////////////////
// Synth interrupts
////////////////////////////////////////////////////////////////////////////////

// 4Khz
void synth_timerInterrupt(void)
{
	int32_t val,pitchAVal,pitchBVal,wmodAVal,wmodBVal,filterVal,ampVal,resVal,wmodEnvAmt,filEnvAmt,resoFactor;

	static int frc=0;

	// lfos
		
	lfo_update(&synth.lfo[0]);
	lfo_update(&synth.lfo[1]);
		
	// global computations
	
		// pitch

	pitchAVal=pitchBVal=0;

	val=scaleU16S16(currentPreset.continuousParameters[cpLFOPitchAmt],synth.lfo[0].output>>1);
	if(currentPreset.steppedParameters[spLFOTargets]&otA)
		pitchAVal+=val;
	if(currentPreset.steppedParameters[spLFOTargets]&otB)
		pitchBVal+=val;

	val=scaleU16S16(currentPreset.continuousParameters[cpLFO2PitchAmt],synth.lfo[1].output>>1);
	if(currentPreset.steppedParameters[spLFO2Targets]&otA)
		pitchAVal+=val;
	if(currentPreset.steppedParameters[spLFO2Targets]&otB)
		pitchBVal+=val;

		// filter

	filterVal=scaleU16S16(currentPreset.continuousParameters[cpLFOFilAmt],synth.lfo[0].output);
	filterVal+=scaleU16S16(currentPreset.continuousParameters[cpLFO2FilAmt],synth.lfo[1].output);
	
		// amplifier

	ampVal=synth.partState.benderCVs[cvAmp]+UINT16_MAX;

	ampVal-=scaleU16U16(currentPreset.continuousParameters[cpLFOAmpAmt],synth.lfo[0].levelCV>>1);
	ampVal+=scaleU16S16(currentPreset.continuousParameters[cpLFOAmpAmt],synth.lfo[0].output);

	ampVal-=scaleU16U16(currentPreset.continuousParameters[cpLFO2AmpAmt],synth.lfo[1].levelCV>>1);
	ampVal+=scaleU16S16(currentPreset.continuousParameters[cpLFO2AmpAmt],synth.lfo[1].output);

		// misc

	filEnvAmt=currentPreset.continuousParameters[cpFilEnvAmt];
	filEnvAmt+=INT16_MIN;

	wmodAVal=currentPreset.continuousParameters[cpABaseWMod];
	if(currentPreset.steppedParameters[spLFOTargets]&otA)
		wmodAVal+=scaleU16S16(currentPreset.continuousParameters[cpLFOWModAmt],synth.lfo[0].output);
	if(currentPreset.steppedParameters[spLFO2Targets]&otA)
		wmodAVal+=scaleU16S16(currentPreset.continuousParameters[cpLFO2WModAmt],synth.lfo[1].output);

	wmodBVal=currentPreset.continuousParameters[cpBBaseWMod];
	if(currentPreset.steppedParameters[spLFOTargets]&otB)
		wmodBVal+=scaleU16S16(currentPreset.continuousParameters[cpLFOWModAmt],synth.lfo[0].output);
	if(currentPreset.steppedParameters[spLFO2Targets]&otB)
		wmodBVal+=scaleU16S16(currentPreset.continuousParameters[cpLFO2WModAmt],synth.lfo[1].output);

	wmodEnvAmt=currentPreset.continuousParameters[cpWModFilEnv];
	wmodEnvAmt+=INT16_MIN;

		// restrict range

	pitchAVal=__SSAT(pitchAVal,16);
	pitchBVal=__SSAT(pitchBVal,16);
	wmodAVal=__USAT(wmodAVal,16);
	wmodBVal=__USAT(wmodBVal,16);
	filterVal=__SSAT(filterVal,16);
	ampVal=__USAT(ampVal,16);

	// voices computations
		
	for(int8_t v=0;v<SYNTH_VOICE_COUNT;++v)
		refreshVoice(v,wmodEnvAmt,filEnvAmt,pitchAVal,pitchBVal,wmodAVal,wmodBVal,filterVal,ampVal,synth.partState.wmodMask);

	// slower updates
	
	switch(frc&0x03) // 4 phases, each 1Khz
	{
	case 0:
		resVal=currentPreset.continuousParameters[cpResonance];
		resVal+=scaleU16S16(currentPreset.continuousParameters[cpLFOResAmt],synth.lfo[0].output);
		resVal+=scaleU16S16(currentPreset.continuousParameters[cpLFO2ResAmt],synth.lfo[1].output);
		resVal=__USAT(resVal,16);

		// compensate pre filter mixer level for resonance

		resoFactor=(30*UINT16_MAX+110*(uint32_t)MAX(0,resVal-6000))/(100*256);

		// CV update
		
		refreshCV(-1,cvAVol,(uint32_t)currentPreset.continuousParameters[cpAVol]*resoFactor/256);
		refreshCV(-1,cvBVol,(uint32_t)currentPreset.continuousParameters[cpBVol]*resoFactor/256);
		refreshCV(-1,cvNoiseVol,(uint32_t)currentPreset.continuousParameters[cpNoiseVol]*resoFactor/256);
		refreshCV(-1,cvResonance,resVal);

		break;
	case 1:
		// midi

		midi_update();

		break;
	case 2:
		// crossover

		if(currentPreset.steppedParameters[spAWModType]==wmCrossOver)
			refreshCrossOver(wmodAVal,wmodEnvAmt);

		break;
	case 3:
		// bit inputs (footswitch / tape in)
	
		handleBitInputs();
		
		// assigner
		
		handleFinishedVoices();
		
		if(frc&0x04)
		{
			// clocking

			if(settings.syncMode==smInternal || synth.pendingExtClock)
			{
				if(synth.pendingExtClock)
					--synth.pendingExtClock;

				if (clock_update())
				{
					// sequencer

					if(seq_getMode(0)!=smOff || seq_getMode(1)!=smOff)
						seq_update();

					// arpeggiator

					if(arp_getMode()!=amOff)
						arp_update();
				}
			}
		}
		else
		{
			// glide

			for(int8_t v=0;v<SYNTH_VOICE_COUNT;++v)
			{
				int16_t amt=synth.partState.glideAmount;

				if(synth.partState.gliding)
				{
					computeGlide(&synth.oscANoteCV[v],synth.oscATargetCV[v],amt);
					computeGlide(&synth.oscBNoteCV[v],synth.oscBTargetCV[v],amt);
					computeGlide(&synth.filterNoteCV[v],synth.filterTargetCV[v],amt);
				}
			}

			// 500hz tick counter

			++currentTick;
		}

		break;
	}

	++frc;
}

////////////////////////////////////////////////////////////////////////////////
// Synth internal events
////////////////////////////////////////////////////////////////////////////////

#define PROC_UPDATE_DACS_VOICE(v) \
FORCEINLINE static void updateDACsVoice##v(int32_t start, int32_t end) \
{ \
	uint32_t syncResets; /* /!\ this won't work if count > 32 */ \
	wtosc_update(&synth.osc[v][0],start,end,osmMaster,&syncResets); \
	syncResets&=synth.partState.syncResetsMask; \
	wtosc_update(&synth.osc[v][1],start,end,osmSlave,&syncResets); \
}

PROC_UPDATE_DACS_VOICE(0);
PROC_UPDATE_DACS_VOICE(1);
PROC_UPDATE_DACS_VOICE(2);
PROC_UPDATE_DACS_VOICE(3);
PROC_UPDATE_DACS_VOICE(4);
PROC_UPDATE_DACS_VOICE(5);

void synth_updateDACsEvent(int32_t start, int32_t count)
{
	int32_t end=start+count-1;

	updateDACsVoice0(start,end);
	updateDACsVoice1(start,end);
	updateDACsVoice2(start,end);
	updateDACsVoice3(start,end);
	updateDACsVoice4(start,end);
	updateDACsVoice5(start,end);
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

void synth_wheelEvent(int16_t bend, uint16_t modulation, uint8_t mask)
{
#ifdef DEBUG_
	rprintf(0,"wheel bend %d mod %d mask %d\n",bend,modulation,mask);
#endif

	if(mask&1)
	{
		synth.partState.benderAmount=bend;
		computeBenderCVs();
		computeTunedCVs();
		
	}
	
	if(mask&2)
	{
		synth.partState.modwheelAmount=modulation;
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
			seq_resetCounter(0,0);
			seq_resetCounter(1,0);
			arp_resetCounter(0);
			clock_reset(); // always do a beat reset on MIDI START
			synth.pendingExtClock=0;
			break;
		case MIDI_STOP:
			seq_silence(0);
			seq_silence(1);
			break;
	}
}