///////////////////////////////////////////////////////////////////////////////
// LCD / keypad / potentiometers user interface
///////////////////////////////////////////////////////////////////////////////

#include "ui.h"
#include "storage.h"
#include "integer.h"
#include "arp.h"
#include "ffconf.h"
#include "dacspi.h"
#include "hd44780.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"

#define DEBOUNCE_THRESHOLD 5

#define PRESET_SLOTS 20
#define PRESET_BANKS 20

#define ADC_QUANTUM 64 // 10 bit

#define POT_COUNT 10
#define POT_SAMPLES 15
#define POT_THRESHOLD (ADC_QUANTUM*16)
#define POT_TIMEOUT_THRESHOLD (ADC_QUANTUM*16)
#define POT_TIMEOUT (TICKER_1S)

#define ACTIVE_SOURCE_TIMEOUT (TICKER_1S)

#define SLOW_UPDATE_TIMEOUT (TICKER_1S/50)

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
	kbSharp,kbAsterisk,
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
			/*0*/ {.type=ptNone},
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
			/*0*/ {.type=ptNone},
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
			/*0*/ {.type=ptNone},
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
			/*0*/ {.type=ptNone},
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
			/*0*/ {.type=ptNone},
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
			/*5*/ {.type=ptCust,.number=8,.shortName="Tune",.longName="Tune hardware",.values={""}},
			/*6*/ {.type=ptNone},
			/*7*/ {.type=ptNone},
			/*8*/ {.type=ptNone},
			/*9*/ {.type=ptNone},
			/*0*/ {.type=ptNone},
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
			/*0*/ {.type=ptNone},
		},
	},
};

static const uint8_t keypadButtonCode[16]=
{
	0x01,0x02,0x04,0x11,0x12,0x14,0x21,0x22,0x24,0x32,
	0x08,0x18,0x28,0x38,
	0x34,0x31
};

static struct
{
	uint16_t potSamples[POT_COUNT][POT_SAMPLES];
	int16_t curPotSample;
	uint16_t potValue[POT_COUNT];
	uint32_t potLockTimeout[POT_COUNT];
	uint32_t potLockValue[POT_COUNT];
	
	int8_t keypadState[16];

	enum uiPage_e activePage;
	int8_t activeSource;
	int8_t sourceChanges,prevSourceChanges;
	uint32_t activeSourceTimeout;
	uint32_t slowUpdateTimeout;
	int8_t slowUpdateTimeoutNumber;
	int8_t pendingScreenClear;

	int8_t presetBank;
	int8_t presetSlot;
	int8_t presetModified;
	
	int8_t tunerActiveVoice;
	
	struct hd44780_data lcd1, lcd2;

} ui;

static uint16_t getPotValue(int8_t pot)
{
	return ui.potValue[pot]&~(ADC_QUANTUM-1);
}

static int sendChar(int lcd, int ch)
{
	if(lcd==2)
		hd44780_driver.write(&ui.lcd2,ch);	
	else
		hd44780_driver.write(&ui.lcd1,ch);	
	return -1;
}

static int putc_lcd2 (int ch)
{
	return sendChar(2,ch);
}


static void sendString(int lcd, const char * s)
{
	while(*s)
		sendChar(lcd, *s++);
}

static void clear(int lcd)
{
	if(lcd==2)
		hd44780_driver.clear(&ui.lcd2);	
	else
		hd44780_driver.clear(&ui.lcd1);	
}

static void setPos(int lcd, int col, int row)
{
	if(lcd==2)
		hd44780_driver.set_position(&ui.lcd2,col+row*HD44780_LINE_OFFSET);	
	else
		hd44780_driver.set_position(&ui.lcd1,col+row*HD44780_LINE_OFFSET);	
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
	static char dv[41];
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

		// always 40chars
		for(int i=strlen(dv);i<40;++i) dv[i]=' ';
		dv[40]=0;
		
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
				v=0;
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
				v=ui.presetSlot+ui.presetBank*PRESET_SLOTS;
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
		memset(&ui.potLockTimeout[0],0,POT_COUNT*sizeof(uint32_t));

		ui.pendingScreenClear=1;
		
		return;
	}
	
	potnum=-source-1;
	prm=&uiParameters[ui.activePage][source<0?0:1][source<0?potnum:source];

	if(ui.activePage==upNone || prm->type==ptNone)
		return;

	// fullscreen display
	++ui.sourceChanges;
	ui.activeSourceTimeout=currentTick+ACTIVE_SOURCE_TIMEOUT;
	if (ui.activeSource!=source)
	{
		ui.pendingScreenClear=ui.sourceChanges==1 || ui.sourceChanges!=ui.prevSourceChanges;
		if(!ui.pendingScreenClear)
			ui.activeSourceTimeout=0;
		ui.activeSource=source;
	}
	
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
			case 2:
				arp_setMode((arp_getMode()+1)%4,arp_getHold());
				break;
			case 3:
				arp_setMode(arp_getMode(),!arp_getHold());
				break;
			case 4:
				ui.presetSlot=(getPotValue(potnum)*PRESET_SLOTS)>>16;
				break;
			case 6:
				preset_saveCurrent(ui.presetSlot+ui.presetBank*PRESET_SLOTS);
				/* fall through */
			case 5:
				if(preset_loadCurrent(ui.presetSlot+ui.presetBank*PRESET_SLOTS))
				{
					settings.presetNumber=ui.presetSlot+ui.presetBank*PRESET_SLOTS;
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
				ui.presetBank=(getPotValue(potnum)*PRESET_BANKS)>>16;
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
	int key;
	int row,col[4];
	int8_t newState;

	for(row=0;row<4;++row)
	{
		GPIO_SetValue(0,0b1111<<19);
		GPIO_ClearValue(0,(8>>row)<<19);
		delay_us(20);
		col[row]=0;
		col[row]|=((GPIO_ReadValue(0)>>10)&1)?0:1;
		col[row]|=((GPIO_ReadValue(4)>>29)&1)?0:2;
		col[row]|=((GPIO_ReadValue(4)>>28)&1)?0:4;
		col[row]|=((GPIO_ReadValue(2)>>13)&1)?0:8;
	}
	
	for(key=0;key<16;++key)
	{
		newState=(col[keypadButtonCode[key]>>4]&keypadButtonCode[key])?1:0;
		
		if(key==kb7 && (!!newState)^(!!ui.keypadState[key]))
			assigner_assignNote(26,newState,newState?FULL_RANGE:0,1);
		if(key==kb8 && (!!newState)^(!!ui.keypadState[key]))
			assigner_assignNote(57,newState,newState?FULL_RANGE:0,1);
		if(key==kb9 && (!!newState)^(!!ui.keypadState[key]))
			assigner_assignNote(108,newState,newState?FULL_RANGE:0,1);
		if(key==kb0 && newState && !ui.keypadState[key])
			assigner_allKeysOff();
			
		
		if(newState && ui.keypadState[key])
		{
			ui.keypadState[key]=MIN(INT8_MAX,ui.keypadState[key]+1);

			if(ui.keypadState[key]==DEBOUNCE_THRESHOLD)
				handleUserInput(key);
		}
		else
		{
			ui.keypadState[key]=newState;
		}
	}
}

static void readPots(void)
{
	uint32_t new;
	int i,pot;
	uint16_t tmp[POT_SAMPLES];
	
	ui.curPotSample=(ui.curPotSample+1)%POT_SAMPLES;
	
	GPIO_ClearValue(0,1<<24); // CS
	delay_us(8);

	for(pot=0;pot<POT_COUNT;++pot)
	{
		// read pot on TLV1543 ADC

		new=0;

		BLOCK_INT
		{
			for(i=0;i<16;++i)
			{
				// read value back

				if(i<10)
					new|=((GPIO_ReadValue(0)&(1<<25))?1:0)<<(15-i);

				// send next address

				if(i<4)
				{
					int nextPot=(pot+1)%POT_COUNT;
					if(nextPot&(1<<(3-i)))
						GPIO_SetValue(0,1<<26); // ADDR 
					else
						GPIO_ClearValue(0,1<<26); // ADDR 
				}

				// wiggle clock

				delay_us(4);
				GPIO_SetValue(0,1<<27); // CLK
				delay_us(4);
				GPIO_ClearValue(0,1<<27); // CLK
				delay_us(2);
			}
		}

		ui.potSamples[pot][ui.curPotSample]=new;
		
		if(ui.activePage==upNone)
			rprintf(0,"% 8d", new>>6);

		// sort values

		memcpy(&tmp[0],&ui.potSamples[pot][0],POT_SAMPLES*sizeof(uint16_t));
		qsort(tmp,POT_SAMPLES,sizeof(uint16_t),uint16Compare);		
		
		// median
	
		new=tmp[POT_SAMPLES/2];
		
		// ignore small changes

		if(abs(new-ui.potValue[pot])>=POT_THRESHOLD || currentTick<ui.potLockTimeout[pot])
		{
			if(abs(new-ui.potLockValue[pot])>=POT_TIMEOUT_THRESHOLD)
			{
				ui.potLockTimeout[pot]=currentTick+POT_TIMEOUT;
				ui.potLockValue[pot]=new;
			}

			ui.potValue[pot]=new;

			handleUserInput(-pot-1);
		}
	}

	GPIO_SetValue(0,1<<24); // CS
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
	
	// init TLV1543 ADC
	
	PINSEL_SetI2C0Pins(PINSEL_I2C_Fast_Mode, DISABLE);
	
	PINSEL_SetPinFunc(0,24,0); // CS
	PINSEL_SetPinFunc(0,25,0); // OUT
	PINSEL_SetPinFunc(0,26,0); // ADDR
	PINSEL_SetPinFunc(0,27,0); // CLK
	PINSEL_SetPinFunc(0,28,0); // EOC
	
	GPIO_SetValue(0,0b01101ul<<24);
	GPIO_SetDir(0,0b01101ul<<24,1);
	
	// init keypad
	
	PINSEL_SetPinFunc(0,22,0); // R1
	PINSEL_SetPinFunc(0,21,0); // R2
	PINSEL_SetPinFunc(0,20,0); // R3
	PINSEL_SetPinFunc(0,19,0); // R4
	PINSEL_SetPinFunc(0,10,0); // C1
	PINSEL_SetPinFunc(4,29,0); // C2
	PINSEL_SetPinFunc(4,28,0); // C3
	PINSEL_SetPinFunc(2,13,0); // C4
	PINSEL_SetResistorMode(0,10,PINSEL_PINMODE_PULLUP);
	PINSEL_SetResistorMode(4,29,PINSEL_PINMODE_PULLUP);
	PINSEL_SetResistorMode(4,28,PINSEL_PINMODE_PULLUP);
	PINSEL_SetResistorMode(2,13,PINSEL_PINMODE_PULLUP);

	GPIO_SetValue(0,0b1111ul<<19);
	GPIO_SetDir(0,0b1111ul<<19,1);
	
	// init screen
	
	ui.lcd1.port = 2;
	ui.lcd1.pins.d4 = 1;
	ui.lcd1.pins.d5 = 0;
	ui.lcd1.pins.d6 = 2;
	ui.lcd1.pins.d7 = 3;
	ui.lcd1.pins.rs = 4;
	ui.lcd1.pins.rw = 5;
	ui.lcd1.pins.e = 6;
	ui.lcd1.Te = 1;
	ui.lcd1.caps = HD44780_CAPS_2LINES;

	ui.lcd2.port = 2;
	ui.lcd2.pins.d4 = 1;
	ui.lcd2.pins.d5 = 0;
	ui.lcd2.pins.d6 = 2;
	ui.lcd2.pins.d7 = 3;
	ui.lcd2.pins.rs = 4;
	ui.lcd2.pins.rw = 5;
	ui.lcd2.pins.e = 7;
	ui.lcd2.Te = 1;
	ui.lcd2.caps = HD44780_CAPS_2LINES;
	
	hd44780_driver.init(&ui.lcd1);
	hd44780_driver.onoff(&ui.lcd1, HD44780_ONOFF_DISPLAY_ON);
	hd44780_driver.init(&ui.lcd2);
	hd44780_driver.onoff(&ui.lcd2, HD44780_ONOFF_DISPLAY_ON);
		
    rprintf_devopen(1,putc_lcd2); 

	// welcome message

	sendString(1,"GliGli's OverCycler2");
	setPos(1,0,1);
	sendString(1,"Build " __DATE__ " " __TIME__);

	ui.activePage=upNone;
	ui.activeSource=INT8_MAX;
	ui.presetSlot=-1;
	ui.presetBank=-1;
	ui.pendingScreenClear=1;
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
		ui.presetSlot=settings.presetNumber%PRESET_SLOTS;
		ui.presetBank=settings.presetNumber/PRESET_SLOTS;
	}
	
	// update pot values

	readPots();

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
	
		// don't go fullscreen if more than one source is edited at the same time
	fsDisp=ui.activeSourceTimeout>currentTick && ui.sourceChanges<2;
	if(!fsDisp && ui.activeSourceTimeout)
	{
		ui.activeSourceTimeout=0;
		ui.pendingScreenClear=1;
	}
	
	if(ui.pendingScreenClear)
	{
		clear(1);
		clear(2);
	}

	setPos(1,0,0);
	setPos(2,0,0);

	if(ui.activePage==upNone)
	{
		if(ui.pendingScreenClear)
		{
			sendString(1,"GliGli's OverCycler2                    ");
			sendString(1,"A: Oscillators     B: Filter            ");
			sendString(2,"C: Amplifier       D: LFO/Modulations   ");
			sendString(2,"*: Miscellaneous   #: Sequencer/Arp     ");
		}
	}
	else if(fsDisp) // fullscreen display
	{
		if(ui.pendingScreenClear)
			sendString(1,getName(ui.activeSource,1));

		setPos(2,18,0);
		sendString(2,getDisplayValue(ui.activeSource,&v));
		
		setPos(2,0,1);
		s=getDisplayFulltext(ui.activeSource);
		if(s)
		{
			sendString(2,s);
		}
		else
		{
			// bargraph			
			for(i=0;i<40;++i)
				if(i<(((int32_t)v+UINT16_MAX/40)*40>>16))
					sendChar(2,255);
				else
					sendChar(2,' ');
		}
	}
	else
	{
		ui.activeSource=INT8_MAX;

		 // buttons
		
		setPos(1,26,1);
		setPos(2,26,0);
		for(i=0;i<6;++i)
		{
			int lcd=(i<3)?1:2;
			sendString(lcd,getDisplayValue(i,NULL));
			if(i!=5 && i!=2)
				sendChar(lcd,' ');
		}
	
		if(ui.pendingScreenClear)
		{
			setPos(1,26,0);
			
			for(i=0;i<3;++i)
			{
				sendString(1,getName(i,0));
				if(i<2)
					sendChar(1,' ');
			}

			setPos(2,26,1);
			for(i=3;i<6;++i)
			{
				sendString(2,getName(i,0));
				if(i<5)
					sendChar(2,' ');
			}
		}
		
		// delimiter

		if(ui.pendingScreenClear)
		{
			#define DELIM(x) sendChar(x,'|'); sendChar(x,'|');

			setPos(1,24,0); DELIM(1)
			setPos(1,24,1); DELIM(1)
			setPos(2,24,0); DELIM(2)
			setPos(2,24,1); DELIM(2)
		}
		
		// pots

		setPos(1,0,1);
		hd44780_driver.home(&ui.lcd2);
		for(i=0;i<POT_COUNT;++i)
		{
			int lcd=(i<POT_COUNT/2)?1:2;
			sendString(lcd,getDisplayValue(-i-1,NULL));
			if(i!=POT_COUNT-1 && i!=POT_COUNT/2-1)
				sendChar(lcd,' ');
		}

		if(ui.pendingScreenClear)
		{
			hd44780_driver.home(&ui.lcd1);
			for(i=0;i<POT_COUNT/2;++i)
			{
				sendString(1,getName(-i-1,0));
				if(i<POT_COUNT/2-1)
					sendChar(1,' ');
			}

			setPos(2,0,1);
			for(i=POT_COUNT/2;i<POT_COUNT;++i)
			{
				sendString(2,getName(-i-1,0));
				if(i<POT_COUNT-1)
					sendChar(2,' ');
			}
		}
	}

	ui.pendingScreenClear=0;
	ui.prevSourceChanges=ui.sourceChanges;
	ui.sourceChanges=0;

	// read keypad
			
	readKeypad();
}
