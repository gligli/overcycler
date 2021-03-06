///////////////////////////////////////////////////////////////////////////////
// LCD / keypad / potentiometers user interface
///////////////////////////////////////////////////////////////////////////////

#include "ui.h"
#include "storage.h"
#include "integer.h"
#include "arp.h"
#include "seq.h"
#include "ffconf.h"
#include "dacspi.h"
#include "hd44780.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "clock.h"
#include "scan.h"

#define PRESET_SLOTS 20
#define PRESET_BANKS 20

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
	upNone=-1,upOscs=0,upFil=1,upAmp=2,upLFO1=3,upLFO2=4,upArp=5,upSeq=6,upMisc=7,upTuner=8
};

struct uiParam_s
{
	enum uiParamType_e type;
	int8_t number;
	const char * shortName; // 4 chars + zero termination
	const char * longName;
	const char * values[8]; // 4 chars + zero termination
};

const struct uiParam_s uiParameters[9][2][10] = // [pages][0=pots/1=keys][pot/key num]
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
			/*1*/ {.type=ptStep,.number=spAWModType,.shortName="AWmT",.longName="Osc A WaveMod Type",.values={"Off ","Alia","Wdth","Freq","XOvr"}},
			/*2*/ {.type=ptStep,.number=spBWModType,.shortName="BWmT",.longName="Osc B WaveMod Type",.values={"Off ","Alia","Wdth","Freq"}},
			/*3*/ {.type=ptStep,.number=spChromaticPitch,.shortName="FrqM",.longName="Frequency Mode",.values={"Free","Semi","Oct "}},
			/*4*/ {.type=ptStep,.number=spOscSync,.shortName="Sync",.longName="Oscillator A to B Synchronization",.values={"Off ","On  "}},
			/*5*/ {.type=ptCust,.number=23,.shortName="XoCp",.longName="Crossover WaveMod Copy B Bank/Wave"},
			/*6*/ {.type=ptNone},
			/*7*/ {.type=ptCust,.number=19,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
			/*8*/ {.type=ptNone},
			/*9*/ {.type=ptNone},
			/*0*/ {.type=ptCust,.number=26,.shortName="Panc",.longName="All voices off (MIDI panic)",.values={""}},
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
			/*1*/ {.type=ptStep,.number=spAWModEnvEn,.shortName="AWmE",.longName="Osc A WaveMod Envelope",.values={"Off ","On  "}},
			/*2*/ {.type=ptStep,.number=spBWModEnvEn,.shortName="BWmE",.longName="Osc B WaveMod Envelope",.values={"Off ","On  "}},
			/*3*/ {.type=ptStep,.number=spFilEnvSlow,.shortName="FEnT",.longName="Filter Envelope Type",.values={"Fast","Slow"}},
			/*4*/ {.type=ptStep,.number=spFilEnvLin,.shortName="FEnS",.longName="Filter Envelope Shape",.values={"Exp ","Lin "}},
			/*5*/ {.type=ptNone},
			/*6*/ {.type=ptNone},
			/*7*/ {.type=ptCust,.number=19,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
			/*8*/ {.type=ptNone},
			/*9*/ {.type=ptNone},
			/*0*/ {.type=ptCust,.number=26,.shortName="Panc",.longName="All voices off (MIDI panic)",.values={""}},
		},
	},
	/* Amplifier page (C) */
	{
		{
			/* 1st row of pots */
			{.type=ptCont,.number=cpGlide,.shortName="Glid",.longName="Glide amount"},
			{.type=ptCont,.number=cpUnisonDetune,.shortName="MDet",.longName="Master unison Detune"},
			{.type=ptCont,.number=cpMasterTune,.shortName="MTun",.longName="Master Tune"},
			{.type=ptStep,.number=spVoiceCount,.shortName="VCnt",.longName="Voice count",.values={"   1","   2","   3","   4","   5","   6"}},
			{.type=ptCont,.number=cpNoiseVol,.shortName="NVol",.longName="Noise Volume"},
			/* 2nd row of pots */
			{.type=ptCont,.number=cpAmpAtt,.shortName="AAtk",.longName="Amplifier Attack"},
			{.type=ptCont,.number=cpAmpDec,.shortName="ADec",.longName="Amplifier Decay"},
			{.type=ptCont,.number=cpAmpSus,.shortName="ASus",.longName="Amplifier Sustain"},
			{.type=ptCont,.number=cpAmpRel,.shortName="ARel",.longName="Amplifier Release"},
			{.type=ptCont,.number=cpAmpVelocity,.shortName="AVel",.longName="Amplifier Velocity"},
		},
		{
			/*1*/ {.type=ptStep,.number=spUnison,.shortName="Unis",.longName="Unison",.values={"Off ","On  "}},
			/*2*/ {.type=ptStep,.number=spAssignerPriority,.shortName="Prio",.longName="Assigner Priority",.values={"Last","Low ","High"}},
			/*3*/ {.type=ptStep,.number=spAmpEnvSlow,.shortName="AEnT",.longName="Amplifier Envelope Type",.values={"Fast","Slow"}},
			/*4*/ {.type=ptNone},
			/*5*/ {.type=ptNone},
			/*6*/ {.type=ptNone},
			/*7*/ {.type=ptCust,.number=19,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
			/*8*/ {.type=ptNone},
			/*9*/ {.type=ptNone},
			/*0*/ {.type=ptCust,.number=26,.shortName="Panc",.longName="All voices off (MIDI panic)",.values={""}},
		},
	},
	/* LFO1 page (D) */
	{
		{
			/* 1st row of pots */
			{.type=ptCont,.number=cpLFOFreq,.shortName="1Frq",.longName="LFO1 Frequency"},
			{.type=ptCont,.number=cpLFOAmt,.shortName="1Amt",.longName="LFO1 Amount (base)"},
			{.type=ptStep,.number=spLFOShape,.shortName="1Wav",.longName="LFO1 Waveform",.values={"Sqr ","Tri ","Rand","Sine","Nois","Saw ","RSaw"}},
			{.type=ptNone},
			{.type=ptCont,.number=cpModDelay,.shortName="MDly",.longName="Modulation Delay"},
			/* 2nd row of pots */
			{.type=ptCont,.number=cpLFOPitchAmt,.shortName="1Pit",.longName="Pitch LFO1 Amount"},
			{.type=ptCont,.number=cpLFOWModAmt,.shortName="1Wmo",.longName="WaveMod LFO1 Amount"},
			{.type=ptCont,.number=cpLFOFilAmt,.shortName="1Fil",.longName="Filter frequency LFO1 Amount"},
			{.type=ptCont,.number=cpLFOResAmt,.shortName="1Res",.longName="Filter resonance LFO1 Amount"},
			{.type=ptCont,.number=cpLFOAmpAmt,.shortName="1Amp",.longName="Amplifier LFO1 Amount"},
		},
		{
			/*1*/ {.type=ptStep,.number=spLFOShift,.shortName="1Rng",.longName="LFO1 Range",.values={"Slow","Fast"}},
			/*2*/ {.type=ptStep,.number=spModwheelRange,.shortName="MRng",.longName="Modwheel Range",.values={"Min ","Low ","High","Full"}},
			/*3*/ {.type=ptStep,.number=spBenderRange,.shortName="BRng",.longName="Bender Range",.values={"3rd ","5th ","Oct "}},
			/*4*/ {.type=ptStep,.number=spLFOTargets,.shortName="1Tgt",.longName="LFO1 Osc Target",.values={"Off ","OscA","OscB","Both"}},
			/*5*/ {.type=ptStep,.number=spModwheelTarget,.shortName="MTgt",.longName="Modwheel Target",.values={"LFO1","LFO2"}},
			/*6*/ {.type=ptStep,.number=spBenderTarget,.shortName="BTgt",.longName="Bender Target",.values={"Off ","Pit ","Fil ","Amp "}},
			/*7*/ {.type=ptCust,.number=19,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
			/*8*/ {.type=ptNone},
			/*9*/ {.type=ptNone},
			/*0*/ {.type=ptCust,.number=26,.shortName="Panc",.longName="All voices off (MIDI panic)",.values={""}},
		},
	},
	/* LFO2 page (D) */
	{
		{
			/* 1st row of pots */
			{.type=ptCont,.number=cpLFO2Freq,.shortName="2Frq",.longName="LFO2 Frequency"},
			{.type=ptCont,.number=cpLFO2Amt,.shortName="2Amt",.longName="LFO2 Amount (base)"},
			{.type=ptStep,.number=spLFO2Shape,.shortName="2Wav",.longName="LFO2 Waveform",.values={"Sqr ","Tri ","Rand","Sine","Nois","Saw ","RSaw"}},
			{.type=ptNone},
			{.type=ptCont,.number=cpModDelay,.shortName="MDly",.longName="Modulation Delay"},
			/* 2nd row of pots */
			{.type=ptCont,.number=cpLFO2PitchAmt,.shortName="2Pit",.longName="Pitch LFO2 Amount"},
			{.type=ptCont,.number=cpLFO2WModAmt,.shortName="2Wmo",.longName="WaveMod LFO2 Amount"},
			{.type=ptCont,.number=cpLFO2FilAmt,.shortName="2Fil",.longName="Filter frequency LFO2 Amount"},
			{.type=ptCont,.number=cpLFO2ResAmt,.shortName="2Res",.longName="Filter resonance LFO2 Amount"},
			{.type=ptCont,.number=cpLFO2AmpAmt,.shortName="2Amp",.longName="Amplifier LFO2 Amount"},
		},
		{
			/*1*/ {.type=ptStep,.number=spLFO2Shift,.shortName="2Rng",.longName="LFO2 Range",.values={"Slow","Fast"}},
			/*2*/ {.type=ptStep,.number=spModwheelRange,.shortName="MRng",.longName="Modwheel Range",.values={"Min ","Low ","High","Full"}},
			/*3*/ {.type=ptStep,.number=spBenderRange,.shortName="BRng",.longName="Bender Range",.values={"3rd ","5th ","Oct "}},
			/*4*/ {.type=ptStep,.number=spLFO2Targets,.shortName="2Tgt",.longName="LFO2 Osc Target",.values={"Off ","OscA","OscB","Both"}},
			/*5*/ {.type=ptStep,.number=spModwheelTarget,.shortName="MTgt",.longName="Modwheel Target",.values={"LFO1","LFO2"}},
			/*6*/ {.type=ptStep,.number=spBenderTarget,.shortName="BTgt",.longName="Bender Target",.values={"Off ","Pit ","Fil ","Amp "}},
			/*7*/ {.type=ptCust,.number=19,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
			/*8*/ {.type=ptNone},
			/*9*/ {.type=ptNone},
			/*0*/ {.type=ptCust,.number=26,.shortName="Panc",.longName="All voices off (MIDI panic)",.values={""}},
		},
	},
	/* Arpeggiator page (#) */
	{
		{
			/* 1st row of pots */
			{.type=ptCust,.number=22,.shortName="Clk ",.longName="Seq/Arp Clock"},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptNone},
			/* 2nd row of pots */
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptCust,.number=20,.shortName="Trsp",.longName="Transpose (hit 7 then a note to change)"},
		},
		{
			/*1*/ {.type=ptCust,.number=2,.shortName="AMod",.longName="Arp Mode",.values={"Off ","UpDn","Rand","Asgn"}},
			/*2*/ {.type=ptCust,.number=3,.shortName="AHld",.longName="Arp Hold",.values={"Off ","On "}},
			/*3*/ {.type=ptNone},
			/*4*/ {.type=ptNone},
			/*5*/ {.type=ptNone},
			/*6*/ {.type=ptNone},
			/*7*/ {.type=ptCust,.number=19,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
			/*8*/ {.type=ptNone},
			/*9*/ {.type=ptNone},
			/*0*/ {.type=ptCust,.number=26,.shortName="Panc",.longName="All voices off (MIDI panic)",.values={""}},
		},
	},
	/* Sequencer page (#) */
	{
		{
			/* 1st row of pots */
			{.type=ptCust,.number=22,.shortName="Clk ",.longName="Seq/Arp Clock"},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptNone},
			/* 2nd row of pots */
			{.type=ptCust,.number=21,.shortName="SBnk",.longName="Sequencer memory Bank"},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptCust,.number=20,.shortName="Trsp",.longName="Transpose (hit 7 then a note to change)"},
		},
		{
			/*1*/ {.type=ptCust,.number=13,.shortName="APly",.longName="Seq A Play/stop",.values={"Stop","Wait","Play","Rec "}},
			/*2*/ {.type=ptCust,.number=14,.shortName="BPly",.longName="Seq B Play/stop",.values={"Stop","Wait","Play","Rec "}},
			/*3*/ {.type=ptCust,.number=15,.shortName="SRec",.longName="Seq record",.values={"Off ","SeqA","SeqB"}},
			/*4*/ {.type=ptCust,.number=17,.shortName="TiRe",.longName="Add Tie/Rest"},
			/*5*/ {.type=ptCust,.number=16,.shortName="Back",.longName="Back one step"},
			/*6*/ {.type=ptCust,.number=18,.shortName="Clr ",.longName="Clear sequence"},
			/*7*/ {.type=ptCust,.number=19,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
			/*8*/ {.type=ptNone},
			/*9*/ {.type=ptNone},
			/*0*/ {.type=ptCust,.number=26,.shortName="Panc",.longName="All voices off (MIDI panic)",.values={""}},
		},
	},
	/* Miscellaneous page (*) */
	{
		{
			/* 1st row of pots */
			{.type=ptCust,.number=4,.shortName="Slot",.longName="Preset Slot"},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptCust,.number=12,.shortName="Sync",.longName="Sync mode",.values={"Int ","MIDI"}},
			/* 2nd row of pots */
			{.type=ptCust,.number=11,.shortName="Bank",.longName="Preset Bank"},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptCust,.number=7,.shortName="MidC",.longName="Midi Channel"},
		},
		{
			/*1*/ {.type=ptCust,.number=5,.shortName="Load",.longName="Load preset"},
			/*2*/ {.type=ptNone},
			/*3*/ {.type=ptCust,.number=6,.shortName="Save",.longName="Save preset"},
			/*4*/ {.type=ptCust,.number=24,.shortName="LPrv",.longName="Load previous preset",.values={""}},
			/*5*/ {.type=ptCust,.number=25,.shortName="LNxt",.longName="Load next preset",.values={""}},
			/*6*/ {.type=ptCust,.number=8,.shortName="Tune",.longName="Tune filters",.values={""}},
			/*7*/ {.type=ptCust,.number=19,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
			/*8*/ {.type=ptNone},
			/*9*/ {.type=ptNone},
			/*0*/ {.type=ptCust,.number=26,.shortName="Panc",.longName="All voices off (MIDI panic)",.values={""}},
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
			/*7*/ {.type=ptCust,.number=19,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
			/*8*/ {.type=ptNone},
			/*9*/ {.type=ptNone},
			/*0*/ {.type=ptCust,.number=26,.shortName="Panc",.longName="All voices off (MIDI panic)",.values={""}},
		},
	},
};

static struct
{
	enum uiPage_e activePage;
	int8_t activeSource;
	int8_t sourceChanges,prevSourceChanges;
	uint32_t activeSourceTimeout;
	uint32_t slowUpdateTimeout;
	int16_t slowUpdateTimeoutNumber;
	int8_t pendingScreenClear;

	int8_t presetBank;
	int8_t presetSlot;
	int8_t presetModified;
	int8_t settingsModified;
	
	int8_t tunerActiveVoice;
	
	int8_t seqRecordingTrack;
	int8_t isTransposing;
	int32_t transpose;
	
	struct hd44780_data lcd1, lcd2;

} ui;

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
	int32_t v;

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
			case 13:
			case 14:
				v=seq_getMode(prm->number-13);
				break;
			case 15:
				v=ui.seqRecordingTrack+1;
				break;
			case 16:
			case 17:
			case 18:
				v=-1;
				if(ui.seqRecordingTrack>=0)
					v=seq_getStepCount(ui.seqRecordingTrack)-1;
				break;
			case 19:
				v=ui.isTransposing;
				break;
			case 20:
				v=ui.transpose-1;
				break;
			case 21:
				v=settings.sequencerBank;
				break;
			case 22:
				if(settings.syncMode==smMIDI)
				{
					v=clock_getSpeed();
					if(v==UINT16_MAX)
						v=0;
					--v;
				}
				else
				{
					v=(((int)settings.seqArpClock*1000)>>16)-1;
				}
				break;
			case 23:
				v=(currentPreset.steppedParameters[spXOvrBank]+1)*100;
				v+=(currentPreset.steppedParameters[spXOvrWave]+1)%100;
				--v;
				break;
			case 24:
			case 25:
			case 26:
				v=0;
				break;
			}
		}
		
		if(v>=0 && v<valCount)
			strcpy(dv,prm->values[v]);
		else
			sprintf(dv,"%4d",v+1);
		break;
	default:
		;
	}
	
	return dv;
}

void ui_scanEvent(int8_t source) // source: keypad (kb0..kbSharp) / (-1..-10)
{
	int32_t data,valCount;
	int8_t potnum,change;
	const struct uiParam_s * prm;

//	rprintf(0,"handleUserInput %d\n",source);
	
	if(source>=kbA)
	{
		switch(source)
		{
			case kbA: 
				ui.activePage=upOscs;
				break;
			case kbB: 
				ui.activePage=upFil;
				break;
			case kbC: 
				ui.activePage=upAmp;
				break;
			case kbD: 
				ui.activePage=(ui.activePage==upLFO1)?upLFO2:upLFO1;
				break;
			case kbAsterisk: 
				ui.activePage=upMisc;
				break;
			case kbSharp: 
				ui.activePage=(ui.activePage==upArp)?upSeq:upArp;
				break;
		}

		rprintf(0,"page %d\n",ui.activePage);

		// cancel ongoing changes
		ui.activeSourceTimeout=0;
		scan_resetPotLocking();

		ui.pendingScreenClear=1;
		
		// to store changes made to settings thru UI
		if(ui.settingsModified)
		{
			settings_save();
			ui.settingsModified=0;
		}
		
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
		change=currentPreset.continuousParameters[prm->number]!=scan_getPotValue(potnum);
		currentPreset.continuousParameters[prm->number]=scan_getPotValue(potnum);
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
			data=(scan_getPotValue(potnum)*valCount)>>16;
		else
			data=(currentPreset.steppedParameters[prm->number]+1)%valCount;

		change=currentPreset.steppedParameters[prm->number]!=data;
		currentPreset.steppedParameters[prm->number]=data;
		
		//	special cases
		if(change)
		{
			if(prm->number==spABank || prm->number==spBBank || prm->number==spAWave || prm->number==spBWave)
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
				ui.presetSlot=(scan_getPotValue(potnum)*PRESET_SLOTS)>>16;
				break;
			case 5:
			case 6:
				ui.slowUpdateTimeout=currentTick+SLOW_UPDATE_TIMEOUT;
				ui.slowUpdateTimeoutNumber=prm->number+0x80;
				break;
			case 7:
				data=((scan_getPotValue(potnum)*17)>>16)-1;
				settings.midiReceiveChannel=data;
				ui.settingsModified=1;
				break;
			case 8:
				ui.activePage=upTuner;
				ui.pendingScreenClear=1;
				break;
			case 9:
				settings.tunes[potnum][ui.tunerActiveVoice]=TUNER_FIL_INIT_OFFSET+potnum*TUNER_FIL_INIT_SCALE-4096+(scan_getPotValue(potnum)>>3);
				settings_save();
				break;
			case 10:
				ui.tunerActiveVoice=source-kb1;
				break;
			case 11:
				ui.presetBank=(scan_getPotValue(potnum)*PRESET_BANKS)>>16;
				break;
			case 12:
				data=(scan_getPotValue(potnum)*2)>>16;
				settings.syncMode=data;
				ui.settingsModified=1;
				break;
			case 13:
			case 14:
				data=prm->number-13;
				seq_setMode(data,seq_getMode(data)==smPlaying?smOff:smPlaying);
				if(data==ui.seqRecordingTrack)
					ui.seqRecordingTrack=-1;
				break;
			case 15:
				ui.seqRecordingTrack=ui.seqRecordingTrack>=1?-1:ui.seqRecordingTrack+1;
				if(seq_getMode(0)==smRecording) seq_setMode(0,smOff);
				if(seq_getMode(1)==smRecording) seq_setMode(1,smOff);
				if(ui.seqRecordingTrack>=0)
					seq_setMode(ui.seqRecordingTrack,smRecording);
				break;
			case 16:
				if(ui.seqRecordingTrack>=0)
					seq_inputNote(SEQ_NOTE_UNDO,1);
				break;
			case 17:
				if(ui.seqRecordingTrack>=0)
					seq_inputNote(SEQ_NOTE_STEP,1);
				break;
			case 18:
				if(ui.seqRecordingTrack>=0)
					seq_inputNote(SEQ_NOTE_CLEAR,1);
				break;
			case 19:
				ui.isTransposing=(ui.isTransposing+1)%3;
				break;
			case 20:
				data=(((int32_t)97*scan_getPotValue(potnum))>>16)-48;
				ui_setTranspose(data);
				break;
			case 21:
				data=((int32_t)20*scan_getPotValue(potnum))>>16;
				settings.sequencerBank=data;
				ui.settingsModified=1;
				break;				
			case 22:
				settings.seqArpClock=scan_getPotValue(potnum);
				ui.settingsModified=1;
				break;
			case 23:
				currentPreset.steppedParameters[spXOvrBank]=currentPreset.steppedParameters[spBBank];
				currentPreset.steppedParameters[spXOvrWave]=currentPreset.steppedParameters[spBWave];
				refreshWaveforms(2);
				break;
			case 24:
			case 25:
				ui.presetSlot = ui.presetSlot + ((prm->number == 24) ? -1 : 1);
				if(ui.presetSlot >= PRESET_SLOTS)
				{
					ui.presetSlot = 0;
					++ui.presetBank;
				}
				else if(ui.presetSlot < 0)
				{
					ui.presetSlot = PRESET_SLOTS - 1;
					--ui.presetBank;
				}
				ui.presetBank = ui.presetBank % PRESET_BANKS;

				ui.slowUpdateTimeout=currentTick+SLOW_UPDATE_TIMEOUT;
				ui.slowUpdateTimeoutNumber=0x85;
				break;
			case 26:
				assigner_panicOff();
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

void ui_setPresetModified(int8_t modified)
{
	ui.presetModified=modified;
}

int8_t ui_isPresetModified(void)
{
	return ui.presetModified;
}

int8_t ui_isTransposing(void)
{
	return ui.isTransposing;
}

int32_t ui_getTranspose(void)
{
	return ui.transpose;
}

void ui_setTranspose(int32_t transpose)
{
	if(ui.transpose==transpose)
		return;
	
	// prevent hanging notes on transposition changes
	if(assigner_getAnyPressed())
		assigner_allKeysOff();
	
	ui.transpose=transpose;
	seq_setTranspose(transpose);
	arp_setTranspose(transpose);
	if(ui.isTransposing==1) //once?
		ui.isTransposing=0;
}


void ui_init(void)
{
	memset(&ui,0,sizeof(ui));
	
	// init screen
	
	ui.lcd1.port = 2;
	ui.lcd1.pins.d4 = 1;
	ui.lcd1.pins.d5 = 0;
	ui.lcd1.pins.d6 = 2;
	ui.lcd1.pins.d7 = 3;
	ui.lcd1.pins.rs = 4;
	ui.lcd1.pins.rw = 5;
	ui.lcd1.pins.e = 6;
	ui.lcd1.caps = HD44780_CAPS_2LINES;

	ui.lcd2.port = 2;
	ui.lcd2.pins.d4 = 1;
	ui.lcd2.pins.d5 = 0;
	ui.lcd2.pins.d6 = 2;
	ui.lcd2.pins.d7 = 3;
	ui.lcd2.pins.rs = 4;
	ui.lcd2.pins.rw = 5;
	ui.lcd2.pins.e = 7;
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
	rprintf(1,"Sampling at %d Hz", SYNTH_MASTER_CLOCK/DACSPI_TICK_RATE);
	setPos(2,0,1);

	ui.activePage=upNone;
	ui.activeSource=INT8_MAX;
	ui.presetSlot=-1;
	ui.presetBank=-1;
	ui.pendingScreenClear=1;
	ui.seqRecordingTrack=-1;
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
			case 0x80+6:
				preset_saveCurrent(ui.presetSlot+ui.presetBank*PRESET_SLOTS);
				/* fall through */
			case 0x80+5:
				settings.presetNumber=ui.presetSlot+ui.presetBank*PRESET_SLOTS;
				settings_save();                
				if(!preset_loadCurrent(settings.presetNumber))
					preset_loadDefault(1);
				
				refreshWaveNames(0);
				refreshWaveNames(1);
				refreshWaveforms(0);
				refreshWaveforms(1);
				refreshWaveforms(2);
				refreshFullState();
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
			sendString(2,"C: Amplifier       D: LFO1/LFO2         ");
			sendString(2,"*: Miscellaneous   #: Arp/Sequencer     ");
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
				if(i<(((int32_t)v+UINT16_MAX/80)*40>>16))
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
}
