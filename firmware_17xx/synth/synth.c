///////////////////////////////////////////////////////////////////////////////
// Synthesizer
///////////////////////////////////////////////////////////////////////////////

#include "synth.h"
#include "wtosc.h"
#include "dacspi.h"
#include "ff.h"
#include "ui.h"
#include "uart_midi.h"

#define SYNTH_VOICE_CV_COUNT 5
#define SYNTH_MASTER_CV_COUNT 2

//#define BANK "AKWF_bw_saw"
//#define BANK "AKWF_bw_squ"
//#define BANK "AKWF_hvoice"
#define BANK "test"

#define BANK_DIR "/WAVEDATA/" BANK


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

typedef enum {cvVrefA=0,cvBVrefB=1,cvCutoff=2,cvResonance=3,cvVCA=4,cvMasterLeft=5,cvMasterRight=6} cv_t; 

struct
{
	struct wtosc_s osc[SYNTH_OSC_COUNT];
	DIR curDir;
	FILINFO curFile;
	char lfname[_MAX_LFN + 1];
	uint16_t cv[SYNTH_VOICE_COUNT][SYNTH_VOICE_CV_COUNT];
	uint16_t masterCv[SYNTH_MASTER_CV_COUNT];
} synth;


volatile int8_t cvReferenceSent = 1;
volatile uint16_t cvReference = 0;

static void setCVReference(uint16_t value)
{
	cvReference=DACSPI_CMD_SET_REF|(value>>4);
	cvReferenceSent=0;

	while(!cvReferenceSent)
		__NOP();
}
static void sendCVReference(void)
{
	if(!cvReferenceSent)
	{
		dacspi_sendCommand(DACSPI_CV_CHANNEL,cvReference);
		cvReferenceSent=1;
	}
}

static void updateCV(int8_t voice, cv_t cv)
{
	GPIO_ClearValue(CVMUX_PORT_ABC,7<<CVMUX_PIN_A);
	
	if(cv<cvVCA)
	{
		setCVReference(synth.cv[voice][cv]);
	
		if(cv&1)
			GPIO_SetValue(CVMUX_PORT_ABC,1<<CVMUX_PIN_A);
		if(cv&2)
			GPIO_SetValue(CVMUX_PORT_ABC,1<<CVMUX_PIN_B);
		if(voice&1)
			GPIO_SetValue(CVMUX_PORT_ABC,1<<CVMUX_PIN_C);
		
		delay_us(5);
	
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
	else
	{
		if(cv==cvVCA)
		{
			GPIO_SetValue(CVMUX_PORT_ABC,voice<<CVMUX_PIN_A);

			setCVReference(synth.cv[voice][cvVCA]);
		}
		else
		{
			cv-=cvMasterLeft;
			GPIO_SetValue(CVMUX_PORT_ABC,(cv+SYNTH_VOICE_COUNT)<<CVMUX_PIN_A);

			setCVReference(synth.masterCv[cv]);
		}
		
		delay_us(5);
	
		GPIO_ClearValue(CVMUX_PORT_VCA,1<<CVMUX_PIN_VCA);
	}

	delay_us(20);
	
	GPIO_SetValue(CVMUX_PORT_CARD0,1<<CVMUX_PIN_CARD0);
	GPIO_SetValue(CVMUX_PORT_CARD1,1<<CVMUX_PIN_CARD1);
	GPIO_SetValue(CVMUX_PORT_CARD2,1<<CVMUX_PIN_CARD2);
	GPIO_SetValue(CVMUX_PORT_VCA,1<<CVMUX_PIN_VCA);
}

static void timersEnable(int8_t enable)
{
	TIM_Cmd(LPC_TIM0,enable);
	TIM_Cmd(LPC_TIM1,enable);
	TIM_Cmd(LPC_TIM2,enable);
}

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
		
		timersEnable(0);

		for(i=0;i<SYNTH_OSC_COUNT;++i)
			wtosc_setSampleData(&synth.osc[i],data,WTOSC_MAX_SAMPLES);
		
		timersEnable(1);
		
		rprintf(0,"loaded\n");
	}
}

__attribute__ ((used)) void TIMER0_IRQHandler(void)
{
	uint32_t ir=LPC_TIM0->IR;
	LPC_TIM0->IR=ir;

	if(ir&(1<<TIM_MR0_INT))
		LPC_TIM0->MR0+=wtosc_update(&synth.osc[0]);

	if(ir&(1<<TIM_MR1_INT))
		LPC_TIM0->MR1+=wtosc_update(&synth.osc[1]);

	if(ir&(1<<TIM_MR2_INT))
		LPC_TIM0->MR2+=wtosc_update(&synth.osc[2]);

	if(ir&(1<<TIM_MR3_INT))
		LPC_TIM0->MR3+=wtosc_update(&synth.osc[3]);

	sendCVReference();
}

__attribute__ ((used)) void TIMER1_IRQHandler(void)
{
	uint32_t ir=LPC_TIM1->IR;
	LPC_TIM1->IR=ir;

	if(ir&(1<<TIM_MR0_INT))
		LPC_TIM1->MR0+=wtosc_update(&synth.osc[4]);

	if(ir&(1<<TIM_MR1_INT))
		LPC_TIM1->MR1+=wtosc_update(&synth.osc[5]);

	if(ir&(1<<TIM_MR2_INT))
		LPC_TIM1->MR2+=wtosc_update(&synth.osc[6]);

	if(ir&(1<<TIM_MR3_INT))
		LPC_TIM1->MR3+=wtosc_update(&synth.osc[7]);
}


__attribute__ ((used)) void TIMER2_IRQHandler(void)
{
	uint32_t ir=LPC_TIM2->IR;
	LPC_TIM2->IR=ir;

	if(ir&(1<<TIM_MR0_INT))
		LPC_TIM2->MR0+=wtosc_update(&synth.osc[8]);

	if(ir&(1<<TIM_MR1_INT))
		LPC_TIM2->MR1+=wtosc_update(&synth.osc[9]);

	if(ir&(1<<TIM_MR2_INT))
		LPC_TIM2->MR2+=wtosc_update(&synth.osc[10]);

	if(ir&(1<<TIM_MR3_INT))
		LPC_TIM2->MR3+=wtosc_update(&synth.osc[11]);
}

void synth_init(void)
{
	int i;
	FRESULT res;
	int16_t sineShape[WTOSC_MAX_SAMPLES];
	
	memset(&synth,0,sizeof(synth));
	
	// init ocs timers (hardcoded for 6 voices)
	
	TIM_TIMERCFG_Type tim;
	
	tim.PrescaleOption=TIM_PRESCALE_TICKVAL;
	tim.PrescaleValue=1;
	
	TIM_Init(LPC_TIM0,TIM_TIMER_MODE,&tim);
	TIM_Init(LPC_TIM1,TIM_TIMER_MODE,&tim);
	TIM_Init(LPC_TIM2,TIM_TIMER_MODE,&tim);
	
	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_TIMER0,CLKPWR_PCLKSEL_CCLK_DIV_1);
	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_TIMER1,CLKPWR_PCLKSEL_CCLK_DIV_1);
	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_TIMER2,CLKPWR_PCLKSEL_CCLK_DIV_1);
	
	TIM_MATCHCFG_Type tm;
	
	for(i=0;i<4;++i)
	{
		tm.MatchChannel=i;
		tm.IntOnMatch=ENABLE;
		tm.ResetOnMatch=DISABLE;
		tm.StopOnMatch=DISABLE;
		tm.ExtMatchOutputType=0;
		tm.MatchValue=(i*4+4)*SYNTH_MASTER_CLOCK/8;
		
		TIM_ConfigMatch(LPC_TIM0,&tm);
		TIM_ConfigMatch(LPC_TIM1,&tm);
		TIM_ConfigMatch(LPC_TIM2,&tm);
	}
	
	NVIC_SetPriority(TIMER0_IRQn,1);
	NVIC_SetPriority(TIMER1_IRQn,1);
	NVIC_SetPriority(TIMER2_IRQn,1);

	NVIC_EnableIRQ(TIMER0_IRQn);
	NVIC_EnableIRQ(TIMER1_IRQn);
	NVIC_EnableIRQ(TIMER2_IRQn);
	
	// init spi dac

	dacspi_init();
	
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
	
	// default waveform
	
	for(i=0;i<WTOSC_MAX_SAMPLES;++i)
		sineShape[i]=sin(M_TWOPI*i/WTOSC_MAX_SAMPLES)/2.0*65535.0;

	// init wavetable oscs
	
	for(i=0;i<SYNTH_OSC_COUNT;++i)
	{
		uint16_t ctl=(i&1)?DACSPI_CMD_SET_B:DACSPI_CMD_SET_A;
		wtosc_init(&synth.osc[i],i>>1,ctl);
		wtosc_setSampleData(&synth.osc[i],sineShape,WTOSC_MAX_SAMPLES);
	}

	synth.curFile.lfname=synth.lfname;
	synth.curFile.lfsize=sizeof(synth.lfname);
	
    if((res=f_opendir(&synth.curDir,BANK_DIR)))
		rprintf(0,"f_opendir res=%d\n",res);

	if((res=f_readdir(&synth.curDir,&synth.curFile)))
		rprintf(0,"f_readdir res=%d\n",res);

	loadWaveTable();

	// init UI
	
	ui_init();
	
	// init MIDI uart
	
	uartMidi_init();
	
	// time to start the oscs

	timersEnable(1);
	
}

void synth_update(void)
{
	int key,v,cv;
	
	static int m=1;
	static int note=33;
	static int ali=0;
	
	key=getkey_serial0();
	
	switch(key)
	{
	case 'r':
		NVIC_SystemReset();
		break;
	case '+':
		++note;
		m=1;
		break;
	case '-':
		if(note>12)
			--note;
		m=1;
		break;
	case '*':
		++ali;
		m=1;
		break;
	case '/':
		if(ali>0)
			--ali;
		m=1;
		break;
	case 'n':
		do
		{
			if(f_readdir(&synth.curDir,&synth.curFile) || (synth.curFile.fname[0]==0))
				f_opendir(&synth.curDir,BANK_DIR);
		}
		while(!strstr(synth.curFile.fname,".WAV"));

		loadWaveTable();
		break;
	}
	
	if(m)
	{
		rprintf(0,"note %d %d\n",ali,note);
		
		for(int i=0;i<SYNTH_OSC_COUNT;++i)
			wtosc_setParameters(&synth.osc[i],(note<<8)+i*13,ali);

		rprintf(0,"refA % 4u refB % 4u Fc % 4u Q % 4u\n",synth.cv[0][0],synth.cv[0][1],synth.cv[0][2],synth.cv[0][3]);
		
		m=0;
	}

	for(v=0;v<SYNTH_VOICE_COUNT;++v)
		for(cv=0;cv<4;++cv)
		{
			synth.cv[v][cv]=synth.cv[0][cv];
			updateCV(v,cv);
		}
	
	ui_update();
	
	synth.cv[0][0]=ui_getPotValue(0);
	synth.cv[0][1]=ui_getPotValue(5);
	synth.cv[0][2]=ui_getPotValue(1);
	synth.cv[0][3]=ui_getPotValue(6);
	if(ali!=ui_getPotValue(4)>>8)
	{
		ali=ui_getPotValue(4)>>8;
		m=1;
	}
}

