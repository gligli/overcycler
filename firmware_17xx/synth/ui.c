///////////////////////////////////////////////////////////////////////////////
// LCD / keypad / potentiometers user interface
///////////////////////////////////////////////////////////////////////////////

#include "ui.h"
#include "storage.h"
#include "integer.h"
#include "arp.h"
#include "ffconf.h"
#include "lpc17xx_gpio.h"

#define DISPLAY_ACK 6

#define ADC_QUANTUM (1<<(16-12)) // 12bit resolution

#define ADCROW_PORT 3
#define ADCROW_PIN 25

#define UI_POT_COUNT 10
#define UI_POT_SAMPLES 80

#define UI_PRESET_SLOTS 20
#define UI_PRESET_BANKS 20

#define POT_CHANGE_DETECT_THRESHOLD (ADC_QUANTUM*32)
#define POT_TIMEOUT_THRESHOLD (ADC_QUANTUM*24)
#define POT_TIMEOUT (TICKER_1S*5/8)
#define ACTIVE_SOURCE_TIMEOUT (TICKER_1S*3/4)
#define SLOW_UPDATE_TIMEOUT (TICKER_1S/10)

enum uiDigitInput_e{
	diSynth,diLoadDecadeDigit,diStoreDecadeDigit,diLoadUnitDigit,diStoreUnitDigit
};

enum uiParamType_e
{
	ptNone=0,ptCont,ptStep,ptCust
};

enum uiKeypadButton_e
{
	kb1=0,kb2,kb3,kb4,kb5,kb6,kb7,kb8,kb9,kb0,
	kbA,kbB,kbC,kbD,
	kbAsterisk,kbSharp
};

enum uiPage_e
{
	upNone=-1,upOscs=0,upFil=1,upAmp=2,upMod=3,upSeqArp=4,upMisc=5,upTuner=6
};

struct uiParam_s
{
	enum uiParamType_e type;
	int8_t number;
	const char * shortName; // 4 chars + zero termination
	const char * longName;
	const char * values[8]; // 4 chars + zero termination
};

const struct uiParam_s uiParameters[7][2][10] = // [pages][0=pots/1=keys][pot/key num]
{
	/* Oscillators page (A) */
	{
		{
			/* 1st row of pots */
			{.type=ptStep,.number=spABank,.shortName="ABnk",.longName="Osc A Bank"},
			{.type=ptStep,.number=spAWave,.shortName="AWav",.longName="Osc A Waveform"},
			{.type=ptCont,.number=cpABaseWMod,.shortName="AWmo",.longName="Osc A WaveMod"},
			{.type=ptCont,.number=cpBFreq,.shortName="Freq",.longName="Osc A/B Frequency"},
			{.type=ptCont,.number=cpAVol,.shortName="AVol",.longName="Osc A Volume"},
			/* 2nd row of pots */
			{.type=ptStep,.number=spBBank,.shortName="BBnk",.longName="Osc B Bank"},
			{.type=ptStep,.number=spBWave,.shortName="BWav",.longName="Osc B Waveform"},
			{.type=ptCont,.number=cpBBaseWMod,.shortName="BWmo",.longName="Osc B WaveMod"},
			{.type=ptCont,.number=cpBFineFreq,.shortName="BDet",.longName="Osc B Detune"},
			{.type=ptCont,.number=cpBVol,.shortName="BVol",.longName="Osc B Volume"},
		},
		{
			/*1*/ {.type=ptStep,.number=spChromaticPitch,.shortName="FrqM",.longName="Frequency mode",.values={"Free","Semi","Oct "}},
			/*2*/ {.type=ptStep,.number=spAWModType,.shortName="AWmT",.longName="Osc A WaveMod type",.values={"Off ","Alia","Wdth","Freq"}},
			/*3*/ {.type=ptStep,.number=spBWModType,.shortName="BWmT",.longName="Osc B WaveMod type",.values={"Off ","Alia","Wdth","Freq"}},
			/*4*/ {.type=ptNone},
			/*5*/ {.type=ptNone},
			/*6*/ {.type=ptNone},
			/*7*/ {.type=ptNone},
			/*8*/ {.type=ptNone},
			/*9*/ {.type=ptNone},
			/*0*/ {.type=ptCust,.number=0,.shortName="Disp",.longName="Display mode",.values={"Pots","Btns"}},
		},
	},
	/* Filter page (B) */
	{
		{
			/* 1st row of pots */
			{.type=ptCont,.number=cpCutoff,.shortName="FCut",.longName="Filter Cutoff freqency"},
			{.type=ptCont,.number=cpResonance,.shortName="FRes",.longName="Filter Resonance"},
			{.type=ptCont,.number=cpFilKbdAmt,.shortName="FKbd",.longName="Filter Keyboard tracking"},
			{.type=ptCont,.number=cpFilEnvAmt,.shortName="FEnv",.longName="Filter Envelope amount"},
			{.type=ptCont,.number=cpWModFilEnv,.shortName="WEnv",.longName="WaveMod Envelope amount"},
			/* 2nd row of pots */
			{.type=ptCont,.number=cpFilAtt,.shortName="FAtk",.longName="Filter Attack"},
			{.type=ptCont,.number=cpFilDec,.shortName="FDec",.longName="Filter Decay"},
			{.type=ptCont,.number=cpFilSus,.shortName="FSus",.longName="Filter Sustain"},
			{.type=ptCont,.number=cpFilRel,.shortName="FRel",.longName="Filter Release"},
			{.type=ptCont,.number=cpFilVelocity,.shortName="FVel",.longName="Filter Velocity"},
		},
		{
			/*1*/ {.type=ptStep,.number=spFilEnvSlow,.shortName="FEnT",.longName="Filter Envelope Type",.values={"Fast","Slow"}},
			/*2*/ {.type=ptStep,.number=spAWModEnvEn,.shortName="AWmE",.longName="A WaveMod envelope",.values={"Off ","On  "}},
			/*3*/ {.type=ptStep,.number=spBWModEnvEn,.shortName="BWmE",.longName="B WaveMod envelope",.values={"Off ","On  "}},
			/*4*/ {.type=ptNone},
			/*5*/ {.type=ptNone},
			/*6*/ {.type=ptNone},
			/*7*/ {.type=ptNone},
			/*8*/ {.type=ptNone},
			/*9*/ {.type=ptNone},
			/*0*/ {.type=ptCust,.number=0,.shortName="Disp",.longName="Display mode",.values={"Pots","Btns"}},
		},
	},
	/* Amplifier page (C) */
	{
		{
			/* 1st row of pots */
			{.type=ptCont,.number=cpGlide,.shortName="Glid",.longName="Glide amount"},
			{.type=ptCont,.number=cpUnisonDetune,.shortName="MDet",.longName="Master unison Detune"},
			{.type=ptCont,.number=cpMasterTune,.shortName="MTun",.longName="Master Tune"},
			{.type=ptNone},
			{.type=ptCont,.number=cpNoiseVol,.shortName="NVol",.longName="Noise volume"},
			/* 2nd row of pots */
			{.type=ptCont,.number=cpAmpAtt,.shortName="AAtk",.longName="Amplifier Attack"},
			{.type=ptCont,.number=cpAmpDec,.shortName="ADec",.longName="Amplifier Decay"},
			{.type=ptCont,.number=cpAmpSus,.shortName="ASus",.longName="Amplifier Sustain"},
			{.type=ptCont,.number=cpAmpRel,.shortName="ARel",.longName="Amplifier Release"},
			{.type=ptCont,.number=cpAmpVelocity,.shortName="AVel",.longName="Amplifier Velocity"},
		},
		{
			/*1*/ {.type=ptStep,.number=spAmpEnvSlow,.shortName="AEnT",.longName="Amplifier Envelope Type",.values={"Fast","Slow"}},
			/*2*/ {.type=ptStep,.number=spUnison,.shortName="Unis",.longName="Unison",.values={"Off ","On  "}},
			/*3*/ {.type=ptStep,.number=spAssignerPriority,.shortName="Prio",.longName="Assigner Priority",.values={"Last","Low ","High"}},
			/*4*/ {.type=ptNone},
			/*5*/ {.type=ptNone},
			/*6*/ {.type=ptNone},
			/*7*/ {.type=ptNone},
			/*8*/ {.type=ptNone},
			/*9*/ {.type=ptNone},
			/*0*/ {.type=ptCust,.number=0,.shortName="Disp",.longName="Display mode",.values={"Pots","Btns"}},
		},
	},
	/* Modulation page (D) */
	{
		{
			/* 1st row of pots */
			{.type=ptCont,.number=cpLFOFreq,.shortName="LFrq",.longName="LFO Frequency"},
			{.type=ptCont,.number=cpLFOAmt,.shortName="LAmt",.longName="LFO Amount (base)"},
			{.type=ptStep,.number=spLFOShape,.shortName="LWav",.longName="LFO Waveform",.values={"Sqr ","Tri ","Rand","Sine","Nois","Saw "}},
			{.type=ptCont,.number=cpLFOPitchAmt,.shortName="LPit",.longName="Pitch LFO Amount"},
			{.type=ptCont,.number=cpLFOWModAmt,.shortName="LWmo",.longName="WaveMod LFO Amount"},
			/* 2nd row of pots */
			{.type=ptCont,.number=cpVibFreq,.shortName="VFrq",.longName="Vibrato Frequency"},
			{.type=ptCont,.number=cpVibAmt,.shortName="VAmt",.longName="Vibrato Amount (base)"},
			{.type=ptCont,.number=cpModDelay,.shortName="MDly",.longName="Modulation Delay"},
			{.type=ptCont,.number=cpLFOFilAmt,.shortName="LFil",.longName="Filter LFO Amount"},
			{.type=ptCont,.number=cpLFOAmpAmt,.shortName="LAmp",.longName="Amplifier LFO Amount"},
		},
		{
			/*1*/ {.type=ptStep,.number=spLFOShift,.shortName="LRng",.longName="LFO Range",.values={"Slow","Fast"}},
			/*2*/ {.type=ptStep,.number=spModwheelRange,.shortName="MRng",.longName="Modwheel range",.values={"Min ","Low ","High","Full"}},
			/*3*/ {.type=ptStep,.number=spBenderRange,.shortName="BRng",.longName="Bender range",.values={"3rd ","5th ","Oct "}},
			/*4*/ {.type=ptStep,.number=spLFOTargets,.shortName="LTgt",.longName="LFO Osc target",.values={"Off ","OscA","OscB","Both"}},
			/*5*/ {.type=ptStep,.number=spModwheelTarget,.shortName="MTgt",.longName="Modwheel target",.values={"LFO ","Vib "}},
			/*6*/ {.type=ptStep,.number=spBenderTarget,.shortName="BTgt",.longName="Bender target",.values={"Off ","Pit ","Fil ","Amp "}},
			/*7*/ {.type=ptNone},
			/*8*/ {.type=ptNone},
			/*9*/ {.type=ptNone},
			/*0*/ {.type=ptCust,.number=0,.shortName="Disp",.longName="Display mode",.values={"Pots","Btns"}},
		},
	},
	/* Sequencer/arpeggiator page (#) */
	{
		{
			/* 1st row of pots */
			{.type=ptCont,.number=cpSeqArpClock,.shortName="Clk ",.longName="Seq/Arp clock"},
		},
		{
			/*1*/ {.type=ptCust,.number=1,.shortName="ArSq",.longName="Seq/Arp choice",.values={"Arp ","Seq "}},
			/*2*/ {.type=ptCust,.number=2,.shortName="AMod",.longName="Arp mode",.values={"Off ","UpDn","Rand","Asgn"}},
			/*3*/ {.type=ptCust,.number=3,.shortName="AHld",.longName="Arp hold",.values={"Off ","On "}},
			/*4*/ {.type=ptNone},
			/*5*/ {.type=ptNone},
			/*6*/ {.type=ptNone},
			/*7*/ {.type=ptNone},
			/*8*/ {.type=ptNone},
			/*9*/ {.type=ptNone},
			/*0*/ {.type=ptCust,.number=0,.shortName="Disp",.longName="Display mode",.values={"Pots","Btns"}},
		},
	},
	/* Miscellaneous page (*) */
	{
		{
			/* 1st row of pots */
			{.type=ptCust,.number=4,.shortName="Slot",.longName="Preset slot"},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptCust,.number=12,.shortName="Sync",.longName="Sync mode",.values={"Int ","MIDI"}},
			/* 2nd row of pots */
			{.type=ptCust,.number=11,.shortName="Bank",.longName="Preset bank"},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptCust,.number=7,.shortName="MidC",.longName="Midi channel"},
		},
		{
			/*1*/ {.type=ptCust,.number=5,.shortName="Load",.longName="Load preset"},
			/*2*/ {.type=ptNone},
			/*3*/ {.type=ptCust,.number=6,.shortName="Save",.longName="Save preset"},
			/*4*/ {.type=ptNone},
			/*5*/ {.type=ptCust,.number=8,.shortName="Tune",.longName="Tune hardware"},
			/*6*/ {.type=ptNone},
			/*7*/ {.type=ptNone},
			/*8*/ {.type=ptNone},
			/*9*/ {.type=ptNone},
			/*0*/ {.type=ptCust,.number=0,.shortName="Disp",.longName="Display mode",.values={"Pots","Btns"}},
		},
	},
	/* Tuner page */
	{
		{
			/* 1st row of pots */
			{.type=ptCust,.number=9,.shortName="Oct1",.longName="Tuner octave 1"},
			{.type=ptCust,.number=9,.shortName="Oct2",.longName="Tuner octave 2"},
			{.type=ptCust,.number=9,.shortName="Oct3",.longName="Tuner octave 3"},
			{.type=ptCust,.number=9,.shortName="Oct4",.longName="Tuner octave 4"},
			{.type=ptCust,.number=9,.shortName="Oct5",.longName="Tuner octave 5"},
			/* 2nd row of pots */
			{.type=ptCust,.number=9,.shortName="Oct6",.longName="Tuner octave 6"},
			{.type=ptCust,.number=9,.shortName="Oct7",.longName="Tuner octave 7"},
			{.type=ptCust,.number=9,.shortName="Oct8",.longName="Tuner octave 8"},
			{.type=ptNone},
			{.type=ptNone},
		},
		{
			/*1*/ {.type=ptCust,.number=10,.shortName="Vce1",.longName="Tuner voice 1"},
			/*2*/ {.type=ptCust,.number=10,.shortName="Vce2",.longName="Tuner voice 2"},
			/*3*/ {.type=ptCust,.number=10,.shortName="Vce3",.longName="Tuner voice 3"},
			/*4*/ {.type=ptCust,.number=10,.shortName="Vce4",.longName="Tuner voice 4"},
			/*5*/ {.type=ptCust,.number=10,.shortName="Vce5",.longName="Tuner voice 5"},
			/*6*/ {.type=ptCust,.number=10,.shortName="Vce6",.longName="Tuner voice 6"},
			/*7*/ {.type=ptNone},
			/*8*/ {.type=ptNone},
			/*9*/ {.type=ptNone},
			/*0*/ {.type=ptCust,.number=0,.shortName="Disp",.longName="Display mode",.values={"Pots","Btns"}},
		},
	},
};

static const uint8_t potToCh[UI_POT_COUNT]=
{
	1<<0,1<<1,1<<2,1<<3,1<<5,
	1<<0,1<<1,1<<2,1<<3,1<<5,
};

static const uint8_t keypadButtonCode[16]=
{
	0xee,0xde,0xbe,0xed,0xdd,0xbd,0xeb,0xdb,0xbb,0xd7,
	0x7e,0x7d,0x7b,0x77,
	0xb7,0xe7
};

static struct
{
	uint16_t pots[UI_POT_COUNT][UI_POT_SAMPLES];
	int16_t curSample;
	int8_t curPot;
	uint16_t value[UI_POT_COUNT];
	uint32_t lockTimeout[UI_POT_COUNT];
	
	int8_t presetModified;
	enum uiPage_e activePage;
	int8_t displayMode;
	
	int8_t activeSource;
	uint32_t activeSourceTimeout;
	
	uint32_t slowUpdateTimeout;
	int8_t slowUpdateTimeoutNumber;
	
	int8_t pendingScreenClear;

	int8_t presetBank;
	int8_t presetSlot;
	
	int8_t tunerActiveVoice;
} ui;

__attribute__ ((used)) void ADC_IRQHandler(void)
{
	
	if(LPC_ADC->ADGDR & ADC_DR_DONE_FLAG)
		ui.pots[ui.curPot][ui.curSample]=(uint16_t)LPC_ADC->ADGDR;

	++ui.curPot;
	
	if(ui.curPot==UI_POT_COUNT/2)
	{
		GPIO_SetValue(ADCROW_PORT,1<<ADCROW_PIN);
	}
	else if(ui.curPot>=UI_POT_COUNT)
	{
		ui.curPot=0;
		GPIO_ClearValue(ADCROW_PORT,1<<ADCROW_PIN);
		
		// scanned all the pots, move to next sample
		
		++ui.curSample;
		if(ui.curSample>=UI_POT_SAMPLES)
			ui.curSample=0;
	}
	
	
	LPC_ADC->ADCR=(LPC_ADC->ADCR&~0xff) | potToCh[ui.curPot] | ADC_CR_START_NOW;
}

static uint16_t getPotValue(int8_t pot)
{
	return ui.value[pot]&0xffc0;
}

int sendChar(int ch)
{
	while(UART_CheckBusy((LPC_UART_TypeDef*)LPC_UART1)==SET);
	UART_SendData((LPC_UART_TypeDef*)LPC_UART1,ch);
	return -1;
}

void sendString(const char * s)
{
	while(*s)
		sendChar(*s++);
}

uint8_t receiveChar(void)
{
	return UART_ReceiveData((LPC_UART_TypeDef*)LPC_UART1);
}

static void sendCommand(const char * command, int8_t waitAck, char * response)
{
	uint8_t c;

	// empty receive fifo
	while(receiveChar());
		
	// send command
	sendChar(0x1b);
	while(*command)
		sendChar(*command++);

	if(waitAck)
	{
		do
		{
			c=receiveChar();
			if(response && c && c!=DISPLAY_ACK)
				*response++=c;
		}
		while(c!=DISPLAY_ACK);
	}

	if(response)
		*response=0;
}

static const char * getName(int8_t source, int8_t longName) // source: keypad (kb0..kbSharp) / (-1..-10)
{
	const struct uiParam_s * prm;
	int8_t potnum;

	potnum=-source-1;
	prm=&uiParameters[ui.activePage][source<0?0:1][source<0?potnum:source];

	if(!longName && prm->shortName)
		return prm->shortName;
	if(longName && prm->longName)
		return prm->longName;
	else
		return "    ";
}

static char * getDisplayFulltext(int8_t source) // source: keypad (kb0..kbSharp) / (-1..-10)
{
	static char dv[_MAX_LFN];
	const struct uiParam_s * prm;
	int8_t potnum;
	
	dv[0]=0;	
	potnum=-source-1;
	prm=&uiParameters[ui.activePage][source<0?0:1][source<0?potnum:source];
	
	if(prm->type==ptStep)
	{
		switch(prm->number)
		{
			case spABank:
				appendBankName(0,dv);
				break;
			case spBBank:
				appendBankName(1,dv);
				break;
			case spAWave:
				appendWaveName(0,dv);
				break;
			case spBWave:
				appendWaveName(1,dv);
				break;
			default:
				return NULL;
		}

		// always 20chars
		for(int i=strlen(dv);i<20;++i) dv[i]=' ';
		dv[20]=0;
		
		return dv;
	}
	
	return NULL;
}

static char * getDisplayValue(int8_t source, uint16_t * contValue) // source: keypad (kb0..kbSharp) / (-1..-10)
{
	static char dv[10]={0};
	const struct uiParam_s * prm;
	int8_t potnum;
	int32_t valCount;
	uint32_t v;

	sprintf(dv,"    ");
	if(contValue)
		*contValue=0;
	
	potnum=-source-1;
	prm=&uiParameters[ui.activePage][source<0?0:1][source<0?potnum:source];
	
	switch(prm->type)
	{
	case ptCont:
		v=currentPreset.continuousParameters[prm->number];
		if(contValue)
			*contValue=v;
		sprintf(dv,"%4d",(v*1000)>>16);
		break;
	case ptStep:
	case ptCust:
		valCount=0;
		while(valCount<8 && prm->values[valCount]!=NULL)
			++valCount;

		
		if(prm->type==ptStep)
		{
			v=currentPreset.steppedParameters[prm->number];
		}
		else
		{
			switch(prm->number)
			{
			case 0:
				v=ui.displayMode;
				break;
			case 1:
				v=0;
				break;
			case 2:
				v=arp_getMode();
				break;
			case 3:
				v=arp_getHold();
				break;
			case 4:
				v=ui.presetSlot;
				break;
			case 5:
			case 6:
				v=ui.presetSlot+ui.presetBank*UI_PRESET_SLOTS;
				break;
			case 7:
				v=settings.midiReceiveChannel;
				break;
			case 8:
				v=0;
				break;
			case 9:
				v=settings.tunes[potnum][ui.tunerActiveVoice]>>3;
				break;
			case 10:
				v=ui.tunerActiveVoice;
				break;
			case 11:
				v=ui.presetBank;
				break;
			case 12:
				v=settings.syncMode;
				break;
			}
		}
		
		if(v<valCount)
			strcpy(dv,prm->values[v]);
		else
			sprintf(dv,"%4d",v+1);
		break;
	default:
		;
	}
	
	return dv;
}

static void handleUserInput(int8_t source) // source: keypad (kb0..kbSharp) / (-1..-10)
{
	int32_t data,valCount;
	int8_t potnum,change;
	const struct uiParam_s * prm;

//	rprintf(0,"handleUserInput %d\n",source);
	
	if(source>=kbA)
	{
		ui.activePage=source-kbA;
		rprintf(0,"page %d\n",ui.activePage);

		// cancel ongoing changes
		ui.activeSourceTimeout=0;
		memset(&ui.lockTimeout[0],0,UI_POT_COUNT*sizeof(uint32_t));

		ui.pendingScreenClear=1;
		
		return;
	}
	
	potnum=-source-1;
	prm=&uiParameters[ui.activePage][source<0?0:1][source<0?potnum:source];

	if(ui.activePage==upNone || prm->type==ptNone)
		return;

	// fullscreen display
	if (ui.activeSource!=source)
		ui.pendingScreenClear=1;
	ui.activeSource=source;
	ui.activeSourceTimeout=currentTick+ACTIVE_SOURCE_TIMEOUT;
	
	switch(prm->type)
	{
	case ptCont:
		change=currentPreset.continuousParameters[prm->number]!=getPotValue(potnum);
		currentPreset.continuousParameters[prm->number]=getPotValue(potnum);
		break;
	case ptStep:
		valCount=0;
		while(valCount<8 && prm->values[valCount]!=NULL)
			++valCount;
		
		if(!valCount)
		{
			switch(prm->number)
			{
				case spABank:
				case spBBank:
					valCount=getBankCount();
					break;
				case spAWave:
					valCount=getWaveCount(0);
					break;
				case spBWave:
					valCount=getWaveCount(1);
					break;
				default:
					valCount=1<<steppedParametersBits[prm->number];
			}
		}

		if(source<0)
			data=(getPotValue(potnum)*valCount)>>16;
		else
			data=(currentPreset.steppedParameters[prm->number]+1)%valCount;

		change=currentPreset.steppedParameters[prm->number]!=data;
		currentPreset.steppedParameters[prm->number]=data;
		
		//	special cases
		if(change)
		{
			if(prm->number==spUnison)
			{
				// unison latch
				if(data)
					assigner_latchPattern();
				else
					assigner_setPoly();
				assigner_getPattern( currentPreset.voicePattern,NULL);
			}
			else if(prm->number==spABank || prm->number==spBBank || prm->number==spAWave || prm->number==spBWave)
			{
				// waveform changes
				ui.slowUpdateTimeout=currentTick+SLOW_UPDATE_TIMEOUT;
				ui.slowUpdateTimeoutNumber=prm->number;
			}
		}
		break;
	case ptCust:
		change=1;
		switch(prm->number)
		{
			case 0:
				ui.displayMode=1-ui.displayMode;
				break;
			case 2:
				arp_setMode((arp_getMode()+1)%4,arp_getHold());
				break;
			case 3:
				arp_setMode(arp_getMode(),!arp_getHold());
				break;
			case 4:
				ui.presetSlot=(getPotValue(potnum)*UI_PRESET_SLOTS)>>16;
				break;
			case 6:
				preset_saveCurrent(ui.presetSlot+ui.presetBank*UI_PRESET_SLOTS);
				/* fall through */
			case 5:
				if(preset_loadCurrent(ui.presetSlot+ui.presetBank*UI_PRESET_SLOTS))
				{
					settings.presetNumber=ui.presetSlot+ui.presetBank*UI_PRESET_SLOTS;
					settings_save();                
				}
				else
				{
					preset_loadDefault(1);
				}

				refreshWaveNames(0);
				refreshWaveNames(1);
				refreshWaveforms(0);
				refreshWaveforms(1);
				break;
			case 7:
				settings.midiReceiveChannel=((getPotValue(potnum)*17)>>16)-1;
				settings_save();
				break;
			case 8:
				ui.activePage=upTuner;
				ui.pendingScreenClear=1;
				break;
			case 9:
				settings.tunes[potnum][ui.tunerActiveVoice]=TUNER_FIL_INIT_OFFSET+potnum*TUNER_FIL_INIT_SCALE-4096+(getPotValue(potnum)>>3);
				settings_save();
				break;
			case 10:
				ui.tunerActiveVoice=source-kb1;
				break;
			case 11:
				ui.presetBank=(getPotValue(potnum)*UI_PRESET_BANKS)>>16;
				break;
			case 12:
				settings.syncMode=(getPotValue(potnum)*2)>>16;
				settings_save();
				break;
		}
		break;
	default:
		/*nothing*/;
	}

	if(change)
	{
		ui.presetModified=1;
		refreshFullState();
	}
}

static void readKeypad(void)
{
	int key,i;
	char response[10],*rb;

	// get all awaiting keys
	
	for(;;)
	{
		sendCommand("[k",1,response);

		key=0;
		rb=response;
		while(*rb)
		{
			if(*rb>='0' && *rb<='9')
				key=key*10+(*rb-'0');
			++rb;
		}
		
		if(!key) break;
		
		for(i=0;i<16;++i)
			if(key==keypadButtonCode[i])
			{
				key=i;
				break;
			}
		
		handleUserInput(key);
	}
}

static void updatePotValue(int8_t pot)
{
	uint16_t tmp[UI_POT_SAMPLES];
	uint32_t new,delta;
	
	// sort values
	
	memcpy(&tmp[0],&ui.pots[pot][0],UI_POT_SAMPLES*sizeof(uint16_t));
	qsort(tmp,UI_POT_SAMPLES,sizeof(uint16_t),uint16Compare);
	
	// median
	
	new=tmp[UI_POT_SAMPLES/2];
	
	// ignore small changes
	
	delta=abs(new-ui.value[pot]);

	if(delta>POT_CHANGE_DETECT_THRESHOLD || currentTick<ui.lockTimeout[pot])
	{
		if(delta>POT_TIMEOUT_THRESHOLD)
			ui.lockTimeout[pot]=currentTick+POT_TIMEOUT;
		
		ui.value[pot]=new;

		handleUserInput(-pot-1);
	}
}

void ui_setPresetModified(int8_t modified)
{
	ui.presetModified=modified;
}

int8_t ui_isPresetModified(void)
{
	return ui.presetModified;
}


void ui_init(void)
{
	memset(&ui,0,sizeof(ui));
	
	// init screen serial
	
	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_UART1,CLKPWR_PCLKSEL_CCLK_DIV_1);	
	PINSEL_SetPinFunc(0,15,1);
	PINSEL_SetPinFunc(0,16,1);

	UART_CFG_Type uart;
	UART_ConfigStructInit(&uart);
	uart.Baud_rate=115200;
	UART_Init((LPC_UART_TypeDef*)LPC_UART1,&uart);
	
	UART_FIFO_CFG_Type fifo;
	UART_FIFOConfigStructInit(&fifo);
	UART_FIFOConfig((LPC_UART_TypeDef*)LPC_UART1,&fifo);
	
	UART_TxCmd((LPC_UART_TypeDef*)LPC_UART1,ENABLE);

	rprintf_devopen(1,sendChar);
	
	// init screen (autobaud)
	sendChar(0x0d);
	delay_ms(500);

	// welcome message
	rprintf(1,"Gli's OverCycler");
	rprintf(1,"    " __DATE__);
	sendCommand("[1b",0,NULL); // default at 115200bps
	delay_ms(500);
	sendCommand("[?27S",0,NULL); // save it as start screen
	delay_ms(500);
	
	// configure screen
	sendCommand("[6k",0,NULL); // ack = DISPLAY_ACK
	sendCommand("[20c",1,NULL); // screen size (20 cols)
	sendCommand("[4L",1,NULL); // screen size (4 rows)
	sendCommand("[?25I",1,NULL); // hide cursor
	sendCommand("[1x",1,NULL); // disable scrolling
	sendCommand("[2J",1,NULL); // clear screen
	sendCommand("[50r",1,NULL); // keypad debounce
	
	// init row select
	
	PINSEL_SetPinFunc(ADCROW_PORT,ADCROW_PIN,0);
	GPIO_SetDir(ADCROW_PORT,1<<ADCROW_PIN,1);
	
	// init adc pins

	PINSEL_SetPinFunc(0,23,1);
	PINSEL_SetPinFunc(0,24,1);
	PINSEL_SetPinFunc(0,25,1);
	PINSEL_SetPinFunc(0,26,1);
	PINSEL_SetPinFunc(1,31,3);

		// unused pins 
	PINSEL_SetPinFunc(1,30,0);
	GPIO_SetDir(1,1<<30,1);
	GPIO_ClearValue(1,1<<30);
	
	// init adc
	
	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_ADC,CLKPWR_PCLKSEL_CCLK_DIV_1);
	CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCAD,ENABLE);

	LPC_ADC->ADCR=ADC_CR_CLKDIV(9);
	LPC_ADC->ADINTEN=ADC_INTEN_GLOBAL;
	
	NVIC_SetPriority(ADC_IRQn,7);
	NVIC_EnableIRQ(ADC_IRQn);

	// start
	
	LPC_ADC->ADCR|= ADC_CR_PDN | ADC_CR_START_NOW;
	
	ui.activePage=upNone;
	ui.activeSource=INT8_MAX;
	ui.presetSlot=-1;
	ui.presetBank=-1;
}

void ui_update(void)
{
	int i;
	uint16_t v;
	static uint8_t frc=0;
	char * s;
	int8_t fsDisp;
	
	++frc;
	
	// init preset #
	
	if(ui.presetSlot==-1)
	{
		ui.presetSlot=settings.presetNumber%UI_PRESET_SLOTS;
		ui.presetBank=settings.presetNumber/UI_PRESET_SLOTS;
	}
	
	// update pot values

	for(i=0;i<UI_POT_COUNT;++i)
		updatePotValue(i);

	// slow updates (if needed)
	
	if(currentTick>ui.slowUpdateTimeout)
	{
		switch(ui.slowUpdateTimeoutNumber)
		{
			case spABank:
				refreshWaveNames(0);
				break;
			case spBBank:
				refreshWaveNames(1);
				break;
			case spAWave:
				refreshWaveforms(0);
				break;
			case spBWave:
				refreshWaveforms(1);
				break;
		}
		
		ui.slowUpdateTimeout=UINT32_MAX;
	}

	// display
	
	fsDisp=ui.activeSourceTimeout>currentTick;
	if(!fsDisp && ui.activeSourceTimeout)
	{
		ui.activeSourceTimeout=0;
		ui.pendingScreenClear=1;
	}
	
	if(ui.pendingScreenClear)
		sendCommand("[2J",1,NULL); // clear screen

	sendCommand("[H",1,NULL); // home pos

	if(ui.activePage==upNone)
	{
		sendString("GliGli's OverCycler ");
		sendString("A: Oscs   B: Filter ");
		sendString("C: Ampli  D: LFO/Mod");
		sendString("*: Misc   #: Seq/Arp");
	}
	else if(fsDisp) // fullscreen display
	{
		if(ui.pendingScreenClear)
			sendString(getName(ui.activeSource,1));

		sendCommand("[3;9H",1,NULL);
		sendString(getDisplayValue(ui.activeSource,&v));
		
		sendString("\r");
		s=getDisplayFulltext(ui.activeSource);
		if(s)
		{
			sendString(s);
		}
		else
		{
			// bargraph			
			for(i=0;i<20;++i)
				if(i<(((int32_t)v+UINT16_MAX/40)*20>>16))
					sendChar(255);
				else
					sendChar(' ');
		}
	}
	else if(ui.displayMode) // buttons
	{
		ui.activeSource=INT8_MAX;

		if(ui.pendingScreenClear)
			for(i=0;i<3;++i)
			{
				sendString(getName(i,0));
				sendChar(' ');
				sendChar(' ');
			}

		sendChar('\r');
 
		for(i=0;i<6;++i)
		{
			if(i==3) sendChar('\r');
			sendString(getDisplayValue(i,NULL));
			sendChar(' ');
			sendChar(' ');
		}
	
		if(ui.pendingScreenClear)
		{
			sendChar('\r');

			for(i=3;i<6;++i)
			{
				sendString(getName(i,0));
				sendChar(' ');
				sendChar(' ');
			}
		}
	}
	else // pots
	{
		ui.activeSource=INT8_MAX;

		if(ui.pendingScreenClear)
			for(i=0;i<UI_POT_COUNT/2;++i)
				sendString(getName(-i-1,0));
			
		sendCommand("[H",1,NULL);
		sendString("\r");
		for(i=0;i<UI_POT_COUNT;++i)
			sendString(getDisplayValue(-i-1,NULL));

		if(ui.pendingScreenClear)
			for(i=UI_POT_COUNT/2;i<UI_POT_COUNT;++i)
				sendString(getName(-i-1,0));
	}

	ui.pendingScreenClear=0;

	// read keypad
			
	readKeypad();
}
