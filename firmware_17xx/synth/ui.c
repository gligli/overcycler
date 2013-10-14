///////////////////////////////////////////////////////////////////////////////
// LCD / keypad / potentiometers user interface
///////////////////////////////////////////////////////////////////////////////

#include "ui.h"
#include "storage.h"
#include "integer.h"

#define DISPLAY_ACK 6

#define ADC_QUANTUM (1<<(16-12)) // 12bit resolution

#define ADCROW_PORT 3
#define ADCROW_PIN 25

#define UI_POT_COUNT 10
#define UI_POT_SAMPLES 64

#define POT_CHANGE_DETECT_THRESHOLD (ADC_QUANTUM*10)
#define POT_TIMEOUT_THRESHOLD (ADC_QUANTUM*5)
#define POT_TIMEOUT (TICKER_1S/2)
#define ACTIVE_SOURCE_TIMEOUT (TICKER_1S*2/3)

enum uiDigitInput_e{
	diSynth,diLoadDecadeDigit,diStoreDecadeDigit,diLoadUnitDigit,diStoreUnitDigit
};

enum uiParamType_e
{
	ptNone=0,ptCont,ptStep,ptActn
};

enum uiKeypadButton_e
{
	kb0=0,kb1,kb2,kb3,kb4,kb5,kb6,kb7,kb8,kb9,
	kbA,kbB,kbC,kbD,
	kbAsterisk,kbSharp
};

enum uiPage_e
{
	upNone=-1,upOscs=0,upFil=1,upAmp=2,upMod=3,upSeqArp=4,upMisc=5
};

struct uiParam_s
{
	enum uiParamType_e type;
	int8_t number;
	const char * shortName; // 4 chars + zero termination
	const char * longName;
	const char * values[8]; // 4 chars + zero termination
};

const struct uiParam_s uiParameters[6][2][10] = // [pages][0=pots/1=keys][pot/key num]
{
	/* Oscillators page (A) */
	{
		{
			/* 1st row of pots */
			{.type=ptStep,.number=spABank,.shortName="ABnk",.longName="Osc A Bank"},
			{.type=ptStep,.number=spAWave,.shortName="AWav",.longName="Osc A Waveform"},
			{.type=ptCont,.number=cpABaseWMod,.shortName="AWmo",.longName="Osc A WaveMod"},
			{.type=ptCont,.number=cpBFreq,.shortName="BFrq",.longName="Osc B Frequency"},
			{.type=ptCont,.number=cpAVol,.shortName="AVol",.longName="Osc A Volume"},
			/* 2nd row of pots */
			{.type=ptStep,.number=spBBank,.shortName="BBnk",.longName="Osc B Bank"},
			{.type=ptStep,.number=spBWave,.shortName="BWav",.longName="Osc B Waveform"},
			{.type=ptCont,.number=cpBBaseWMod,.shortName="BWmo",.longName="Osc B WaveMod"},
			{.type=ptCont,.number=cpBFineFreq,.shortName="BDet",.longName="Osc B Detune"},
			{.type=ptCont,.number=cpBVol,.shortName="BVol",.longName="Osc B Volume"},
		},
		{
			/*0*/ {.type=ptNone},
			/*1*/ {.type=ptStep,.number=spAWModType,.shortName="AWmT",.longName="Osc A WaveMod type",.values={"Off ","Alia","Wdth"}},
			/*2*/ {.type=ptStep,.number=spBWModType,.shortName="BWmT",.longName="Osc B WaveMod type",.values={"Off ","Alia","Wdth"}},
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
			/*0*/ {.type=ptNone},
			/*1*/ {.type=ptStep,.number=spFilEnvSlow,.shortName="FEnT",.longName="Filter Envelope Type",.values={"Fast","Slow"}},
			/*2*/ {.type=ptStep,.number=spAWModEnvEn,.shortName="AWmE",.longName="A WaveMod envelope",.values={"Off ","On  "}},
			/*3*/ {.type=ptStep,.number=spBWModEnvEn,.shortName="BWmE",.longName="B WaveMod envelope",.values={"Off ","On  "}},
		},
	},
	/* Amplifier page (C) */
	{
		{
			/* 1st row of pots */
			{.type=ptCont,.number=cpGlide,.shortName="Glid",.longName="Glide amount"},
			{.type=ptCont,.number=cpUnisonDetune,.shortName="MDet",.longName="Master unison Detune"},
			{.type=ptCont,.number=cpMasterTune,.shortName="MTun",.longName="Master Tune"},
			{.type=ptCont,.number=cpMasterLeft,.shortName="MLfV",.longName="Master Left volume"},
			{.type=ptCont,.number=cpMasterRight,.shortName="MRtV",.longName="Master Right volume"},
			/* 2nd row of pots */
			{.type=ptCont,.number=cpAmpAtt,.shortName="AAtk",.longName="Amplifier Attack"},
			{.type=ptCont,.number=cpAmpDec,.shortName="ADec",.longName="Amplifier Decay"},
			{.type=ptCont,.number=cpAmpSus,.shortName="ASus",.longName="Amplifier Sustain"},
			{.type=ptCont,.number=cpAmpRel,.shortName="ARel",.longName="Amplifier Release"},
			{.type=ptCont,.number=cpAmpVelocity,.shortName="AVel",.longName="Amplifier Velocity"},
		},
		{
			/*0*/ {.type=ptNone},
			/*1*/ {.type=ptStep,.number=spAmpEnvSlow,.shortName="AEnT",.longName="Amplifier Envelope Type",.values={"Fast","Slow"}},
			/*2*/ {.type=ptStep,.number=spUnison,.shortName="Unis",.longName="Unison",.values={"Off ","On  "}},
			/*3*/ {.type=ptStep,.number=spAssignerPriority,.shortName="Prio",.longName="Assigner Priority",.values={"Last","Low ","High"}},
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
			/*0*/ {.type=ptNone},
			/*1*/ {.type=ptStep,.number=spLFOShift,.shortName="LRng",.longName="LFO Range",.values={"Slow","Fast"}},
			/*2*/ {.type=ptStep,.number=spLFOTargets,.shortName="LTgt",.longName="LFO OSc target",.values={"Off ","OscA","OscB","Both"}},
			/*3*/ {.type=ptStep,.number=spModwheelRange,.shortName="MRng",.longName="Modwheel range",.values={"Min ","Low ","High","Full"}},
			/*4*/ {.type=ptStep,.number=spModwheelTarget,.shortName="MTgt",.longName="Modwheel target",.values={"LFO ","Vib "}},
		},
	},
	/* Sequencer/arpeggiator page (#) */
	{
		{
		
		},
		{

		},
	},
	/* Miscellaneous page (*) */
	{
		{

		},
		{

		},
	},
};

static const uint8_t potToCh[UI_POT_COUNT]=
{
	0,1,2,3,5,
	0,1,2,3,5,
};

static const uint8_t keypadButtonCode[16]=
{
	0xd7,0xee,0xde,0xbe,0xed,0xdd,0xbd,0xeb,0xdb,0xbb,
	0x7e,0x7d,0x7b,0x77,
	0xe7,0xb7
};

static struct
{
	uint16_t pots[UI_POT_COUNT][UI_POT_SAMPLES];
	int16_t curSample;
	int8_t row;
	uint16_t value[UI_POT_COUNT];
	uint32_t lockTimeout[UI_POT_COUNT];
	
	int8_t presetModified;
	enum uiPage_e activePage;
	
	int8_t activeSource;
	uint32_t activeSourceTimeout;
	
	uint32_t slowUpdateTimeout;
	int8_t slowUpdateTimeoutNumber;
	
	int8_t pendingScreenClear;
} ui;

extern void refreshFullState(void);
extern void refreshWaves(int8_t ab);

__attribute__ ((used)) void ADC_IRQHandler(void)
{
	int pot,i;
	
	pot=ui.row?(UI_POT_COUNT/2):0;

	for(i=0;i<UI_POT_COUNT/2;++i)
	{
		uint32_t adc_value;
 
		adc_value = *(uint32_t *) ((&LPC_ADC->ADDR0) + potToCh[pot]);
		
		if(adc_value&ADC_DR_DONE_FLAG)
			ui.pots[pot][ui.curSample]=(uint16_t)adc_value;
		
		++pot;
	}

	if(ui.row)
	{
		GPIO_ClearValue(ADCROW_PORT,1<<ADCROW_PIN);
	}
	else
	{
		GPIO_SetValue(ADCROW_PORT,1<<ADCROW_PIN);
		
		// scanned all the pots, move to next sample
		
		++ui.curSample;
		if(ui.curSample>=UI_POT_SAMPLES)
			ui.curSample=0;
	}
	
	ui.row=!ui.row;
	
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
		sprintf(dv,"%4d",v>>6);
		break;
	case ptStep:
		valCount=0;
		while(valCount<8 && prm->values[valCount]!=NULL)
			++valCount;

		v=currentPreset.steppedParameters[prm->number];
		
		if(v<valCount)
			strcpy(dv,prm->values[v]);
		else
			sprintf(dv,"%4d",v);
		break;
	default:
		;
	}
	
	return dv;
}

static void handleUserInput(int8_t source) // source: keypad (kb0..kbSharp) / (-1..-10)
{
	int32_t data,valCount;
	int8_t potnum;
	const struct uiParam_s * prm;

//	rprintf(0,"handleUserInput %d\n",source);
	
	if(source>=kbA)
	{
		ui.activePage=source-kbA;
		rprintf(0,"page %d\n",ui.activePage);
		return;
	}
	
	if(ui.activePage==upNone)
		return;

	// fullscreen display
	if (ui.activeSource!=source)
		ui.pendingScreenClear=1;
	ui.activeSource=source;
	ui.activeSourceTimeout=currentTick+ACTIVE_SOURCE_TIMEOUT;
	
	potnum=-source-1;
	prm=&uiParameters[ui.activePage][source<0?0:1][source<0?potnum:source];

	switch(prm->type)
	{
	case ptCont:
		currentPreset.continuousParameters[prm->number]=getPotValue(potnum);
		break;
	case ptStep:
		valCount=0;
		while(valCount<8 && prm->values[valCount]!=NULL)
			++valCount;
		
		if(!valCount)
			valCount=1<<steppedParametersBits[prm->number];

		if(source<0)
			data=(getPotValue(potnum)*valCount)>>16;
		else
			data=(currentPreset.steppedParameters[prm->number]+1)%valCount;

		currentPreset.steppedParameters[prm->number]=data;
		
		//	special cases
		
			// unison latch
		if(prm->number==spUnison)
		{
			if(data)
				assigner_latchPattern();
			else
				assigner_setPoly();
			assigner_getPattern(currentPreset.voicePattern,NULL);
		}
		else if(prm->number==spAWave || prm->number==spBWave)
		{
			ui.slowUpdateTimeout=currentTick+ACTIVE_SOURCE_TIMEOUT;
			ui.slowUpdateTimeoutNumber=prm->number;
		}
		
		break;
	default:
		/*nothing*/;
	}
	
	ui.presetModified=1;

	refreshFullState();
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
	int i,cnt;
	uint32_t acc,new,delta;
	
	// sort values
	
	memcpy(&tmp[0],&ui.pots[pot][0],UI_POT_SAMPLES*sizeof(uint16_t));
	qsort(tmp,UI_POT_SAMPLES,sizeof(uint16_t),uint16Compare);
	
	// average of non-extreme values
	
	acc=0;
	cnt=0;

	for(i=UI_POT_SAMPLES*7/16;i<UI_POT_SAMPLES*9/16;++i)
	{
		acc+=tmp[i];
		++cnt;
	}

	new=acc/cnt;
	
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
	
	// init row select
	
	PINSEL_SetPinFunc(ADCROW_PORT,ADCROW_PIN,0);
	GPIO_SetDir(ADCROW_PORT,1<<ADCROW_PIN,1);
	
	// init adc pins

	PINSEL_SetPinFunc(0,23,1);
	PINSEL_SetPinFunc(0,24,1);
	PINSEL_SetPinFunc(0,25,1);
	PINSEL_SetPinFunc(0,26,1);
	PINSEL_SetPinFunc(1,31,3);
	
	// init adc
	
	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_ADC,CLKPWR_PCLKSEL_CCLK_DIV_8);
	CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCAD,ENABLE);

	LPC_ADC->ADCR=0x2f | ADC_CR_CLKDIV(31);
	LPC_ADC->ADINTEN=0x01;
	
	NVIC_SetPriority(ADC_IRQn,15);
	NVIC_EnableIRQ(ADC_IRQn);

	// start
	
	LPC_ADC->ADCR|= ADC_CR_PDN | ADC_CR_BURST;
	
	ui.activePage=upNone;
}

void ui_update(void)
{
	int i;
	uint16_t v;
	
	// update pot values

	for(i=0;i<UI_POT_COUNT;++i)
		updatePotValue(i);

	// display

	if(ui.pendingScreenClear)
		sendCommand("[2J",1,NULL); // clear screen
	else
		sendCommand("[H",1,NULL); // home pos
	ui.pendingScreenClear=0;

	if(ui.activePage==upNone)
	{
		rprintf(1,"GliGli's OverCycler ");
		rprintf(1,"A: Oscs   B: Filter ");
		rprintf(1,"C: Ampli  D: LFO/Mod");
		rprintf(1,"*: Misc   #: Seq/Arp");
		
	}
	else if(ui.activeSourceTimeout>currentTick)
	{
		rprintf(1,getName(ui.activeSource,1));
		sendCommand("[H",1,NULL);
		rprintf(1,"\r\r        %s\r",getDisplayValue(ui.activeSource,&v));
		for(i=0;i<20;++i)
			if(i<(((int32_t)v+UINT16_MAX/40)*20>>16))
				sendChar(255);
			else
				sendChar(' ');

	}
	else
	{
		ui.activeSource=INT8_MAX;

		for(i=0;i<UI_POT_COUNT/2;++i)
			rprintf(1,getName(-i-1,0));
			
		for(i=0;i<UI_POT_COUNT;++i)
			rprintf(1,getDisplayValue(-i-1,NULL));

		for(i=UI_POT_COUNT/2;i<UI_POT_COUNT;++i)
			rprintf(1,getName(-i-1,0));
	}

	// read keypad
			
	readKeypad();
	
	// slow updates
	
	if(currentTick>ui.slowUpdateTimeout)
	{
		// waveforms
		if(ui.slowUpdateTimeoutNumber==spAWave)
			refreshWaves(0);
		
		if(ui.slowUpdateTimeoutNumber==spBWave)
			refreshWaves(1);
		
		ui.slowUpdateTimeout=UINT32_MAX;
	}
}
