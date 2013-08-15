///////////////////////////////////////////////////////////////////////////////
// Synthesizer
///////////////////////////////////////////////////////////////////////////////

#include "synth.h"
#include "wtosc.h"
#include "dacspi.h"

#define SYNTH_CV_COUNT 5

typedef enum {cvVrefA,cvBVrefB,cvCutoff,cvResonance,cvVCA} cv_t; 

struct
{
	struct wtosc_s osc[SYNTH_OSC_COUNT];
	struct fat16_dir_entry_struct dir_entry;
	struct fat16_dir_struct* dd;
	uint16_t cv[SYNTH_VOICE_COUNT][SYNTH_CV_COUNT];
} synth;

static void updateCV(int8_t voice, cv_t cv)
{
	int cvv;
	uint32_t addr;
	
	FIO0SET=0x400000;
	delay_us(2);
	FIO0CLR=0x4000c;

	cvv=voice*4+cv;
	addr=((cvv&3)<<2)|((cvv&4)<<16);
	
	dacspi_setReference(synth.cv[/*voice*/0][cv]);
	FIO0SET=addr;

	delay_us(20);

	FIO0CLR=0x400000;

	delay_us(10);
}

static void loadWaveTable(void)
{
	int i;
	
	struct fat16_file_struct* f;

	f=fat16_open_file(fs,&synth.dir_entry);
	if(f)
	{
		rprintf("loading %s %d\n",synth.dir_entry.long_name,synth.dir_entry.file_size);

		int32_t offs=0x2c;
		fat16_seek_file(f,&offs,SEEK_SET);

		int16_t data[600];

		fat16_read_file(f,(uint8_t*)data,sizeof(data));
		fat16_close_file(f);

		for(i=0;i<SYNTH_OSC_COUNT;++i)
			wtosc_setSampleData(&synth.osc[i],data,600);
	}
}

//#define BANK "AKWF_bw_saw"
//#define BANK "AKWF_bw_squ"
//#define BANK "AKWF_hvoice"
#define BANK "AKWF_bw_perfectwaves"

void synth_init(void)
{
	int i;
	int16_t sineShape[600];
	
	memset(&synth,0,sizeof(synth));
	
	for(i=0;i<600;++i)
		sineShape[i]=sin((i/300.0)*M_PI)/2.0*65535.0;

	for(i=0;i<SYNTH_OSC_COUNT;++i)
	{
		wtosc_init(&synth.osc[i],i,0x7000|((i&1)?0x8000:0));
		wtosc_setSampleData(&synth.osc[i],sineShape,600);
	}
	
	T0TC=0;	
	T0MR0=SYNTH_MASTER_CLOCK;
	T0MR1=SYNTH_MASTER_CLOCK;
	T0MR2=SYNTH_MASTER_CLOCK;
	T0MR3=SYNTH_MASTER_CLOCK;
	T0MCR=0x249; // int for all MR*

	T0PR=(CPU_CLOCK/SYNTH_MASTER_CLOCK)-1;
	T0PC=0;
	T0TCR=1;
	
	T1TC=0;	
	T1MR0=SYNTH_MASTER_CLOCK;
	T1MR1=SYNTH_MASTER_CLOCK;
	T1MR2=SYNTH_MASTER_CLOCK;
	T1MR3=SYNTH_MASTER_CLOCK;
	T1MCR=0x249; // int for all MR*

	T1PR=(CPU_CLOCK/SYNTH_MASTER_CLOCK)-1;
	T1PC=0;
	T1TCR=1;
	
	// setup TIMER0/TIMER1 vector as FIQ
	VICIntSelect|=3<<4;
	VICIntEnable|=3<<4; // enable interrupt

	dacspi_init();

    if(fat16_get_dir_entry_of_path(fs, "/WAVEDATA/" BANK, &synth.dir_entry))
	{
		synth.dd = fat16_open_dir(fs, &synth.dir_entry);
	}
	
	// cv
	FIO0DIR|=0x44000c;
}

extern volatile uint32_t dacspi_states[];
void synth_update(void)
{
	int key,v,cv;
	
	static int m=1;
	static int note=33;
	static int ali=0;
	static int inc=1024;
	
	key=getkey_serial0();
	
	switch(key)
	{
	case 'r':
		reset();
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
		if(fat16_read_dir(synth.dd,&synth.dir_entry))
		{
			while(!strstr(synth.dir_entry.long_name,".wav") && !strstr(synth.dir_entry.long_name,".WAV"))
				fat16_read_dir(synth.dd,&synth.dir_entry);
			loadWaveTable();
		}
		break;
	case 'q':
		synth.cv[0][0]+=inc;
		m=1;
		break;
	case 'w':
		synth.cv[0][0]-=inc;
		m=1;
		break;
	case 's':
		synth.cv[0][1]+=inc;
		m=1;
		break;
	case 'x':
		synth.cv[0][1]-=inc;
		m=1;
		break;
	case 'd':
		synth.cv[0][2]+=inc;
		m=1;
		break;
	case 'c':
		synth.cv[0][2]-=inc;
		m=1;
		break;
	case 'f':
		synth.cv[0][3]+=inc;
		m=1;
		break;
	case 'v':
		synth.cv[0][3]-=inc;
		m=1;
		break;
	}
	
	if(m)
	{
		rprintf("note %d %d\n",ali,note);
		
		for(int i=0;i<SYNTH_VOICE_COUNT/2;++i)
		{
			wtosc_setParameters(&synth.osc[i*4+0],((note+0)<<8)+32,ali);
			wtosc_setParameters(&synth.osc[i*4+1],((note+0)<<8)+7,ali);
			wtosc_setParameters(&synth.osc[i*4+2],((note+0)<<8)-27,ali);
			wtosc_setParameters(&synth.osc[i*4+3],((note+0)<<8)-8,ali);
		}
		
		rprintf("% 4u % 4u % 4u % 4u\n",synth.cv[0][0],synth.cv[0][1],synth.cv[0][2],synth.cv[0][3]);
		
		m=0;
	}
/*	rprintf("% 7u ",dacspi_states[7]);
	rprintf("% 7u ",dacspi_states[8+7]);
	rprintf("% 7u ",dacspi_states[16+7]);
	rprintf("% 7u ",dacspi_states[24+7]);
	rprintf("\n");*/
	
	for(v=0;v<SYNTH_VOICE_COUNT;++v)
		for(cv=0;cv<4;++cv)
			updateCV(v,cv);
}

