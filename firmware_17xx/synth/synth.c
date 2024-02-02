///////////////////////////////////////////////////////////////////////////////
// Synthesizer
///////////////////////////////////////////////////////////////////////////////

#include "synth.h"
#include "wtosc.h"
#include "dacspi.h"
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
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_pinsel.h"
#include "main.h"

#define BIT_INPUT_FOOTSWITCH (1<<26)

#define WAVEDATA_PATH "/WAVEDATA"

#define MAX_BANKS 100
#define MAX_BANK_WAVES 200

#define POT_DEAD_ZONE 512

volatile uint32_t currentTick=0; // 500hz

static struct
{
	int bankCount;
	int curWaveCount;
	char bankNames[MAX_BANKS][MAX_FILENAME];
	char curWaveNames[MAX_BANK_WAVES][MAX_FILENAME];
	
	int8_t bankSorted;
	int8_t curWaveSorted;
	int8_t curWaveABX;
	char curWaveBank[128];
	
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
	struct adsr_s wmodEnvs[SYNTH_VOICE_COUNT];
	struct lfo_s lfo[2];
	
	uint16_t oscANoteCV[SYNTH_VOICE_COUNT];
	uint16_t oscBNoteCV[SYNTH_VOICE_COUNT];
	uint16_t filterNoteCV[SYNTH_VOICE_COUNT]; 
	
	uint16_t oscATargetCV[SYNTH_VOICE_COUNT];
	uint16_t oscBTargetCV[SYNTH_VOICE_COUNT];
	uint16_t filterTargetCV[SYNTH_VOICE_COUNT];

	struct
	{
		int16_t benderAmount;
		uint16_t modwheelAmount;
		uint16_t pressureAmount;
		int16_t glideAmount;
		int8_t gliding;

		uint32_t modulationDelayStart;
		uint16_t modulationDelayTickCount;

		uint8_t wmodMask;
		uint32_t syncResetsMask;
		int32_t oldCrossOver;

		uint16_t wmodAVal;
		int16_t wmodAEnvAmt;
	} partState;
	
	uint8_t pendingExtClock;
} synth;

extern const uint16_t attackCurveLookup[]; // for modulation delay

const uint16_t extClockDividers[16] = {192,168,144,128,96,72,48,36,24,18,12,9,6,4,3,2};

static const uint8_t abx2bsp[3] = {spABank_Legacy, spBBank_Legacy, spXOvrBank_Legacy};
static const uint8_t abx2wsp[3] = {spAWave_Legacy, spBWave_Legacy, spXOvrWave_Legacy};

////////////////////////////////////////////////////////////////////////////////
// Non speed critical internal code
////////////////////////////////////////////////////////////////////////////////

static int32_t getStaticCV(cv_t cv)
{
	static const modulationTarget_t cv2mod[cvCount]={modVolume,modVolume,modFilter,modNone,modPitch,modPitch,modCrossOver,modNone,modNone};
	modulationTarget_t mod=cv2mod[cv];
	int32_t res=0;
	
	if(mod!=modNone)
	{
		if(currentPreset.steppedParameters[spBenderTarget]==mod)
			res+=synth.partState.benderAmount;
		if(currentPreset.steppedParameters[spPressureTarget]==mod)
			res+=synth.partState.pressureAmount;
	}
	
	return __SSAT(res,16);
}

static void computeTunedCVs(void)
{
	uint16_t cva,cvb,cvf;
	uint8_t note,baseCutoffNote,baseANote,baseBNote,trackingNote;
	int8_t v;

	uint16_t baseAPitch,baseBPitch,baseCutoff;
	int16_t mTune,detune,unisonDetune;

	uint16_t baseCutoffRaw,mTuneRaw,detuneRaw,unisonDetuneRaw,trackRaw;
	uint8_t chrom;
	
	int32_t add;
	
	for(v=0;v<SYNTH_VOICE_COUNT;++v)
	{
		if (!assigner_getAssignment(v,&note))
			continue;
		
		// get raw values

		mTuneRaw=currentPreset.continuousParameters[cpMasterTune];
		detuneRaw=currentPreset.continuousParameters[cpDetune];
		baseCutoffRaw=currentPreset.continuousParameters[cpCutoff];
		baseAPitch=currentPreset.continuousParameters[cpAFreq]>>2;
		baseBPitch=currentPreset.continuousParameters[cpBFreq]>>2;
		unisonDetuneRaw=currentPreset.continuousParameters[cpUnisonDetune];
		trackRaw=currentPreset.continuousParameters[cpFilKbdAmt];
		chrom=currentPreset.steppedParameters[spChromaticPitch];

		// compute for oscs & filters

		mTune=(mTuneRaw>>7)+INT8_MIN*2;
		detune=(detuneRaw>>8)+INT8_MIN;
		baseCutoff=((uint32_t)baseCutoffRaw*3)>>2; // 75% of raw cutoff

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
		
		add=getStaticCV(cvAPitch)+mTune-(detune>>1);
		cva=satAddU16S32(tuner_computeCVFromNote(v,baseANote+note,baseAPitch,cvAPitch),add);
		
		add=getStaticCV(cvBPitch)+mTune+(detune>>1);
		cvb=satAddU16S32(tuner_computeCVFromNote(v,baseBNote+note,baseBPitch,cvBPitch),add);
		
		unisonDetune=(1+(v>>1))*(v&1?-1:1)*(unisonDetuneRaw>>9);
		cva=satAddU16S16(cva,unisonDetune);
		cvb=satAddU16S16(cvb,unisonDetune);
		
		// filter
		
		trackingNote=MAX(0,baseCutoffNote+((((int8_t)note-MIDDLE_C_NOTE)*(trackRaw>>8))>>8));
			
		add=getStaticCV(cvCutoff);
		cvf=satAddU16S32(tuner_computeCVFromNote(v,trackingNote,baseCutoff,cvCutoff),add);
		
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
			adsr_reset(&synth.wmodEnvs[v]);
		}
	}
}

static void refreshAssignerSettings(void)
{
	static const uint8_t vc2msk[7]={1,3,7,15,31,63};
 
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
	int8_t slow,lin,loop;
	int8_t i;
	struct adsr_s * a;
		
	for(i=0;i<SYNTH_VOICE_COUNT;++i)
	{
		switch(type)
		{
		case 0:
			a=&synth.ampEnvs[i];
			slow=currentPreset.steppedParameters[spAmpEnvSlow];
			lin=currentPreset.steppedParameters[spAmpEnvLin];
			loop=currentPreset.steppedParameters[spAmpEnvLoop];
			break;
		case 1:
			a=&synth.filEnvs[i];
			slow=currentPreset.steppedParameters[spFilEnvSlow];
			lin=currentPreset.steppedParameters[spFilEnvLin];
			loop=currentPreset.steppedParameters[spFilEnvLoop];
			break;
		case 2:
			a=&synth.wmodEnvs[i];
			slow=currentPreset.steppedParameters[spWModEnvSlow];
			lin=currentPreset.steppedParameters[spWModEnvLin];
			loop=currentPreset.steppedParameters[spWModEnvLoop];
			break;
		default:
			return;
		}
		
		adsr_setSpeedShift(a,(slow)?4:2,5);
		adsr_setShape(a,(lin)?0:1,loop);

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

		adsr_setCVs(&synth.wmodEnvs[i],
				 currentPreset.continuousParameters[cpWModAtt],
				 currentPreset.continuousParameters[cpWModDec],
				 currentPreset.continuousParameters[cpWModSus],
				 currentPreset.continuousParameters[cpWModRel],
				 0,0x0f);
	}
}

static void refreshLfoSettings(void)
{
	uint16_t lfoAmt,lfo2Amt,dlyAmt;
	uint32_t elapsed;

	lfo_setShape(&synth.lfo[0],currentPreset.steppedParameters[spLFOShape]);
	lfo_setShape(&synth.lfo[1],currentPreset.steppedParameters[spLFO2Shape]);
	
	lfo_setSpeedShift(&synth.lfo[0],3,5);
	lfo_setSpeedShift(&synth.lfo[1],3,5);

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

	lfoAmt=currentPreset.continuousParameters[cpLFOAmt];
	lfoAmt=(lfoAmt<POT_DEAD_ZONE)?0:(lfoAmt-POT_DEAD_ZONE);
	if(currentPreset.steppedParameters[spPressureTarget]==modLFO1)
		lfoAmt=satAddU16U16(lfoAmt,synth.partState.pressureAmount);

	lfo2Amt=currentPreset.continuousParameters[cpLFO2Amt];
	lfo2Amt=(lfo2Amt<POT_DEAD_ZONE)?0:(lfo2Amt-POT_DEAD_ZONE);
	if(currentPreset.steppedParameters[spPressureTarget]==modLFO2)
		lfo2Amt=satAddU16U16(lfo2Amt,synth.partState.pressureAmount);

	if(currentPreset.steppedParameters[spModwheelTarget]==0) // targeting lfo1?
	{
		lfo_setCVs(&synth.lfo[0],
				currentPreset.continuousParameters[cpLFOFreq],
				satAddU16U16(lfoAmt,synth.partState.modwheelAmount));
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
				satAddU16U16(lfo2Amt,synth.partState.modwheelAmount));
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

	if(currentPreset.steppedParameters[spBWModType]==wmAliasing)
		synth.partState.wmodMask|=16;
	if(currentPreset.steppedParameters[spBWModType]==wmWidth)
		synth.partState.wmodMask|=32;
	if(currentPreset.steppedParameters[spBWModType]==wmFrequency)
		synth.partState.wmodMask|=64;
	
	// waveforms
	
	for(int i=0;i<SYNTH_VOICE_COUNT;++i)
	{
		int p=(currentPreset.steppedParameters[spAWModType]==wmCrossOver ||
				currentPreset.steppedParameters[spBenderTarget]==modCrossOver ||
				currentPreset.steppedParameters[spPressureTarget]==modCrossOver)?3:0;
		wtosc_setSampleData(&synth.osc[i][0],waveData.sampleData[p],waveData.sampleCount[p]);
		wtosc_setSampleData(&synth.osc[i][1],waveData.sampleData[1],waveData.sampleCount[1]);
	}
}

void synth_refreshFullState(void)
{
	refreshModulationDelay(1);
	refreshAssignerSettings();
	refreshLfoSettings();
	refreshEnvSettings(0);
	refreshEnvSettings(1);
	refreshEnvSettings(2);
	refreshMisc();
	computeTunedCVs();
}

int32_t synth_getVisualEnvelope(int8_t voice)
{
	if(assigner_getAssignment(voice,NULL))
		return synth.ampEnvs[voice].output;
	else
		return -1;
}

void synth_reloadLegacyBankWaveIndexes(int8_t abx, int8_t loadDefault, int8_t sort)
{
	int i,bankNum,waveNum,oriBankNum,oriWaveNum,newBankNum,newWaveNum;
	char fn[MAX_FILENAME];

	if(loadDefault)
	{
		bankNum=-1;
		waveNum=-1;
	}
	else
	{
		bankNum=currentPreset.steppedParameters[abx2bsp[abx]];
		waveNum=currentPreset.steppedParameters[abx2wsp[abx]];
	}
	
	oriWaveNum=waveNum;
	oriBankNum=bankNum;

	if(!synth_refreshBankNames(sort))
		return;

reload:
	
	newBankNum=-1;	
	fn[0]=0;
	if(bankNum<0 || bankNum>=waveData.bankCount)
		strcpy(fn,SYNTH_DEFAULT_WAVE_BANK);
	else
		strcpy(fn,waveData.bankNames[bankNum]);
	for(i=0;i<waveData.bankCount;++i)
	{
		if(!strcasecmp(fn,waveData.bankNames[i]))
		{
			newBankNum=i;
			break;
		}
	}
	if(newBankNum<0) // in case bank not found, reload default wave and bank
	{
		rprintf(0,"Error: abx %d bank %s not found!\n",abx,fn);
		bankNum=-1;
		goto reload;
	}

	strcpy(currentPreset.oscBank[abx],waveData.bankNames[newBankNum]);

	synth_refreshCurWaveNames(abx,sort);

	newWaveNum=-1;
	fn[0]=0;
	if(waveNum<0 || waveNum>=waveData.curWaveCount)
		strcpy(fn,SYNTH_DEFAULT_WAVE_NAME);
	else
		strcpy(fn,waveData.curWaveNames[waveNum]);
	for(i=0;i<waveData.curWaveCount;++i)
	{
		if(!strcasecmp(fn,waveData.curWaveNames[i]))
		{
			newWaveNum=i;
			break;
		}
	}
	if(newWaveNum<0) // in case wave not found, reload default wave and bank
	{
		rprintf(0,"Error: abx %d waveform %s not found!\n",abx,fn);
		bankNum=-1;
		waveNum=-1;
		goto reload;
	}

	strcpy(currentPreset.oscWave[abx],waveData.curWaveNames[newWaveNum]);
	
	if (oriBankNum!=newBankNum || oriWaveNum!=newWaveNum)
	{
		rprintf(0,"reloadLegacyBankWaveIndexes abx %d from %d/%d to %d/%d\n",abx,oriBankNum,oriWaveNum,newBankNum,newWaveNum);
		currentPreset.steppedParameters[abx2bsp[abx]]=newBankNum;
		currentPreset.steppedParameters[abx2wsp[abx]]=newWaveNum;
	}
}

int synth_getBankCount(void)
{
	return waveData.bankCount;
}

int synth_getCurWaveCount(void)
{
	return waveData.curWaveCount;
}

int8_t synth_getBankName(int bankIndex, char * res)
{
	if(bankIndex<0 || bankIndex>=waveData.bankCount)
	{
		res[0]=0;
		return 0;
	}
	
	strcpy(res,waveData.bankNames[bankIndex]);
	return 1;
}

int8_t synth_getWaveName(int waveIndex, char * res)
{
	if(waveIndex<0 || waveIndex>=waveData.curWaveCount)
	{
		res[0]=0;
		return 0;
	}
	
	strcpy(res,waveData.curWaveNames[waveIndex]);
	return 1;
}

int8_t synth_refreshBankNames(int8_t sort)
{
	FRESULT res;
	
	if(waveData.bankSorted==sort) // already loaded and same state
		return 1;
	
	waveData.bankCount=0;

	if((res=f_opendir(&waveData.curDir,WAVEDATA_PATH)))
	{
		rprintf(0,"f_opendir res=%d\n",res);
		return 0;
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
	
	if(sort)
		qsort(waveData.bankNames,waveData.bankCount,sizeof(waveData.bankNames[0]),stringCompare);
	
	waveData.bankSorted=sort;
	
#ifdef DEBUG
	rprintf(0,"bankCount %d\n",waveData.bankCount);
#endif		
	
	return 1;
}

void synth_refreshCurWaveNames(int8_t abx, int8_t sort)
{
	FRESULT res;
	char fn[128];
	
	strcpy(fn,WAVEDATA_PATH "/");
	strcat(fn,currentPreset.oscBank[abx]);
	
	if(!strcmp(waveData.curWaveBank,fn) && waveData.curWaveSorted==sort && waveData.curWaveABX==abx) // already loaded and same state
		return;
	
	waveData.curWaveCount=0;

#ifdef DEBUG
	rprintf(0,"loading %s\n",fn);
#endif		
	
	if((res=f_opendir(&waveData.curDir,fn)))
	{
		rprintf(0,"f_opendir res=%d\n",res);
		return;
	}

	if((res=f_readdir(&waveData.curDir,&waveData.curFile)))
		rprintf(0,"f_readdir res=%d\n",res);

	while(!res)
	{
		if(strstr(waveData.curFile.fname,".WAV") || strstr(waveData.curFile.fname,".wav"))
		{
			strncpy(waveData.curWaveNames[waveData.curWaveCount],waveData.curFile.lfname,waveData.curFile.lfsize);
			++waveData.curWaveCount;
		}
		
		res=f_readdir(&waveData.curDir,&waveData.curFile);
		if (!waveData.curFile.fname[0] || waveData.curWaveCount>=MAX_BANK_WAVES)
			break;
	}

	if(sort)
		qsort(waveData.curWaveNames,waveData.curWaveCount,sizeof(waveData.curWaveNames[0]),stringCompare);

	waveData.curWaveSorted=sort;
	waveData.curWaveABX=abx;
	strcpy(waveData.curWaveBank,fn);

#ifdef DEBUG
	rprintf(0,"curWaveCount %d %d\n",abx,waveData.curWaveCount);
#endif		
}

void synth_refreshWaveforms(int8_t abx)
{
	int i;
	FIL f;
	FRESULT res;
	char fn[256];
	int16_t data[WTOSC_MAX_SAMPLES];
	int32_t d;
	int32_t smpSize = 0;
	
	strcpy(fn,WAVEDATA_PATH "/");
	strcat(fn,currentPreset.oscBank[abx]);
	strcat(fn,"/");
	strcat(fn,currentPreset.oscWave[abx]);

#ifdef DEBUG
	rprintf(0,"loading %s\n",fn);
#endif		
	
	if(!f_open(&f,fn,FA_READ))
	{

		if((res=f_lseek(&f,0x28)))
			rprintf(0,"f_lseek res=%d\n",res);

		if((res=f_read(&f,&smpSize,sizeof(smpSize),&i)))
			rprintf(0,"f_read res=%d\n",res);
		
		smpSize=MIN(smpSize,WTOSC_MAX_SAMPLES*2);
		
#ifdef DEBUG
		rprintf(0, "smpSize %d\n", smpSize);
#endif		

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
		value>>=2; // limit VCA output level to 4Vpp
		value=computeShape(value<<8,vcaLinearizationCurve,1);
		break;
	case cvNoiseVol:
		value=computeShape(value<<8,vcNoiseLinearizationCurve,1);
		break;
	case cvResonance:
		value>>=1;
		break;
	default:
		/* nothing */;
	}
	
	return value;
}

FORCEINLINE void synth_refreshCV(int8_t voice, cv_t cv, uint32_t v)
{
	static const uint8_t ampVoice2CV[SYNTH_VOICE_COUNT]={5,6,7,9,10,11};
	static const uint8_t cutoffVoice2CV[SYNTH_VOICE_COUNT]={2,3,4,13,14,15};
	uint16_t value,channel;
	
	v=__USAT(v,16);
	value=adjustCV(cv,v);

	switch(cv)
	{
	case cvAmp:
		channel=ampVoice2CV[voice];
		break;
	case cvNoiseVol:
		channel=8;
		break;
	case cvResonance:
		channel=12;
		break;
	case cvCutoff:
		channel=cutoffVoice2CV[voice];
		break;
	case cvAVol:
		channel=1;
		break;
	case cvBVol:
		channel=0;
		break;
	default:
		return;
	}
	
	dacspi_setCVValue(channel,value);
}

static void refreshCrossOver(uint16_t wmod, int16_t wmodEnvAmt)
{
	int i;
	int8_t v;
	uint32_t xovr;
	uint16_t *p0,*p2,*p3,xovr16;
	
	xovr=wmod;

	// paraphonic wavemod envelope mod
	if(currentPreset.continuousParameters[cpWModAEnv]!=HALF_RANGE)
	{
		v=assigner_getLatestAssigned(NULL);
		if(v>=0)
		{
			xovr+=scaleU16S16(synth.wmodEnvs[v].output,wmodEnvAmt)<<1;
			xovr=__USAT(xovr,16);
		}
	}
	
	if(xovr==synth.partState.oldCrossOver)
		return;

	p0=waveData.sampleData[0];
	p2=waveData.sampleData[2];
	p3=waveData.sampleData[3];
	xovr16=xovr;
	
	for(i=0;i<WTOSC_MAX_SAMPLES/4;++i)
	{
		*p3++=lerp16(*p0++,*p2++,xovr16);
		*p3++=lerp16(*p0++,*p2++,xovr16);
		*p3++=lerp16(*p0++,*p2++,xovr16);
		*p3++=lerp16(*p0++,*p2++,xovr16);
	}
	
	synth.partState.oldCrossOver=xovr;
}

static FORCEINLINE void refreshVoice(int8_t v,int32_t wmodAEnvAmt,int32_t wmodBEnvAmt,int32_t filEnvAmt,int32_t pitchAVal,int32_t pitchBVal,int32_t wmodAVal,int32_t wmodBVal,int32_t filterVal,int32_t ampVal,uint8_t wmodMask)
{
	int32_t vpa,vpb,vma,vmb,vf,vamp;

	// envs

	adsr_update(&synth.ampEnvs[v]);
	adsr_update(&synth.filEnvs[v]);
	adsr_update(&synth.wmodEnvs[v]);

	// filter

	vf=filterVal;
	vf+=scaleU16S16(synth.filEnvs[v].output,filEnvAmt);
	vf+=synth.filterNoteCV[v];
	synth_refreshCV(v,cvCutoff,vf);

	// oscs
	
	vma=wmodAVal;
	vma+=scaleU16S16(synth.wmodEnvs[v].output,wmodAEnvAmt);
	vma=__USAT(vma,16);

	vmb=wmodBVal;
	vmb+=scaleU16S16(synth.wmodEnvs[v].output,wmodBEnvAmt);
	vmb=__USAT(vmb,16);

	vpa=pitchAVal;
	if(wmodMask&4)
		vpa+=vma-HALF_RANGE;

	vpb=pitchBVal;
	if(wmodMask&64)
		vpb+=vmb-HALF_RANGE;

	// osc A

	vpa+=synth.oscANoteCV[v];
	vpa=__USAT(vpa,16);
	wtosc_setParameters(&synth.osc[v][0],vpa,(wmodMask&1)?vma:HALF_RANGE,(wmodMask&2)?vma:HALF_RANGE);

	// osc B

	vpb+=synth.oscBNoteCV[v];
	vpb=__USAT(vpb,16);
	wtosc_setParameters(&synth.osc[v][1],vpb,(wmodMask&16)?vmb:HALF_RANGE,(wmodMask&32)?vmb:HALF_RANGE);

	// amplifier
	
	vamp=scaleU16U16(synth.ampEnvs[v].output,ampVal);
	synth_refreshCV(v,cvAmp,vamp);
}

////////////////////////////////////////////////////////////////////////////////
// Synth main code
////////////////////////////////////////////////////////////////////////////////

void synth_init(void)
{
	int i;
	
	memset(&synth,0,sizeof(synth));
	memset(&waveData,0,sizeof(waveData));
	waveData.bankSorted=-1;
	waveData.curWaveSorted=-1;
	waveData.curWaveABX=-1;

	// init footswitch in

	GPIO_SetDir(3,1<<26,0);
	PINSEL_SetPinMode(3,26,PINSEL_BASICMODE_PLAINOUT);
	PINSEL_SetOpenDrainMode(3,26,DISABLE);
	
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
		adsr_init(&synth.wmodEnvs[i]);
	}

	lfo_init(&synth.lfo[0]);
	lfo_init(&synth.lfo[1]);

	// load settings from storage
	
	if(!settings_load())
		settings_loadDefault();

	synth_refreshBankNames(1);
	
	// upgrade presets from before storing bank/wave names
	preset_upgradeBankWaveStorage();
	
	// load last preset & do a full refresh
	
	if(!preset_loadCurrent(settings.presetNumber))
		preset_loadDefault(1);
	ui_setPresetModified(0);

	synth_refreshFullState();
	synth_refreshWaveforms(0);
	synth_refreshWaveforms(1);
	synth_refreshWaveforms(2);

	usb_setMode(settings.usbMIDI?umMIDI:umPowerOnly,NULL);
}


void synth_update(void)
{
#if 0
	putc_serial0('.');	
#else
	static int32_t frc=0,prevTick=0;
	++frc;
	if(currentTick-prevTick>TICKER_1S)
	{
		rprintf(0,"%d u/s\n",frc);
		frc=0;
		prevTick=currentTick;
	}
#endif

	scan_update();
	ui_update();
	
	refreshLfoSettings();
	
	synth.partState.syncResetsMask=currentPreset.steppedParameters[spOscSync]?UINT32_MAX:0;
}

////////////////////////////////////////////////////////////////////////////////
// Synth interrupts
////////////////////////////////////////////////////////////////////////////////

// @ 500Hz from system timer
void synth_timerInterrupt(void)
{
	int32_t resVal,resoFactor;
	
	resVal=currentPreset.continuousParameters[cpResonance];
	resVal+=scaleU16S16(currentPreset.continuousParameters[cpLFOResAmt],synth.lfo[0].output);
	resVal+=scaleU16S16(currentPreset.continuousParameters[cpLFO2ResAmt],synth.lfo[1].output);
	resVal=__USAT(resVal,16);

	// compensate pre filter mixer level for resonance

	resoFactor=(30*UINT16_MAX+110*(uint32_t)MAX(0,resVal-6000))/(100*256);

	// CV update
	
	auto uint16_t staticCV(cv_t cv)
	{
		return getStaticCV(cv)-INT16_MIN;
	}
		
	synth_refreshCV(-1,cvAVol,scaleU16U16(currentPreset.continuousParameters[cpAVol],staticCV(cvAVol))*resoFactor/256);
	synth_refreshCV(-1,cvBVol,scaleU16U16(currentPreset.continuousParameters[cpBVol],staticCV(cvBVol))*resoFactor/256);
	synth_refreshCV(-1,cvNoiseVol,scaleU16U16(currentPreset.continuousParameters[cpNoiseVol],staticCV(cvNoiseVol))*resoFactor/256);
	synth_refreshCV(-1,cvResonance,resVal);

	// midi

	midi_update();

	// crossover

	if(currentPreset.steppedParameters[spAWModType]==wmCrossOver ||
			currentPreset.steppedParameters[spBenderTarget]==modCrossOver ||
			currentPreset.steppedParameters[spPressureTarget]==modCrossOver)
	{
		int32_t wmod;
		
		wmod=synth.partState.wmodAVal;
		wmod+=getStaticCV(cvACrossOver);
		wmod=__USAT(wmod,16);
				
		refreshCrossOver(wmod,synth.partState.wmodAEnvAmt);
	}

	// bit inputs (footswitch / tape in)

	handleBitInputs();

	// assigner

	handleFinishedVoices();

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

////////////////////////////////////////////////////////////////////////////////
// Synth internal events
////////////////////////////////////////////////////////////////////////////////

// @ 5Khz from dacspi CV update
void synth_updateCVsEvent(void)
{
	int32_t val,pitchAVal,pitchBVal,wmodAVal,wmodBVal,filterVal,ampVal,wmodAEnvAmt,wmodBEnvAmt,filEnvAmt;

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

	ampVal=UINT16_MAX;

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

	wmodAEnvAmt=currentPreset.continuousParameters[cpWModAEnv];
	wmodBEnvAmt=currentPreset.continuousParameters[cpWModBEnv];
	wmodAEnvAmt+=INT16_MIN;
	wmodBEnvAmt+=INT16_MIN;

		// restrict range

	pitchAVal=__SSAT(pitchAVal,16);
	pitchBVal=__SSAT(pitchBVal,16);
	wmodAVal=__USAT(wmodAVal,16);
	wmodBVal=__USAT(wmodBVal,16);
	filterVal=__SSAT(filterVal,16);
	ampVal=__USAT(ampVal,16);

	// voices computations
		
	for(int8_t v=0;v<SYNTH_VOICE_COUNT;++v)
		refreshVoice(v,wmodAEnvAmt,wmodBEnvAmt,filEnvAmt,pitchAVal,pitchBVal,wmodAVal,wmodBVal,filterVal,ampVal,synth.partState.wmodMask);
	
	synth.partState.wmodAVal=wmodAVal;
	synth.partState.wmodAEnvAmt=wmodAEnvAmt;
}

#define PROC_UPDATE_DACS_VOICE(v) \
FORCEINLINE static void updateDACsVoice##v(int32_t start, int32_t end) \
{ \
	uint32_t syncResets=0; /* /!\ this won't work if count > 32 */ \
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

void synth_assignerEvent(uint8_t note, int8_t gate, int8_t voice, uint16_t velocity, uint8_t flags)
{
#ifdef DEBUG
	rprintf(0,"assign note %d gate %d voice %d velocity %d flags %d\n",note,gate,voice,velocity,flags);
#endif

	uint16_t velAmt;
	
	// mod delay
	refreshModulationDelay(0);

	// prepare CVs
	computeTunedCVs();

	// set gates (don't retrigger gate, unless we're arpeggiating)
	if(!(flags&ASSIGNER_EVENT_FLAG_LEGATO) || arp_getMode()!=amOff)
	{
		adsr_setGate(&synth.wmodEnvs[voice],gate);
		adsr_setGate(&synth.filEnvs[voice],gate);
		adsr_setGate(&synth.ampEnvs[voice],gate);
	}

	if(gate)
	{
		// handle velocity
		velAmt=currentPreset.continuousParameters[cpWModVelocity];
		adsr_setCVs(&synth.wmodEnvs[voice],0,0,0,0,(UINT16_MAX-velAmt)+scaleU16U16(velocity,velAmt),0x10);
		velAmt=currentPreset.continuousParameters[cpFilVelocity];
		adsr_setCVs(&synth.filEnvs[voice],0,0,0,0,(UINT16_MAX-velAmt)+scaleU16U16(velocity,velAmt),0x10);
		velAmt=currentPreset.continuousParameters[cpAmpVelocity];
		adsr_setCVs(&synth.ampEnvs[voice],0,0,0,0,(UINT16_MAX-velAmt)+scaleU16U16(velocity,velAmt),0x10);
	}
}

void synth_uartMIDIEvent(uint8_t data)
{
#ifdef DEBUG_
	rprintf(0,"uart midi %x\n",data);
#endif

	midi_newData(MIDI_PORT_UART, data);
}

void synth_usbMIDIEvent(uint8_t data)
{
#ifdef DEBUG_
	rprintf(0,"usb midi %x\n",data);
#endif

	midi_newData(MIDI_PORT_USB, data);
}

void synth_wheelEvent(int16_t bend, uint16_t modulation, uint8_t mask)
{
	static const int8_t br[]={4,7,12,0};
	static const int8_t mr[]={5,3,1,0};

#ifdef DEBUG_
	rprintf(0,"wheel bend %d mod %d mask %d\n",bend,modulation,mask);
#endif

	if(mask&1)
	{
		uint8_t range=br[currentPreset.steppedParameters[spBenderRange]];
		
		switch(currentPreset.steppedParameters[spBenderTarget])
		{
			case modPitch:
				bend=scaleU16S16(tuner_computeCVFromNote(0,range*2,0,cvAPitch)-tuner_computeCVFromNote(0,0,0,cvAPitch),bend);
				break;
			case modFilter:
				bend=scaleU16S16(tuner_computeCVFromNote(0,range*8,0,cvCutoff)-tuner_computeCVFromNote(0,0,0,cvCutoff),bend);
				break;
			case modVolume:
			case modCrossOver:
				bend=(bend/12)*range;
				break;
		}

		synth.partState.benderAmount=bend;

		if(currentPreset.steppedParameters[spBenderTarget]==modPitch ||
				currentPreset.steppedParameters[spBenderTarget]==modFilter)
			computeTunedCVs();
	}
	
	if(mask&2)
	{
		synth.partState.modwheelAmount=modulation>>mr[currentPreset.steppedParameters[spModwheelRange]];
		refreshLfoSettings();
	}
}

void synth_pressureEvent(uint16_t pressure)
{
	static const int8_t pr[]={5,3,1,0};

#ifdef DEBUG_
	rprintf(0,"pressure %d\n",pressure);
#endif
	
	synth.partState.pressureAmount=pressure>>pr[currentPreset.steppedParameters[spPressureRange]];

	switch(currentPreset.steppedParameters[spPressureTarget])
	{
		case modPitch:
			synth.partState.pressureAmount>>=2; // less modulation for pitch
			computeTunedCVs();
			break;
		case modFilter:
			computeTunedCVs();
			break;
		case modLFO1:
		case modLFO2:
			refreshLfoSettings();
			break;
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