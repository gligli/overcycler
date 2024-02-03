///////////////////////////////////////////////////////////////////////////////
// LCD / keypad / potentiometers user interface
///////////////////////////////////////////////////////////////////////////////

#include "ui.h"
#include "storage.h"
#include "integer.h"
#include "assigner.h"
#include "arp.h"
#include "seq.h"
#include "ffconf.h"
#include "dacspi.h"
#include "hd44780.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x_dac.h"
#include "clock.h"
#include "scan.h"

#define ACTIVE_SOURCE_TIMEOUT (TICKER_1S)

#define SLOW_UPDATE_TIMEOUT (TICKER_1S/5)

#define PANEL_DEADBAND 2048

enum uiParamType_e
{
	ptNone=0,ptCont,ptStep,ptCust
};

enum uiPage_e
{
	upNone=-1,upOscs=0,upWMod=1,upFil=2,upAmp=3,upLFO1=4,upLFO2=5,upArp=6,upSeq=7,upMisc=8,

	// /!\ this must stay last
	upCount
};

enum uiCustomParamNumber_e
{
	cnNone=0,cnAMod,cnAHld,cnPrUn,cnLoad,cnSave,cnMidC,cnTune,cnPrTe,cnSync,cnAPly,cnBPly,cnSRec,cnBack,cnTiRe,cnClr,
	cnTrspM,cnTrspV,cnSBnk,cnClk,cnXoCp,cnLPrv,cnLNxt,cnPanc,cnLBas,cnPack,cnPrHu,cnNPrs,cnNVal,cnUsbM,
	cnCtst
};

struct uiParam_s
{
	enum uiParamType_e type;
	int8_t number;
	const char * shortName; // 4 chars + zero termination
	const char * longName;
	const char * values[8]; // 4 chars + zero termination
	int32_t custPotMul, custPotAdd;
};

const struct uiParam_s uiParameters[upCount][2][SCAN_POT_COUNT] = // [pages][0=pots/1=keys][pot/key num]
{
	/* Oscillators page (A) */
	{
		{
			/* 1st row of pots */
			{.type=ptStep,.number=spABank_Unsaved,.shortName="ABnk",.longName="Osc A Bank"},
			{.type=ptStep,.number=spAWave_Unsaved,.shortName="AWav",.longName="Osc A Waveform"},
			{.type=ptCont,.number=cpAFreq,.shortName="AFrq",.longName="Osc A Frequency"},
			{.type=ptCont,.number=cpNoiseVol,.shortName="NVol",.longName="Noise Volume"},
			{.type=ptCont,.number=cpAVol,.shortName="AVol",.longName="Osc A Volume"},
			/* 2nd row of pots */
			{.type=ptStep,.number=spBBank_Unsaved,.shortName="BBnk",.longName="Osc B Bank"},
			{.type=ptStep,.number=spBWave_Unsaved,.shortName="BWav",.longName="Osc B Waveform"},
			{.type=ptCont,.number=cpBFreq,.shortName="BFrq",.longName="Osc B Frequency"},
			{.type=ptCont,.number=cpDetune,.shortName="Detn",.longName="Osc A/B Detune"},
			{.type=ptCont,.number=cpBVol,.shortName="BVol",.longName="Osc B Volume"},
		},
		{
			/*0*/ {.type=ptCust,.number=cnNVal,.shortName="NVal",.longName="Numerically set last potentiometer value"},
			/*1*/ {.type=ptStep,.number=spOscSync,.shortName="Sync",.longName="Oscillator A to B Synchronization",.values={"Off ","On  "}},
			/*2*/ {.type=ptCust,.number=cnXoCp,.shortName="XoCp",.longName="Crossover WaveMod Copy A Bank/Wave"},
			/*3*/ {.type=ptStep,.number=spChromaticPitch,.shortName="FrqM",.longName="Frequency Mode",.values={"Free","Semi","Oct "}},
			/*4*/ {.type=ptNone},
			/*5*/ {.type=ptNone},
			/*6*/ {.type=ptNone},
			/*7*/ {.type=ptCust,.number=cnTrspM,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
			/*8*/ {.type=ptCust,.number=cnPanc,.shortName="Panc",.longName="All voices off (MIDI panic)",.values={""}},
			/*9*/ {.type=ptCust,.number=cnLBas,.shortName="LBas",.longName="Load basic preset",.values={""}},
		},
	},
	/* WaveMod page (A) */
	{
		{
			/* 1st row of pots */
			{.type=ptCont,.number=cpABaseWMod,.shortName="AWmo",.longName="Osc A WaveMod"},
			{.type=ptCont,.number=cpBBaseWMod,.shortName="BWmo",.longName="Osc B WaveMod"},
			{.type=ptNone},
			{.type=ptCont,.number=cpWModAEnv,.shortName="AWEA",.longName="Osc A WaveMod Envelope amount"},
			{.type=ptCont,.number=cpWModBEnv,.shortName="BWEA",.longName="Osc B WaveMod Envelope amount"},
			/* 2nd row of pots */
			{.type=ptCont,.number=cpWModAtt,.shortName="WAtk",.longName="WaveMod Attack"},
			{.type=ptCont,.number=cpWModDec,.shortName="WDec",.longName="WaveMod Decay"},
			{.type=ptCont,.number=cpWModSus,.shortName="WSus",.longName="WaveMod Sustain"},
			{.type=ptCont,.number=cpWModRel,.shortName="WRel",.longName="WaveMod Release"},
			{.type=ptCont,.number=cpWModVelocity,.shortName="WVel",.longName="WaveMod Velocity"},
		},
		{
			/*0*/ {.type=ptCust,.number=cnNVal,.shortName="NVal",.longName="Numerically set last potentiometer value"},
			/*1*/ {.type=ptStep,.number=spAWModType,.shortName="AWmT",.longName="Osc A WaveMod Type",.values={"Off ","Grit","Wdth","Freq","XOvr"}},
			/*2*/ {.type=ptStep,.number=spBWModType,.shortName="BWmT",.longName="Osc B WaveMod Type",.values={"Off ","Grit","Wdth","Freq"}},
			/*3*/ {.type=ptNone},
			/*4*/ {.type=ptStep,.number=spWModEnvSlow,.shortName="WEnT",.longName="WaveMod Envelope Type",.values={"Fast","Slow"}},
			/*5*/ {.type=ptStep,.number=spWModEnvLoop,.shortName="WEnL",.longName="WaveMod Envelope Loop",.values={"Norm","Loop"}},
			/*6*/ {.type=ptStep,.number=spWModEnvLin,.shortName="WEnS",.longName="WaveMod Envelope Shape",.values={"Exp ","Lin "}},
			/*7*/ {.type=ptCust,.number=cnTrspM,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
			/*8*/ {.type=ptCust,.number=cnPanc,.shortName="Panc",.longName="All voices off (MIDI panic)",.values={""}},
			/*9*/ {.type=ptCust,.number=cnLBas,.shortName="LBas",.longName="Load basic preset",.values={""}},
		},
	},
	/* Filter page (B) */
	{
		{
			/* 1st row of pots */
			{.type=ptCont,.number=cpCutoff,.shortName="FCut",.longName="Filter Cutoff freqency"},
			{.type=ptCont,.number=cpResonance,.shortName="FRes",.longName="Filter Resonance"},
			{.type=ptNone},
			{.type=ptCont,.number=cpFilKbdAmt,.shortName="FKbd",.longName="Filter Keyboard tracking"},
			{.type=ptCont,.number=cpFilEnvAmt,.shortName="FEnv",.longName="Filter Envelope amount"},
			/* 2nd row of pots */
			{.type=ptCont,.number=cpFilAtt,.shortName="FAtk",.longName="Filter Attack"},
			{.type=ptCont,.number=cpFilDec,.shortName="FDec",.longName="Filter Decay"},
			{.type=ptCont,.number=cpFilSus,.shortName="FSus",.longName="Filter Sustain"},
			{.type=ptCont,.number=cpFilRel,.shortName="FRel",.longName="Filter Release"},
			{.type=ptCont,.number=cpFilVelocity,.shortName="FVel",.longName="Filter Velocity"},
		},
		{
			/*0*/ {.type=ptCust,.number=cnNVal,.shortName="NVal",.longName="Numerically set last potentiometer value"},
			/*1*/ {.type=ptNone},
			/*2*/ {.type=ptNone},
			/*3*/ {.type=ptNone},
			/*4*/ {.type=ptStep,.number=spFilEnvSlow,.shortName="FEnT",.longName="Filter Envelope Type",.values={"Fast","Slow"}},
			/*5*/ {.type=ptStep,.number=spFilEnvLoop,.shortName="FEnL",.longName="Filter Envelope Loop",.values={"Norm","Loop"}},
			/*6*/ {.type=ptStep,.number=spFilEnvLin,.shortName="FEnS",.longName="Filter Envelope Shape",.values={"Exp ","Lin "}},
			/*7*/ {.type=ptCust,.number=cnTrspM,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
			/*8*/ {.type=ptCust,.number=cnPanc,.shortName="Panc",.longName="All voices off (MIDI panic)",.values={""}},
			/*9*/ {.type=ptCust,.number=cnLBas,.shortName="LBas",.longName="Load basic preset",.values={""}},
		},
	},
	/* Amplifier page (C) */
	{
		{
			/* 1st row of pots */
			{.type=ptCont,.number=cpGlide,.shortName="Glid",.longName="Glide amount"},
			{.type=ptNone},
			{.type=ptCont,.number=cpUnisonDetune,.shortName="MDet",.longName="Master unison Detune"},
			{.type=ptCont,.number=cpMasterTune,.shortName="MTun",.longName="Master Tune"},
			{.type=ptStep,.number=spVoiceCount,.shortName="VCnt",.longName="Voice count",.values={"   1","   2","   3","   4","   5","   6"}},
			/* 2nd row of pots */
			{.type=ptCont,.number=cpAmpAtt,.shortName="AAtk",.longName="Amplifier Attack"},
			{.type=ptCont,.number=cpAmpDec,.shortName="ADec",.longName="Amplifier Decay"},
			{.type=ptCont,.number=cpAmpSus,.shortName="ASus",.longName="Amplifier Sustain"},
			{.type=ptCont,.number=cpAmpRel,.shortName="ARel",.longName="Amplifier Release"},
			{.type=ptCont,.number=cpAmpVelocity,.shortName="AVel",.longName="Amplifier Velocity"},
		},
		{
			/*0*/ {.type=ptCust,.number=cnNVal,.shortName="NVal",.longName="Numerically set last potentiometer value"},
			/*1*/ {.type=ptStep,.number=spUnison,.shortName="Unis",.longName="Unison",.values={"Off ","On  "}},
			/*2*/ {.type=ptStep,.number=spAssignerPriority,.shortName="Prio",.longName="Assigner Priority",.values={"Last","Low ","High"}},
			/*3*/ {.type=ptNone},
			/*4*/ {.type=ptStep,.number=spAmpEnvSlow,.shortName="AEnT",.longName="Amplifier Envelope Type",.values={"Fast","Slow"}},
			/*5*/ {.type=ptStep,.number=spAmpEnvLoop,.shortName="AEnL",.longName="Amplifier Envelope Loop",.values={"Norm","Loop"}},
			/*6*/ {.type=ptStep,.number=spAmpEnvLin,.shortName="AEnS",.longName="Amplifier Envelope Shape",.values={"Exp ","Lin "}},
			/*7*/ {.type=ptCust,.number=cnTrspM,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
			/*8*/ {.type=ptCust,.number=cnPanc,.shortName="Panc",.longName="All voices off (MIDI panic)",.values={""}},
			/*9*/ {.type=ptCust,.number=cnLBas,.shortName="LBas",.longName="Load basic preset",.values={""}},
		},
	},
	/* LFO1 page (D) */
	{
		{
			/* 1st row of pots */
			{.type=ptCont,.number=cpLFOFreq,.shortName="1Frq",.longName="LFO1 Frequency"},
			{.type=ptCont,.number=cpLFOAmt,.shortName="1Amt",.longName="LFO1 Amount (base)"},
			{.type=ptStep,.number=spLFOShape,.shortName="1Wav",.longName="LFO1 Waveform",.values={"Sqr ","Tri ","Rand","Sine","Nois","Saw ","RSaw"}},
			{.type=ptStep,.number=spLFOTargets,.shortName="1Tgt",.longName="LFO1 Osc Target",.values={"None","OscA","OscB","Both"}},
			{.type=ptCont,.number=cpModDelay,.shortName="MDly",.longName="Modulation Delay"},
			/* 2nd row of pots */
			{.type=ptCont,.number=cpLFOPitchAmt,.shortName="1Pit",.longName="Pitch LFO1 Amount"},
			{.type=ptCont,.number=cpLFOWModAmt,.shortName="1Wmo",.longName="WaveMod LFO1 Amount"},
			{.type=ptCont,.number=cpLFOFilAmt,.shortName="1Fil",.longName="Filter frequency LFO1 Amount"},
			{.type=ptCont,.number=cpLFOResAmt,.shortName="1Res",.longName="Filter resonance LFO1 Amount"},
			{.type=ptCont,.number=cpLFOAmpAmt,.shortName="1Amp",.longName="Amplifier LFO1 Amount"},
		},
		{
			/*0*/ {.type=ptCust,.number=cnNVal,.shortName="NVal",.longName="Numerically set last potentiometer value"},
			/*1*/ {.type=ptStep,.number=spModwheelRange,.shortName="MRng",.longName="Modwheel Range",.values={"Min ","Low ","High","Full"}},
			/*2*/ {.type=ptStep,.number=spBenderRange,.shortName="BRng",.longName="Bender Range",.values={"3rd ","5th ","Oct "}},
			/*3*/ {.type=ptStep,.number=spPressureRange,.shortName="PRng",.longName="Pressure Range",.values={"Min ","Low ","High","Full"}},
			/*4*/ {.type=ptStep,.number=spModwheelTarget,.shortName="MTgt",.longName="Modwheel Target",.values={"LFO1","LFO2"}},
			/*5*/ {.type=ptStep,.number=spBenderTarget,.shortName="BTgt",.longName="Bender Target",.values={"None","Pit ","Fil ","Vol ","XOvr"}},
			/*6*/ {.type=ptStep,.number=spPressureTarget,.shortName="PTgt",.longName="Pressure Target",.values={"None","Pit ","Fil ","Vol ","XOvr","LFO1","LFO2"}},
			/*7*/ {.type=ptCust,.number=cnTrspM,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
			/*8*/ {.type=ptCust,.number=cnPanc,.shortName="Panc",.longName="All voices off (MIDI panic)",.values={""}},
			/*9*/ {.type=ptCust,.number=cnLBas,.shortName="LBas",.longName="Load basic preset",.values={""}},
		},
	},
	/* LFO2 page (D) */
	{
		{
			/* 1st row of pots */
			{.type=ptCont,.number=cpLFO2Freq,.shortName="2Frq",.longName="LFO2 Frequency"},
			{.type=ptCont,.number=cpLFO2Amt,.shortName="2Amt",.longName="LFO2 Amount (base)"},
			{.type=ptStep,.number=spLFO2Shape,.shortName="2Wav",.longName="LFO2 Waveform",.values={"Sqr ","Tri ","Rand","Sine","Nois","Saw ","RSaw"}},
			{.type=ptStep,.number=spLFO2Targets,.shortName="2Tgt",.longName="LFO2 Osc Target",.values={"None","OscA","OscB","Both"}},
			{.type=ptCont,.number=cpModDelay,.shortName="MDly",.longName="Modulation Delay"},
			/* 2nd row of pots */
			{.type=ptCont,.number=cpLFO2PitchAmt,.shortName="2Pit",.longName="Pitch LFO2 Amount"},
			{.type=ptCont,.number=cpLFO2WModAmt,.shortName="2Wmo",.longName="WaveMod LFO2 Amount"},
			{.type=ptCont,.number=cpLFO2FilAmt,.shortName="2Fil",.longName="Filter frequency LFO2 Amount"},
			{.type=ptCont,.number=cpLFO2ResAmt,.shortName="2Res",.longName="Filter resonance LFO2 Amount"},
			{.type=ptCont,.number=cpLFO2AmpAmt,.shortName="2Amp",.longName="Amplifier LFO2 Amount"},
		},
		{
			/*0*/ {.type=ptCust,.number=cnNVal,.shortName="NVal",.longName="Numerically set last potentiometer value"},
			/*1*/ {.type=ptStep,.number=spModwheelRange,.shortName="MRng",.longName="Modwheel Range",.values={"Min ","Low ","High","Full"}},
			/*2*/ {.type=ptStep,.number=spBenderRange,.shortName="BRng",.longName="Bender Range",.values={"3rd ","5th ","Oct "}},
			/*3*/ {.type=ptStep,.number=spPressureRange,.shortName="PRng",.longName="Pressure Range",.values={"Min ","Low ","High","Full"}},
			/*4*/ {.type=ptStep,.number=spModwheelTarget,.shortName="MTgt",.longName="Modwheel Target",.values={"LFO1","LFO2"}},
			/*5*/ {.type=ptStep,.number=spBenderTarget,.shortName="BTgt",.longName="Bender Target",.values={"None","Pit ","Fil ","Vol ","XOvr"}},
			/*6*/ {.type=ptStep,.number=spPressureTarget,.shortName="PTgt",.longName="Pressure Target",.values={"None","Pit ","Fil ","Vol ","XOvr","LFO1","LFO2"}},
			/*7*/ {.type=ptCust,.number=cnTrspM,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
			/*8*/ {.type=ptCust,.number=cnPanc,.shortName="Panc",.longName="All voices off (MIDI panic)",.values={""}},
			/*9*/ {.type=ptCust,.number=cnLBas,.shortName="LBas",.longName="Load basic preset",.values={""}},
		},
	},
	/* Arpeggiator page (#) */
	{
		{
			/* 1st row of pots */
			{.type=ptCust,.number=cnClk,.shortName="Clk ",.longName="Seq/Arp Clock",.custPotMul=UINT16_MAX+1,.custPotAdd=0},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptCust,.number=cnSync,.shortName="Sync",.longName="Sync mode",.values={"Int ","MIDI"},.custPotMul=2,.custPotAdd=0},
			/* 2nd row of pots */
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptCust,.number=cnTrspV,.shortName="Trsp",.longName="Transpose (hit 7 then a note to change)",.custPotMul=49,.custPotAdd=-24},
		},
		{
			/*0*/ {.type=ptCust,.number=cnNVal,.shortName="NVal",.longName="Numerically set last potentiometer value"},
			/*1*/ {.type=ptCust,.number=cnAMod,.shortName="AMod",.longName="Arp Mode",.values={"Off ","UpDn","Rand","Asgn"}},
			/*2*/ {.type=ptCust,.number=cnAHld,.shortName="AHld",.longName="Arp Hold",.values={"Off ","On "}},
			/*3*/ {.type=ptNone},
			/*4*/ {.type=ptNone},
			/*5*/ {.type=ptNone},
			/*6*/ {.type=ptNone},
			/*7*/ {.type=ptCust,.number=cnTrspM,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
			/*8*/ {.type=ptCust,.number=cnPanc,.shortName="Panc",.longName="All voices off (MIDI panic)",.values={""}},
			/*9*/ {.type=ptCust,.number=cnLBas,.shortName="LBas",.longName="Load basic preset",.values={""}},
		},
	},
	/* Sequencer page (#) */
	{
		{
			/* 1st row of pots */
			{.type=ptCust,.number=cnClk,.shortName="Clk ",.longName="Seq/Arp Clock",.custPotMul=UINT16_MAX+1,.custPotAdd=0},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptCust,.number=cnSync,.shortName="Sync",.longName="Sync mode",.values={"Int ","MIDI"},.custPotMul=2,.custPotAdd=0},
			/* 2nd row of pots */
			{.type=ptCust,.number=cnSBnk,.shortName="SBnk",.longName="Sequencer memory Bank",.custPotMul=20,.custPotAdd=0},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptCust,.number=cnTrspV,.shortName="Trsp",.longName="Transpose (hit 7 then a note to change)",.custPotMul=49,.custPotAdd=-24},
		},
		{
			/*0*/ {.type=ptCust,.number=cnNVal,.shortName="NVal",.longName="Numerically set last potentiometer value"},
			/*1*/ {.type=ptCust,.number=cnAPly,.shortName="APly",.longName="Seq A Play/stop",.values={"Stop","Wait","Play","Rec "}},
			/*2*/ {.type=ptCust,.number=cnBPly,.shortName="BPly",.longName="Seq B Play/stop",.values={"Stop","Wait","Play","Rec "}},
			/*3*/ {.type=ptCust,.number=cnSRec,.shortName="SRec",.longName="Seq record",.values={"Off ","SeqA","SeqB"}},
			/*4*/ {.type=ptCust,.number=cnTiRe,.shortName="TiRe",.longName="Add Tie/Rest"},
			/*5*/ {.type=ptCust,.number=cnBack,.shortName="Back",.longName="Back one step"},
			/*6*/ {.type=ptCust,.number=cnClr,.shortName="Clr ",.longName="Clear sequence"},
			/*7*/ {.type=ptCust,.number=cnTrspM,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
			/*8*/ {.type=ptCust,.number=cnPanc,.shortName="Panc",.longName="All voices off (MIDI panic)",.values={""}},
			/*9*/ {.type=ptCust,.number=cnLBas,.shortName="LBas",.longName="Load basic preset",.values={""}},
		},
	},
	/* Miscellaneous page (*) */
	{
		{
			/* 1st row of pots */
			{.type=ptCust,.number=cnPrHu,.shortName="PrHu",.longName="Preset number hundreds",.custPotMul=10,.custPotAdd=0},
			{.type=ptCust,.number=cnPrTe,.shortName="PrTe",.longName="Preset number tens",.custPotMul=10,.custPotAdd=0},
			{.type=ptCust,.number=cnPrUn,.shortName="PrUn",.longName="Preset number units",.custPotMul=10,.custPotAdd=0},
			{.type=ptStep,.number=spPresetType,.shortName="PrTy",.longName="Preset type",.values={"Othr","Perc","Bass","Pad ","Keys","Stab","Lead","Arpg"}},
			{.type=ptStep,.number=spPresetStyle,.shortName="PrSt",.longName="Preset style",.values={"Othr","Neut","Clen","Real","Slky","Raw ","Hevy","Krch"}},
			/* 2nd row of pots */
			{.type=ptCust,.number=cnMidC,.shortName="MidC",.longName="Midi Channel",.custPotMul=17,.custPotAdd=0},
			{.type=ptNone},
			{.type=ptNone},
			{.type=ptCust,.number=cnCtst,.shortName="Ctst",.longName="LCD contrast",.custPotMul=UI_MAX_LCD_CONTRAST+1,.custPotAdd=0},
			{.type=ptCust,.number=cnUsbM,.shortName="UsbM",.longName="USB Mode",.values={"None","MIDI","Disk"},.custPotMul=3,.custPotAdd=0},
		},
		{
			/*0*/ {.type=ptCust,.number=cnNPrs,.shortName="NPrs",.longName="Numerically set preset number"},
			/*1*/ {.type=ptCust,.number=cnLoad,.shortName="Load",.longName="Load preset"},
			/*2*/ {.type=ptNone},
			/*3*/ {.type=ptCust,.number=cnSave,.shortName="Save",.longName="Save preset"},
			/*4*/ {.type=ptCust,.number=cnLPrv,.shortName="LPrv",.longName="Load previous preset",.values={""}},
			/*5*/ {.type=ptCust,.number=cnLNxt,.shortName="LNxt",.longName="Load next preset",.values={""}},
			/*6*/ {.type=ptCust,.number=cnTune,.shortName="Tune",.longName="Tune filters",.values={""}},
			/*7*/ {.type=ptCust,.number=cnTrspM,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
			/*8*/ {.type=ptCust,.number=cnPanc,.shortName="Panc",.longName="All voices off (MIDI panic)",.values={""}},
			/*9*/ {.type=ptCust,.number=cnLBas,.shortName="LBas",.longName="Load basic preset",.values={""}},
		},
	},
};

static struct
{
	enum uiPage_e activePage;
	int8_t activeSource;
	int8_t sourceChanges,prevSourceChanges;
	uint32_t settingsModifiedTimeout;
	uint32_t activeSourceTimeout;
	uint32_t slowUpdateTimeout;
	int16_t slowUpdateTimeoutNumber;
	int8_t pendingScreenClear;
	int8_t lastInputPot;
	
	int8_t kpInputDecade,kpInputPot;
	uint16_t kpInputValue;
	
	uint32_t potsAcquired;
	int32_t potsPrevValue[SCAN_POT_COUNT];

	int8_t presetModified;
	int8_t usbMSC;
	
	int8_t seqRecordingTrack;
	int8_t isTransposing;
	int32_t transpose;
	
	struct hd44780_data lcd1, lcd2;
} ui;

struct deadband_s {
	uint16_t middle;
	uint16_t guard;
	uint16_t deadband;
	uint32_t precalcLow;
	uint32_t precalcHigh;
};

struct deadband_s panelDeadband = { HALF_RANGE, SCAN_ADC_QUANTUM, PANEL_DEADBAND };

// Precalculate factor for dead band scaling to avoid time consuming
// division operation.
// so instead of doing foo*=32768; foo/=factor; we precalculate
// precalc=32768<<16/factor, and do foo*=precalc; foo>>=16; runtime.
static void precalcDeadband(struct deadband_s *d)
{
	uint16_t middleLow=d->middle-d->deadband;
	uint16_t middleHigh=d->middle+d->deadband;

	d->precalcLow=HALF_RANGE_L/(middleLow-d->guard);
	d->precalcHigh=HALF_RANGE_L/(FULL_RANGE-d->guard-middleHigh);
}

static inline uint16_t addDeadband(uint16_t value, struct deadband_s *d)
{
	uint16_t middleLow=d->middle-d->deadband;
	uint16_t middleHigh=d->middle+d->deadband;
	uint32_t amt;

	if(value>FULL_RANGE-d->guard)
		return FULL_RANGE;
	if(value<d->guard)
		return 0;

	amt=value;

	if(value<middleLow)
	{
		amt-=d->guard;
		amt*=d->precalcLow; // result is 65536 too big now
	}
	else if(value>middleHigh)
	{
		amt-=middleHigh;
		amt*=d->precalcHigh; // result is 65536 too big now
		amt+=HALF_RANGE_L;
	}
	else // in deadband
	{
		return HALF_RANGE;
	}
	
	// result of our calculations will be 0..UINT16_MAX<<16
	return amt>>16;
}

static int sendChar(int lcd, int ch)
{
	if(lcd==2)
		hd44780_driver.write(&ui.lcd2,ch);	
	else
		hd44780_driver.write(&ui.lcd1,ch);	
	return -1;
}

static int putc_lcd2(int ch)
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


static void setLcdContrast(uint8_t contrast)
{
	DAC_UpdateValue(0,(UI_MAX_LCD_CONTRAST-MIN(contrast,UI_MAX_LCD_CONTRAST))*400/UI_MAX_LCD_CONTRAST);
}

static void drawPresetModified(int lcd)
{
	int8_t i;
	static int8_t old_pm=INT8_MIN;

	if(ui.presetModified!=old_pm)
	{
		if(ui.presetModified)
		{
			sendChar(lcd, 0b00000);
			sendChar(lcd, 0b00110);
			sendChar(lcd, 0b00101);
			sendChar(lcd, 0b00101);
			sendChar(lcd, 0b00110);
			sendChar(lcd, 0b00100);
			sendChar(lcd, 0b00100);
			sendChar(lcd, 0b00000);

			sendChar(lcd, 0b00000);
			sendChar(lcd, 0b10100);
			sendChar(lcd, 0b11100);
			sendChar(lcd, 0b10100);
			sendChar(lcd, 0b10100);
			sendChar(lcd, 0b10100);
			sendChar(lcd, 0b10100);
			sendChar(lcd, 0b00000);
		}
		else
		{
			for(i=0;i<4;++i)
			{
				sendChar(lcd, 0b00001);
				sendChar(lcd, 0b00000);
			}

			for(i=0;i<4;++i)
			{
				sendChar(lcd, 0b10000);
				sendChar(lcd, 0b00000);
			}
		}
	
		old_pm=ui.presetModified;
	}
}

static void drawVisualEnv(int lcd, int8_t voicePair)
{
	int8_t i,ve,ve2;
	uint32_t veb;
	static int8_t old_ve[SYNTH_VOICE_COUNT]={INT8_MIN,INT8_MIN,INT8_MIN,INT8_MIN,INT8_MIN,INT8_MIN};
	
	ve=synth_getVisualEnvelope(voicePair)>>11;
	ve2=synth_getVisualEnvelope(voicePair+1)>>11;
	if(ve!=old_ve[voicePair] || ve2!=old_ve[voicePair+1])
	{
		veb=0;
		for(i=0;i<32;++i)
			if(ve>=i)
				veb|=(1<<i);

		sendChar(lcd, 0b00001 | ((veb>>27) & 0x1e));
		sendChar(lcd, 0b00000 | ((veb>>23) & 0x1e));
		sendChar(lcd, 0b00001 | ((veb>>16) & 0x1e));
		sendChar(lcd, 0b00000 | ((veb>>15) & 0x1e));
		sendChar(lcd, 0b00001 | ((veb>>11) & 0x1e));
		sendChar(lcd, 0b00000 | ((veb>>7) & 0x1e));
		sendChar(lcd, 0b00001 | ((veb>>3) & 0x1e));
		sendChar(lcd, 0b00000 | ((veb<<1) & 0x1e));

		veb=0;
		for(i=0;i<32;++i)
			if(ve2>=i)
				veb|=(1<<i);

		sendChar(lcd, 0b10000 | ((veb>>28) & 0xf));
		sendChar(lcd, 0b00000 | ((veb>>24) & 0xf));
		sendChar(lcd, 0b10000 | ((veb>>20) & 0xf));
		sendChar(lcd, 0b00000 | ((veb>>16) & 0xf));
		sendChar(lcd, 0b10000 | ((veb>>12) & 0xf));
		sendChar(lcd, 0b00000 | ((veb>>8) & 0xf));
		sendChar(lcd, 0b10000 | ((veb>>4) & 0xf));
		sendChar(lcd, 0b00000 | ((veb) & 0xf));
		
		old_ve[voicePair]=ve;
		old_ve[voicePair+1]=ve2;
	}
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

static char * getDisplayValue(int8_t source, int32_t * valueOut) // source: keypad (kb0..kbSharp) / (-1..-10)
{
	static char dv[10]={0};
	const struct uiParam_s * prm;
	int8_t potnum;
	int32_t valCount;
	int32_t v=INT32_MIN;

	dv[0]=0;
	if(valueOut)
		*valueOut=v;
	
	potnum=-source-1;
	prm=&uiParameters[ui.activePage][source<0?0:1][source<0?potnum:source];
	
	switch(prm->type)
	{
		case ptCont:
			v=currentPreset.continuousParameters[prm->number];
			
			if(continuousParametersZeroCentered[prm->number])
			{
				sprintf(dv,"%4d",((v+INT16_MIN)*1000)>>16);
			}
			else
			{
				sprintf(dv,"%4d",(v*1000)>>16);
			}
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
				case cnNone:
					v=0;
					break;
				case cnAMod:
					v=arp_getMode();
					break;
				case cnAHld:
					v=arp_getHold();
					break;
				case cnPrUn:
					v=(settings.presetNumber+1000)%10;
					break;
				case cnLoad:
				case cnSave:
					v=settings.presetNumber;
					break;
				case cnMidC:
					v=settings.midiReceiveChannel+1;
					if(!v)
						strcpy(dv,"Omni");
					break;
				case cnTune:
					v=0;
					break;
				case cnPrTe:
					v=((settings.presetNumber+1000)/10)%10;
					break;
				case cnSync:
					v=settings.syncMode;
					break;
				case cnAPly:
				case cnBPly:
					v=seq_getMode((prm->number==cnBPly)?1:0);
					break;
				case cnSRec:
					v=ui.seqRecordingTrack+1;
					break;
				case cnBack:
				case cnTiRe:
				case cnClr:
					v=0;
					if(ui.seqRecordingTrack>=0)
						v=seq_getStepCount(ui.seqRecordingTrack);
					break;
				case cnTrspM:
					v=ui.isTransposing;
					break;
				case cnTrspV:
					v=ui.transpose;
					break;
				case cnSBnk:
					v=settings.sequencerBank;
					break;
				case cnClk:
					if(settings.syncMode==smMIDI)
					{
						v=clock_getSpeed();
						if(v==UINT16_MAX)
							v=0;
					}
					else
					{
						v=((int)settings.seqArpClock*1000)>>16;
					}
					break;
				case cnXoCp:
					v=currentPreset.steppedParameters[spXOvrBank_Unsaved]*100;
					v+=currentPreset.steppedParameters[spXOvrWave_Unsaved]%100;
					sprintf(dv,"%04d",v);
					break;
				case cnLPrv:
				case cnLNxt:
				case cnPanc:
				case cnLBas:
				case cnPack:
					v=0;
					break;
				case cnPrHu:
					v=((settings.presetNumber+1000)/100)%10;
					break;
				case cnNPrs:
				case cnNVal:
					v=ui.kpInputValue;
					sprintf(dv,"%03d ",v);
					for(int i=0;i<=ui.kpInputDecade;++i)
						dv[2-i]='_';				
					break;
				case cnUsbM:
					v=ui.usbMSC?umMSC:(settings.usbMIDI?umMIDI:umPowerOnly);
					break;
				case cnCtst:
					v=settings.lcdContrast;
					break;
				}
			}

			if(!dv[0])
			{
				if(v>=0 && v<valCount)
					strcpy(dv,prm->values[v]);
				else
					sprintf(dv,"%4d",v);
			}
			break;
		default:
			strcpy(dv,"    ");
	}

	if(valueOut)
		*valueOut=v;

	return dv;
}

static char * getDisplayFulltext(int8_t source) // source: keypad (kb0..kbSharp) / (-1..-10)
{
	static char dv[41];
	const struct uiParam_s * prm;
	int8_t potnum;
	
	dv[0]=0;	
	potnum=-source-1;
	prm=&uiParameters[ui.activePage][source<0?0:1][source<0?potnum:source];
	
	if(prm->type==ptCont)
	{
		int32_t v=currentPreset.continuousParameters[prm->number];
		
		if(continuousParametersZeroCentered[prm->number])
		{
			v+=INT16_MIN;

			// nicer sampling
			if (v>=0)
				v+=UINT16_MAX/80;
			else if (v<0)
				v-=UINT16_MAX/80;
			
			// scale
			v=v*40>>16;

			// zero centered bargraph			
			for(int i=0;i<40;++i)
				if (v>=0)
				{
					dv[i]=((i>=20) && (i-20)<v) ? '\xff' : ' ';
				}
				else
				{
					dv[i]=((i<=20) && (i-20)>v) ? '\xff' : ' ';
				}
			
			// center
			dv[20]='|';
		}
		else
		{
			// nicer sampling
			v+=UINT16_MAX/80;
			
			// scale
			v=v*40>>16;

			// regular bargraph			
			for(int i=0;i<40;++i)
				dv[i]=(i<v) ? '\xff' : ' ';
		}
	}
	if (prm->type==ptStep && prm->number==spABank_Unsaved)
	{
		strcpy(dv,currentPreset.oscBank[0]);
	}
	else if (prm->type==ptStep && prm->number==spBBank_Unsaved)
	{
		strcpy(dv,currentPreset.oscBank[1]);
	}
	else if (prm->type==ptStep && prm->number==spXOvrBank_Unsaved)
	{
		strcpy(dv,currentPreset.oscBank[2]);
	}
	else if (prm->type==ptStep && prm->number==spAWave_Unsaved)
	{
		strcpy(dv,currentPreset.oscWave[0]);
	}
	else if (prm->type==ptStep && prm->number==spBWave_Unsaved)
	{
		strcpy(dv,currentPreset.oscWave[1]);
	}
	else if (prm->type==ptStep && prm->number==spXOvrWave_Unsaved)
	{
		strcpy(dv,currentPreset.oscWave[2]);
	}
	else if (prm->type==ptCust && prm->number==cnNVal)
	{
		if(ui.kpInputPot>=0)
			strcpy(dv,uiParameters[ui.activePage][0][ui.kpInputPot].longName);
	}
	else
	{
		char * selected;
		int32_t valCount;

		selected = getDisplayValue(source, NULL);
		valCount=0;
		while(valCount<8 && prm->values[valCount]!=NULL)
		{
			strcat(dv, (strcmp(selected, "") && !strcmp(selected, prm->values[valCount])) ? "\x7e"  : " ");
			strcat(dv, prm->values[valCount]);
			++valCount;
		}
	}

	// always 40chars
	for(int i=strlen(dv);i<40;++i) dv[i]=' ';
	dv[40]=0;

	return dv;
}

static void scanEvent(int8_t source, uint16_t * forcedValue) // source: keypad (kb0..kbSharp) / (-1..-10)
{
	int32_t data,valCount;
	int32_t potSetting;
	int8_t potnum,change,settingsModified;
	const struct uiParam_s * prm;

//	rprintf(0,"handleUserInput %d\n",source);
	
	// page change
	if(source>=kbA)
	{
		switch(source)
		{
			case kbA: 
				ui.activePage=(ui.activePage==upOscs)?upWMod:upOscs;
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
		
		//pots need to be reacquired
		ui.potsAcquired=0;
		for(int i=0;i<SCAN_POT_COUNT;++i)
			ui.potsPrevValue[i]=INT32_MIN;

		// cancel ongoing changes
		ui.lastInputPot=-1;
		ui.kpInputPot=-1;
		ui.kpInputDecade=-1;
		ui.activeSourceTimeout=0;
		scan_resetPotLocking();

		ui.pendingScreenClear=1;
		
		return;
	}
	
	// keypad value input mode	
	if (ui.kpInputDecade>=0)
	{
		if(source>=kb0 && source<=kb9)
		{
			uint16_t decade=1;
			for(int i=0;i<ui.kpInputDecade;++i)
				decade*=10;

			ui.kpInputValue+=(source-kb0)*decade;
			--ui.kpInputDecade;

			if(ui.kpInputDecade<0) // end of input?
			{
				// back to regular page display
				ui.activeSourceTimeout=currentTick+ACTIVE_SOURCE_TIMEOUT;
				ui.pendingScreenClear=1;

				if(ui.kpInputPot>=0)
				{
					scanEvent(-ui.kpInputPot-1,&ui.kpInputValue);
				}
				else
				{
					// preset number
					settings.presetNumber=ui.kpInputValue;
				}
			}
		}

		return;
	}

	potnum=-source-1;
	prm=&uiParameters[ui.activePage][source<0?0:1][source<0?potnum:source];
	
	if(source<0)
		ui.lastInputPot=potnum;

	// nothing to do -> return
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
	
	// get stepped parameter value count
	valCount=0;
	if(prm->type==ptStep)
	{
		while(valCount<8 && prm->values[valCount]!=NULL)
			++valCount;
		
		if(!valCount)
		{
			switch(prm->number)
			{
				case spABank_Unsaved:
				case spBBank_Unsaved:
				case spXOvrBank_Unsaved:
					synth_refreshBankNames(1);
					valCount=synth_getBankCount();
					break;
				case spAWave_Unsaved:
					synth_refreshCurWaveNames(0,1);
					valCount=synth_getCurWaveCount();
					break;
				case spBWave_Unsaved:
					synth_refreshCurWaveNames(1,1);
					valCount=synth_getCurWaveCount();
					break;
				case spXOvrWave_Unsaved:
					synth_refreshCurWaveNames(2,1);
					valCount=synth_getCurWaveCount();
					break;
				default:
					valCount=steppedParametersSteps[prm->number];
			}
		}
	}
	
	if(forcedValue)
	{
		switch(prm->type)
		{
			case ptCont:
				potSetting=((*forcedValue)*UINT16_MAX)/999;
				break;
			case ptStep:
				potSetting=MIN(*forcedValue,valCount-1);
				break;
			case ptCust:
				potSetting=MIN(((*forcedValue)*UINT16_MAX)/999,prm->custPotMul+prm->custPotAdd-1);
				break;
			default:
				/* nothing */;
		}
	}
	else
	{
		int32_t potQuantum;
		
		// transform potentiometer value into setting value
		potSetting=0;
		potQuantum=0;
		if(source<0)
		{
			data=scan_getPotValue(potnum);
			switch(prm->type)
			{
				case ptCont:
					potSetting=data;

					if(continuousParametersZeroCentered[prm->number])
						potSetting=addDeadband(potSetting, &panelDeadband);

					potQuantum=SCAN_ADC_QUANTUM;
					break;
				case ptStep:
					potSetting=(valCount*data)>>16;
					break;
				case ptCust:
					potSetting=((prm->custPotMul*data)>>16)+prm->custPotAdd;
					break;
				default:
					/* nothing */;
			}
		}

		// lock potentiometers until prev value is reacquired
		if(source<0)
		{
			int32_t value,prevValue;
			getDisplayValue(source,&value);

			prevValue=(ui.potsPrevValue[potnum]==INT32_MIN)?potSetting:ui.potsPrevValue[potnum];

			if(value>=MIN(prevValue,potSetting) && value<=MAX(prevValue,potSetting)+potQuantum)
				ui.potsAcquired|=1<<potnum;

			ui.potsPrevValue[potnum]=potSetting;

			if(((ui.potsAcquired>>potnum)&1)==0)
				return;
		}
	}

	change=0;
	settingsModified=0;
	
	// store new setting
	switch(prm->type)
	{
	case ptCont:
		data=potSetting;
		change=currentPreset.continuousParameters[prm->number]!=data;
		currentPreset.continuousParameters[prm->number]=data;
		break;
	case ptStep:
		if(source<0)
			data=potSetting;
		else
			data=(currentPreset.steppedParameters[prm->number]+1)%valCount;

		change=currentPreset.steppedParameters[prm->number]!=data;
		currentPreset.steppedParameters[prm->number]=data;
		
		// special cases
		if(change)
		{
			if(prm->number==spABank_Unsaved || prm->number==spBBank_Unsaved || prm->number==spXOvrBank_Unsaved ||
					prm->number==spAWave_Unsaved || prm->number==spBWave_Unsaved || prm->number==spXOvrWave_Unsaved)
			{
				switch(prm->number)
				{
					case spABank_Unsaved:
						synth_getBankName(data,currentPreset.oscBank[0]);
						break;
					case spBBank_Unsaved:
						synth_getBankName(data,currentPreset.oscBank[1]);
						break;
					case spXOvrBank_Unsaved:
						synth_getBankName(data,currentPreset.oscBank[2]);
						break;
					case spAWave_Unsaved:
						synth_getWaveName(data,currentPreset.oscWave[0]);
						break;
					case spBWave_Unsaved:
						synth_getWaveName(data,currentPreset.oscWave[1]);
						break;
					case spXOvrWave_Unsaved:
						synth_getWaveName(data,currentPreset.oscWave[2]);
						break;
				}
				
				// waveform changes
				ui.slowUpdateTimeout=currentTick+SLOW_UPDATE_TIMEOUT;
				ui.slowUpdateTimeoutNumber=prm->number;
			}
		}
		break;
	case ptCust:
		switch(prm->number)
		{
			case cnAMod:
				arp_setMode((arp_getMode()+1)%4,arp_getHold());
				break;
			case cnAHld:
				arp_setMode(arp_getMode(),!arp_getHold());
				break;
			case cnPrUn:
				settings.presetNumber=settings.presetNumber-(settings.presetNumber%10)+potSetting;
				break;
			case cnLoad:
			case cnSave:
			case cnTune:
				ui.slowUpdateTimeout=currentTick+SLOW_UPDATE_TIMEOUT;
				ui.slowUpdateTimeoutNumber=prm->number+0x80;
				break;
			case cnMidC:
				settings.midiReceiveChannel=potSetting-1;
				settingsModified=1;
				break;
			case cnPrTe:
				settings.presetNumber=settings.presetNumber-(((settings.presetNumber/10)%10)*10)+potSetting*10;
				break;
			case cnSync:
				settings.syncMode=potSetting;
				settingsModified=1;
				break;
			case cnAPly:
			case cnBPly:
				data=(prm->number==cnBPly)?1:0;
				seq_setMode(data,seq_getMode(data)==smPlaying?smOff:smPlaying);
				if(data==ui.seqRecordingTrack)
					ui.seqRecordingTrack=-1;
				break;
			case cnSRec:
				ui.seqRecordingTrack=ui.seqRecordingTrack>=1?-1:ui.seqRecordingTrack+1;
				if(seq_getMode(0)==smRecording) seq_setMode(0,smOff);
				if(seq_getMode(1)==smRecording) seq_setMode(1,smOff);
				if(ui.seqRecordingTrack>=0)
					seq_setMode(ui.seqRecordingTrack,smRecording);
				break;
			case cnBack:
				if(ui.seqRecordingTrack>=0)
					seq_inputNote(SEQ_NOTE_UNDO,1);
				break;
			case cnTiRe:
				if(ui.seqRecordingTrack>=0)
					seq_inputNote(SEQ_NOTE_STEP,1);
				break;
			case cnClr:
				if(ui.seqRecordingTrack>=0)
					seq_inputNote(SEQ_NOTE_CLEAR,1);
				break;
			case cnTrspM:
				ui.isTransposing=(ui.isTransposing+1)%3;
				break;
			case cnTrspV:
				ui_setTranspose(potSetting);
				break;
			case cnSBnk:
				settings.sequencerBank=potSetting;
				settingsModified=1;
				break;				
			case cnClk:
				settings.seqArpClock=potSetting;
				settingsModified=1;
				break;
			case cnXoCp:
				currentPreset.steppedParameters[spXOvrBank_Unsaved]=currentPreset.steppedParameters[spABank_Unsaved];
				currentPreset.steppedParameters[spXOvrWave_Unsaved]=currentPreset.steppedParameters[spAWave_Unsaved];
				strcpy(currentPreset.oscBank[2], currentPreset.oscBank[0]);
				strcpy(currentPreset.oscWave[2], currentPreset.oscWave[0]);
				synth_refreshWaveforms(2);
				change=1;
				break;
			case cnLPrv:
			case cnLNxt:
				data=settings.presetNumber+((prm->number==cnLPrv)?-1:1);
				data=(data+1000)%1000;
				settings.presetNumber=data;

				ui.slowUpdateTimeout=0;
				ui.slowUpdateTimeoutNumber=0x80+cnLoad;
				break;
			case cnPanc:
				assigner_panicOff();
				synth_refreshFullState();
				break;
			case cnLBas:
				preset_loadDefault(1);
				synth_refreshWaveforms(0);
				synth_refreshWaveforms(1);
				synth_refreshWaveforms(2);
				change=1;
				break;
			case cnPack:
				preset_packAndRemoveDuplicates();
				break;
			case cnPrHu:
				settings.presetNumber=settings.presetNumber-(((settings.presetNumber/100)%10)*100)+potSetting*100;
				break;
			case cnNPrs:
				ui.kpInputValue=0;
				ui.kpInputPot=-1;
				ui.kpInputDecade=2;
				ui.activeSourceTimeout=UINT32_MAX;
				break;
			case cnNVal:
				ui.kpInputValue=0;
				ui.kpInputDecade=-1;
				if(ui.lastInputPot>=0 && uiParameters[ui.activePage][0][ui.lastInputPot].type!=ptNone)
				{
					ui.kpInputPot=ui.lastInputPot;
					ui.kpInputDecade=2;
					ui.activeSourceTimeout=UINT32_MAX;
				}
				break;
			case cnUsbM:
				switch(potSetting)
				{
				case umMIDI:
					settings.usbMIDI=1;
					break;
				case umMSC:
					ui.usbMSC=1;
					break;
				default:
					settings.usbMIDI=0;
					ui.usbMSC=0;
				}
				ui.slowUpdateTimeout=currentTick+SLOW_UPDATE_TIMEOUT;
				ui.slowUpdateTimeoutNumber=prm->number+0x80;
				settingsModified=1;
				break;
			case cnCtst:
				settings.lcdContrast=potSetting;
				settingsModified=1;
				break;
		}
		break;
	default:
		/*nothing*/;
	}

	if(change)
	{
		ui.presetModified=change;
		synth_refreshFullState();
	}
	
	if(settingsModified)
	{
		ui.settingsModifiedTimeout=currentTick+ACTIVE_SOURCE_TIMEOUT;
		synth_refreshFullState();
	}
}

static void scanEventCallback(int8_t source)
{
	scanEvent(source,NULL);
}

static void diskModeScanEventCallback(int8_t source)
{
	if(source>=kb0)
		ui.usbMSC=0;
}

static int8_t usbMSCCallback(void)
{
	scan_setScanEventCallback(diskModeScanEventCallback);
	scan_update();
	scan_setScanEventCallback(scanEventCallback);
	
	return ui.usbMSC;
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

	// DAC for contrast
	
	DAC_Init(0);
	setLcdContrast(UI_DEFAULT_LCD_CONTRAST);
		
	// welcome message

	sendString(1,"GliGli's OverCycler");
	setPos(1,0,1);
	sendString(1,"Build " __DATE__ " " __TIME__);
	rprintf(1,"Sampling at %d Hz", SYNTH_MASTER_CLOCK/DACSPI_TICK_RATE);
	setPos(2,0,1);

	ui.activePage=upNone;
	ui.activeSource=INT8_MAX;
	ui.pendingScreenClear=1;
	ui.seqRecordingTrack=-1;
	ui.lastInputPot=-1;
	ui.kpInputDecade=-1;
	ui.kpInputPot=-1;
	
	precalcDeadband(&panelDeadband);
}

void ui_update(void)
{
	int i;
	static uint8_t frc=0;
	int8_t fsDisp;
	
	++frc;
	
	// slow updates (if needed)
	
	if(currentTick>ui.slowUpdateTimeout)
	{
		switch(ui.slowUpdateTimeoutNumber)
		{
			case spABank_Unsaved:
				synth_refreshCurWaveNames(0,1);
				break;
			case spBBank_Unsaved:
				synth_refreshCurWaveNames(1,1);
				break;
			case spXOvrBank_Unsaved:
				synth_refreshCurWaveNames(2,1);
				break;
			case spAWave_Unsaved:
				synth_refreshWaveforms(0);
				break;
			case spBWave_Unsaved:
				synth_refreshWaveforms(1);
				break;
			case spXOvrWave_Unsaved:
				synth_refreshWaveforms(2);
				break;
			case 0x80+cnSave:
				preset_saveCurrent(settings.presetNumber);
				settings_save();                
				ui_setPresetModified(0);	
				break;
			case 0x80+cnLoad:
				BLOCK_INT(1)
				{
					// temporarily silence voices
					for(int8_t v=0;v<SYNTH_VOICE_COUNT;++v)
						synth_refreshCV(v,cvAmp, 0);					
					
					settings_save();                
					if(!preset_loadCurrent(settings.presetNumber))
						preset_loadDefault(1);
					ui_setPresetModified(0);	

					synth_refreshWaveforms(0);
					synth_refreshWaveforms(1);
					synth_refreshWaveforms(2);
					synth_refreshFullState();
				}
				break;
			case 0x80+cnTune:
				setPos(2,0,1);
				tuner_tuneSynth();
				break;
			case 0x80+cnUsbM:
				if(ui.usbMSC)
				{
					setPos(2,0,1);
					sendString(2,"USB Disk mode, press any button to quit");
					usb_setMode(umMSC,usbMSCCallback);
					ui.pendingScreenClear=1;
				}
				usb_setMode(settings.usbMIDI?umMIDI:umPowerOnly,NULL);
				break;
		}
		
		ui.slowUpdateTimeout=UINT32_MAX;
	}

	// to store changes made to settings thru UI

	if(currentTick>ui.settingsModifiedTimeout)
	{
		settings_save();
		ui.settingsModifiedTimeout=UINT32_MAX;
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
			sendString(1,"GliGli's OverCycler                     ");
			sendString(1,"A: Oscs/WaveMod    B: Filter            ");
			sendString(2,"C: Amplifier       D: LFO1/LFO2         ");
			sendString(2,"*: Miscellaneous   #: Arp/Sequencer     ");
		}
		delay_ms(2);
	}
	else if(fsDisp) // fullscreen display
	{
		char * dv;
		int32_t v;
		int8_t noAcq,smaller,larger;
		
		if(ui.pendingScreenClear)
			sendString(1,getName(ui.activeSource,1));

		dv=getDisplayValue(ui.activeSource, &v);
				
		noAcq=ui.activeSource<0&&(((ui.potsAcquired>>(-ui.activeSource-1))&1)==0);
		smaller=noAcq&&ui.potsPrevValue[-ui.activeSource-1]<v;
		larger=noAcq&&ui.potsPrevValue[-ui.activeSource-1]>v;
		
		setPos(2,17,0);
		sendChar(2,(smaller) ? '\x7e' : ' ');
		sendString(2,dv);
		sendChar(2,(larger) ? '\x7f' : ' ');
		
		setPos(2,0,1);
		sendString(2,getDisplayFulltext(ui.activeSource));
	}
	else
	{
		ui.activeSource=INT8_MAX;

		 // buttons
		
		setPos(1,26,1);
		setPos(2,26,0);
		for(i=kb1;i<=kb6;++i)
		{
			int lcd=(i<=kb3)?1:2;
			sendString(lcd,getDisplayValue(i, NULL));
			if(i!=kb6 && i!=kb3)
				sendChar(lcd,' ');
		}
	
		if(ui.pendingScreenClear)
		{
			setPos(1,26,0);
			
			for(i=kb1;i<=kb3;++i)
			{
				sendString(1,getName(i,0));
				if(i<kb3)
					sendChar(1,' ');
			}

			setPos(2,26,1);
			for(i=kb4;i<=kb6;++i)
			{
				sendString(2,getName(i,0));
				if(i<kb6)
					sendChar(2,' ');
			}
		}
		
		// delimiter

			// CGRAM update ("preset modified", visual envelopes)
		
		hd44780_driver.write_cmd(&ui.lcd1,CMD_CGRAM_ADDR);
		drawPresetModified(1);
		hd44780_driver.write_cmd(&ui.lcd1,CMD_CGRAM_ADDR+16);
		drawVisualEnv(1,0);
		
		hd44780_driver.write_cmd(&ui.lcd2,CMD_CGRAM_ADDR);
		drawVisualEnv(2,2);
		hd44780_driver.write_cmd(&ui.lcd2,CMD_CGRAM_ADDR+16);
		drawVisualEnv(2,4);

			// actual "text"
		
		if(ui.pendingScreenClear)
		{
			setPos(1,24,0); sendChar(1,'\x08'); sendChar(1,'\x09');
			setPos(1,24,1); sendChar(1,'\x0a'); sendChar(1,'\x0b');
			setPos(2,24,0); sendChar(2,'\x08'); sendChar(2,'\x09');
			setPos(2,24,1); sendChar(2,'\x0a'); sendChar(2,'\x0b');
		}
		
		// pots

		setPos(1,0,1);
		hd44780_driver.home(&ui.lcd2);
		for(i=0;i<SCAN_POT_COUNT;++i)
		{
			int lcd=(i<SCAN_POT_COUNT/2)?1:2;
			sendString(lcd,getDisplayValue(-i-1, NULL));
			if(i!=SCAN_POT_COUNT-1 && i!=SCAN_POT_COUNT/2-1)
				sendChar(lcd,' ');
		}

		if(ui.pendingScreenClear)
		{
			hd44780_driver.home(&ui.lcd1);
			for(i=0;i<SCAN_POT_COUNT/2;++i)
			{
				sendString(1,getName(-i-1,0));
				if(i<SCAN_POT_COUNT/2-1)
					sendChar(1,' ');
			}

			setPos(2,0,1);
			for(i=SCAN_POT_COUNT/2;i<SCAN_POT_COUNT;++i)
			{
				sendString(2,getName(-i-1,0));
				if(i<SCAN_POT_COUNT-1)
					sendChar(2,' ');
			}
		}
	}

	ui.pendingScreenClear=0;
	ui.prevSourceChanges=ui.sourceChanges;
	ui.sourceChanges=0;
	
	// LCD contrast
	
	setLcdContrast(settings.lcdContrast);
	
	// events from scanner
	scan_setScanEventCallback(scanEventCallback);
}

