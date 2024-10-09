#ifndef UI_PAGES_H
#define UI_PAGES_H

#include "ui.h"
#include "storage.h"
#include "scan.h"
#include "seq.h"
#include "clock.h"

enum uiParamType_e
{
	ptNone=0,ptCont,ptStep,ptCust
};

enum uiPage_e
{
	upHelp,upOscs,upWMod,upFil,upAmp,upLFO1,upLFO2,upArp,upSeqPlay,upSeqRec,upMisc,upPresets,

	// /!\ this must stay last
	upCount
};

enum uiCustomParamNumber_e
{
	cnNone=0,cnAMod,cnAHld,cnLoad,cnSave,cnMidC,cnTune,cnSync,cnAPly,cnBPly,cnSRec,cnBack,cnTiRe,cnClr,
	cnTrspM,cnTrspV,cnSBnk,cnClk,cnAXoSw,cnBXoSw,cnLPrv,cnLNxt,cnPanc,cnLBas,cnNPrs,cnNVal,cnUsbM,cnCtst,
	cnWEnT,cnFEnT,cnAEnT,cnHelp,
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

const struct uiParam_s uiParameters[upCount][SCAN_POT_COUNT+(kbAsterisk-kbA+1)] = // [pages][pot/key num]
{
	/* Help page (startup) */
	{
		/* 1st row of pots */
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptNone},
		/* 2nd row of pots */
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptNone},
		/* buttons (A,B,C,D,#,*) */
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptCust,.number=cnTrspM,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
		{.type=ptNone},
	},
	/* Oscillators page (1) */
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
		/* buttons (A,B,C,D,#,*) */
		{.type=ptCust,.number=cnAXoSw,.shortName="AXoS",.longName="Swap A bank/wave with A Crossover"},
		{.type=ptCust,.number=cnBXoSw,.shortName="BXoS",.longName="Swap B bank/wave with B Crossover"},
		{.type=ptStep,.number=spChromaticPitch,.shortName="FrqM",.longName="Frequency Mode",.values={"Free","Semi","Oct "}},
		{.type=ptStep,.number=spOscSync,.shortName="Sync",.longName="Oscillator A to B Synchronization",.values={"Off ","On  "}},
		{.type=ptCust,.number=cnTrspM,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
		{.type=ptCust,.number=cnNVal,.shortName="NVal",.longName="Set last potentiometer digits"},
	},
	/* WaveMod page (2) */
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
		/* buttons (A,B,C,D,#,*) */
		{.type=ptStep,.number=spAWModType,.shortName="AWmT",.longName="Osc A WaveMod Type",.values={"None","Grit","Wdth","Freq","XOvr","Fold"}},
		{.type=ptStep,.number=spBWModType,.shortName="BWmT",.longName="Osc B WaveMod Type",.values={"None","Grit","Wdth","Freq","XOvr","Fold"}},
		{.type=ptCust,.number=cnWEnT,.shortName="WEnT",.longName="WaveMod Envelope Type",.values={"FExp","SExp","FLin","SLin"}},
		{.type=ptStep,.number=spWModEnvLoop,.shortName="WEnL",.longName="WaveMod Envelope Loop",.values={"Norm","Loop"}},
		{.type=ptCust,.number=cnTrspM,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
		{.type=ptCust,.number=cnNVal,.shortName="NVal",.longName="Set last potentiometer digits"},
	},
	/* Filter page (3) */
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
		/* buttons (A,B,C,D,#,*) */
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptCust,.number=cnFEnT,.shortName="FEnT",.longName="Filter Envelope Type",.values={"FExp","SExp","FLin","SLin"}},
		{.type=ptStep,.number=spFilEnvLoop,.shortName="FEnL",.longName="Filter Envelope Loop",.values={"Norm","Loop"}},
		{.type=ptCust,.number=cnTrspM,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
		{.type=ptCust,.number=cnNVal,.shortName="NVal",.longName="Set last potentiometer digits"},
	},
	/* Amplifier page (4) */
	{
		/* 1st row of pots */
		{.type=ptCont,.number=cpVCALevel,.shortName="VLvl",.longName="VCA Level"},
		{.type=ptCont,.number=cpGlide,.shortName="Glid",.longName="Glide amount"},
		{.type=ptCont,.number=cpUnisonDetune,.shortName="MDet",.longName="Master unison Detune"},
		{.type=ptCont,.number=cpMasterTune,.shortName="MTun",.longName="Master Tune"},
		{.type=ptStep,.number=spVoiceCount,.shortName="VCnt",.longName="Voice count",.values={"   1","   2","   3","   4","   5","   6"}},
		/* 2nd row of pots */
		{.type=ptCont,.number=cpAmpAtt,.shortName="AAtk",.longName="Amplifier Attack"},
		{.type=ptCont,.number=cpAmpDec,.shortName="ADec",.longName="Amplifier Decay"},
		{.type=ptCont,.number=cpAmpSus,.shortName="ASus",.longName="Amplifier Sustain"},
		{.type=ptCont,.number=cpAmpRel,.shortName="ARel",.longName="Amplifier Release"},
		{.type=ptCont,.number=cpAmpVelocity,.shortName="AVel",.longName="Amplifier Velocity"},
		/* buttons (A,B,C,D,#,*) */
		{.type=ptStep,.number=spUnison,.shortName="Unis",.longName="Unison",.values={"Off ","On  "}},
		{.type=ptStep,.number=spAssignerPriority,.shortName="Prio",.longName="Assigner Priority",.values={"Last","Low ","High"}},
		{.type=ptCust,.number=cnAEnT,.shortName="AEnT",.longName="Amplifier Envelope Type",.values={"FExp","SExp","FLin","SLin"}},
		{.type=ptStep,.number=spAmpEnvLoop,.shortName="AEnL",.longName="Amplifier Envelope Loop",.values={"Norm","Loop"}},
		{.type=ptCust,.number=cnTrspM,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
		{.type=ptCust,.number=cnNVal,.shortName="NVal",.longName="Set last potentiometer digits"},
	},
	/* LFO1 page (5) */
	{
		/* 1st row of pots */
		{.type=ptCont,.number=cpLFOFreq,.shortName="1Spd",.longName="LFO1 Speed (BPM)"},
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
		/* buttons (A,B,C,D,#,*) */
		{.type=ptStep,.number=spLFOSpeed,.shortName="1Spd",.longName="LFO1 Speed multiplier",.values={"  x1","  x2","  x4","  x8"}},
		{.type=ptStep,.number=spLFOTargets,.shortName="1Tgt",.longName="LFO1 Osc Target",.values={"None","OscA","OscB","Both"}},
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptCust,.number=cnTrspM,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
		{.type=ptCust,.number=cnNVal,.shortName="NVal",.longName="Set last potentiometer digits"},
	},
	/* LFO2 page (6) */
	{
		/* 1st row of pots */
		{.type=ptCont,.number=cpLFO2Freq,.shortName="2Spd",.longName="LFO2 Speed (BPM)"},
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
		/* buttons (A,B,C,D,#,*) */
		{.type=ptStep,.number=spLFO2Speed,.shortName="2Spd",.longName="LFO2 Speed multiplier",.values={"  x1","  x2","  x4","  x8"}},
		{.type=ptStep,.number=spLFO2Targets,.shortName="2Tgt",.longName="LFO2 Osc Target",.values={"None","OscA","OscB","Both"}},
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptCust,.number=cnTrspM,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
		{.type=ptCust,.number=cnNVal,.shortName="NVal",.longName="Set last potentiometer digits"},
	},
	/* Arpeggiator page (7) */
	{
		/* 1st row of pots */
		{.type=ptCust,.number=cnClk,.shortName="Clk ",.longName="Seq/Arp Clock (Int:BPM, MIDI:Divider)",.custPotMul=CLOCK_MAX_BPM+1,.custPotAdd=0},
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptNone},
		/* 2nd row of pots */
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptCust,.number=cnTrspV,.shortName="Trsp",.longName="Transpose (hit 7 then a note to change)",.custPotMul=49,.custPotAdd=-24},
		/* buttons (A,B,C,D,#,*) */
		{.type=ptCust,.number=cnAMod,.shortName="AMod",.longName="Arp Mode",.values={"Off ","UpDn","Rand","Asgn"}},
		{.type=ptCust,.number=cnAHld,.shortName="AHld",.longName="Arp Hold",.values={"Off ","On "}},
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptCust,.number=cnTrspM,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
		{.type=ptCust,.number=cnNVal,.shortName="NVal",.longName="Set last potentiometer digits"},
	},
	/* Sequencer page (8) */
	{
		/* 1st row of pots */
		{.type=ptCust,.number=cnClk,.shortName="Clk ",.longName="Seq/Arp Clock (Int:BPM, MIDI:Divider)",.custPotMul=CLOCK_MAX_BPM+1,.custPotAdd=0},
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptNone},
		/* 2nd row of pots */
		{.type=ptCust,.number=cnSBnk,.shortName="SBnk",.longName="Sequencer memory Bank",.custPotMul=SEQ_BANK_COUNT,.custPotAdd=0},
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptCust,.number=cnTrspV,.shortName="Trsp",.longName="Transpose (hit 7 then a note to change)",.custPotMul=49,.custPotAdd=-24},
		/* buttons (A,B,C,D,#,*) */
		{.type=ptCust,.number=cnAPly,.shortName="APly",.longName="Seq A Play/stop",.values={"Stop","Wait","Play","Rec "}},
		{.type=ptCust,.number=cnBPly,.shortName="BPly",.longName="Seq B Play/stop",.values={"Stop","Wait","Play","Rec "}},
		{.type=ptNone},
		{.type=ptCust,.number=cnSRec,.shortName="SRec",.longName="Seq record",.values={"Off ","SeqA","SeqB"}},
		{.type=ptCust,.number=cnTrspM,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
		{.type=ptCust,.number=cnNVal,.shortName="NVal",.longName="Set last potentiometer digits"},
	},
	/* Sequencer record page (8) */
	{
		/* 1st row of pots */
		{.type=ptCust,.number=cnClk,.shortName="Clk ",.longName="Seq/Arp Clock (Int:BPM, MIDI:Divider)",.custPotMul=CLOCK_MAX_BPM+1,.custPotAdd=0},
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptNone},
		/* 2nd row of pots */
		{.type=ptCust,.number=cnSBnk,.shortName="SBnk",.longName="Sequencer memory Bank",.custPotMul=20,.custPotAdd=0},
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptCust,.number=cnTrspV,.shortName="Trsp",.longName="Transpose (hit 7 then a note to change)",.custPotMul=49,.custPotAdd=-24},
		/* buttons (A,B,C,D,#,*) */
		{.type=ptCust,.number=cnTiRe,.shortName="TiRe",.longName="Add Tie/Rest"},
		{.type=ptCust,.number=cnBack,.shortName="Back",.longName="Back one step"},
		{.type=ptCust,.number=cnClr,.shortName="Clr ",.longName="Clear sequence"},
		{.type=ptCust,.number=cnSRec,.shortName="SRec",.longName="Seq record",.values={"Off ","SeqA","SeqB"}},
		{.type=ptCust,.number=cnTrspM,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
		{.type=ptCust,.number=cnNVal,.shortName="NVal",.longName="Set last potentiometer digits"},
	},
	/* Miscellaneous page (9) */
	{
		/* 1st row of pots */
		{.type=ptStep,.number=spModwheelRange,.shortName="MRng",.longName="Modwheel Range",.values={"Min ","Low ","High","Full"}},
		{.type=ptStep,.number=spBenderRange,.shortName="BRng",.longName="Bender Range",.values={"3rd ","5th ","Oct "}},
		{.type=ptStep,.number=spPressureRange,.shortName="PRng",.longName="Pressure Range",.values={"Min","Low","High","Full"}},
		{.type=ptCust,.number=cnMidC,.shortName="MidC",.longName="Midi Channel",.custPotMul=17,.custPotAdd=0},
		{.type=ptCust,.number=cnSync,.shortName="Sync",.longName="Sync mode",.values={"Int ","MIDI", "USB "},.custPotMul=3,.custPotAdd=0},
		/* 2nd row of pots */
		{.type=ptStep,.number=spModwheelTarget,.shortName="MTgt",.longName="Modwheel Target",.values={"LFO1","LFO2"}},
		{.type=ptStep,.number=spBenderTarget,.shortName="BTgt",.longName="Bender Target",.values={"None","Pit ","Fil ","Vol ","XOvr"}},
		{.type=ptStep,.number=spPressureTarget,.shortName="PTgt",.longName="Pressure Target",.values={"None","Pit ","Fil ","Vol ","XOvr","LFO1","LFO2"}},
		{.type=ptCust,.number=cnCtst,.shortName="Ctst",.longName="LCD contrast",.custPotMul=UI_MAX_LCD_CONTRAST+1,.custPotAdd=0},
		{.type=ptCust,.number=cnUsbM,.shortName="UsbM",.longName="USB Mode",.values={"None","Disk","MIDI"},.custPotMul=3,.custPotAdd=0},
		/* buttons (A,B,C,D,#,*) */
		{.type=ptCust,.number=cnLBas,.shortName="LBas",.longName="Load basic preset",.values={""}},
		{.type=ptCust,.number=cnPanc,.shortName="Panc",.longName="All voices off (MIDI panic)",.values={""}},
		{.type=ptCust,.number=cnHelp,.shortName="Help",.longName="Return to help page",.values={""}},
		{.type=ptCust,.number=cnTune,.shortName="Tune",.longName="Tune filters",.values={""}},
		{.type=ptCust,.number=cnTrspM,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
		{.type=ptCust,.number=cnNVal,.shortName="NVal",.longName="Set last potentiometer digits"},
	},
	/* Presets page (0) */
	{
		/* 1st row of pots */
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptNone},
		/* 2nd row of pots */
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptNone},
		{.type=ptStep,.number=spPresetType,.shortName="Type",.longName="Preset type",.values={"Othr","Bass","Pad","Strn","Brass","Keys","Lead","Arpg"}},
		{.type=ptStep,.number=spPresetStyle,.shortName="Styl",.longName="Preset style",.values={"Othr","Neut","Clen","Real","Slky","Raw ","Hevy","Krch"}},
		/* buttons (A,B,C,D,#,*) */
		{.type=ptCust,.number=cnLoad,.shortName="Load",.longName="Load preset"},
		{.type=ptCust,.number=cnSave,.shortName="Save",.longName="Save preset"},
		{.type=ptCust,.number=cnLPrv,.shortName="Prev",.longName="Load previous preset"},
		{.type=ptCust,.number=cnLNxt,.shortName="Next",.longName="Load next preset"},
		{.type=ptCust,.number=cnTrspM,.shortName="Trsp",.longName="Keyboard Transpose",.values={"Off ","Once","On  "}},
		{.type=ptCust,.number=cnNPrs,.shortName="NPrs",.longName="Set preset number digits"},
	},
};

#endif /* UI_PAGES_H */

