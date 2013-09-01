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
	struct wtosc_s osc[SYNTH_VOICE_COUNT][2];
	DIR curDir;
	FILINFO curFile;
	char lfname[_MAX_LFN + 1];
	uint16_t cv[SYNTH_VOICE_COUNT][SYNTH_VOICE_CV_COUNT];
	uint16_t masterCv[SYNTH_MASTER_CV_COUNT];
} synth;


static void setCVReference(uint16_t value)
{
	uint16_t cmd;
	
	cmd=(value>>4)|DACSPI_CMD_SET_REF;
	
	dacspi_setCommand(DACSPI_CV_CHANNEL,0,cmd);
	dacspi_setCommand(DACSPI_CV_CHANNEL,1,cmd);
	delay_us(10);
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
		
		for(i=0;i<SYNTH_VOICE_COUNT;++i)
		{
			wtosc_setSampleData(&synth.osc[i][0],data,WTOSC_MAX_SAMPLES);
			wtosc_setSampleData(&synth.osc[i][1],data,WTOSC_MAX_SAMPLES);
		}
		
		rprintf(0,"loaded\n");
	}
}

#define DO_OSC(v,o) dacspi_setCommand(v,o,wtosc_update(&synth.osc[v][o],DACSPI_TICK_RATE))

void synth_updateDACs(void)
{
	DO_OSC(0,0);
	DO_OSC(0,1);
	DO_OSC(1,0);
	DO_OSC(1,1);
	DO_OSC(2,0);
	DO_OSC(2,1);
	DO_OSC(3,0);
	DO_OSC(3,1);
	DO_OSC(4,0);
	DO_OSC(4,1);
	DO_OSC(5,0);
	DO_OSC(5,1);
}

void synth_init(void)
{
	int i;
	FRESULT res;
	int16_t sineShape[WTOSC_MAX_SAMPLES];
	
	memset(&synth,0,sizeof(synth));
	
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
	
	for(i=0;i<SYNTH_VOICE_COUNT;++i)
	{
		wtosc_init(&synth.osc[i][0],DACSPI_CMD_SET_A);
		wtosc_init(&synth.osc[i][1],DACSPI_CMD_SET_B);
		wtosc_setSampleData(&synth.osc[i][0],sineShape,WTOSC_MAX_SAMPLES);
		wtosc_setSampleData(&synth.osc[i][1],sineShape,WTOSC_MAX_SAMPLES);
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
	
	// init spi dac

	dacspi_init();
	
}

void synth_update(void)
{
	int v,cv;
	
	static int m=1;

	static int note=33;
	static int ali=0;
	static int uni=0;
	static int det=0;
	static int shp=0;

	updateCV(6,cvMasterLeft);
	updateCV(6,cvMasterRight);
	
	for(v=0;v<SYNTH_VOICE_COUNT;++v)
		for(cv=0;cv<SYNTH_VOICE_CV_COUNT;++cv)
		{
			synth.cv[v][cv]=synth.cv[0][cv];
			updateCV(v,cv);
		}
	
	ui_update();
	
	synth.masterCv[0]=ui_getPotValue(4);
	synth.masterCv[1]=ui_getPotValue(4);
	
	synth.cv[0][0]=ui_getPotValue(0);
	synth.cv[0][1]=ui_getPotValue(5);
	synth.cv[0][2]=UINT16_MAX-ui_getPotValue(1);
	synth.cv[0][3]=ui_getPotValue(6);
	synth.cv[0][4]=satAddU16S16(ui_getPotValue(9),-(int32_t)ui_getPotValue(6)/6);
/*	if(ali!=ui_getPotValue(4)>>8)
	{
		ali=ui_getPotValue(4)>>8;
		m=1;
	}*/
	if(det!=ui_getPotValue(2)>>8)
	{
		det=ui_getPotValue(2)>>8;
		m=1;
	}
	if(uni!=ui_getPotValue(8)>>10)
	{
		uni=ui_getPotValue(8)>>10;
		m=1;
	}
	if(note!=(ui_getPotValue(7)>>9)<<9)
	{
		note=(ui_getPotValue(7)>>9)<<9;
		m=1;
	}
	if(shp!=ui_getPotValue(3)>>9)
	{
		int p=0;		

		shp=ui_getPotValue(3)>>9;
				
		f_opendir(&synth.curDir,BANK_DIR);
		do
		{
			f_readdir(&synth.curDir,&synth.curFile);
			++p;
		}
		while(p<=shp && synth.curFile.fname[0]!=0);

		loadWaveTable();
	}


	if(m)
	{
//		rprintf(0,"note %d %d\n",ali,note);
		
		for(v=0;v<SYNTH_VOICE_COUNT;++v)
		{
			if(v&1)
				cv=note+v*uni;
			else
				cv=note-(v+1)*uni;
				
			wtosc_setParameters(&synth.osc[v][0],cv-det,ali);
			wtosc_setParameters(&synth.osc[v][1],/*512*12+*/cv+det,ali);
		}

//		rprintf(0,"refA % 4u refB % 4u Fc % 4u Q % 4u\n",synth.cv[0][0],synth.cv[0][1],synth.cv[0][2],synth.cv[0][3]);
		
		m=0;
	}

}

