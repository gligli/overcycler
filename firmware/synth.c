///////////////////////////////////////////////////////////////////////////////
// Synthesizer
///////////////////////////////////////////////////////////////////////////////

#include "synth.h"
#include "wtosc.h"
#include "dacspi.h"

struct
{
	struct wtosc_s osc[SYNTH_OSC_COUNT];
	uint32_t lastCounter;
} wtsynth;

void wtsynth_init(void);
void wtsynth_setParameters(int8_t osc, uint16_t cv, uint16_t increment);

void wtsynth_init(void)
{
	int i;
	int16_t sineShape[600];
		
	for(i=0;i<600;++i)
	{
		sineShape[i]=sin((i/300.0)*M_PI)/2.0*65535.0;
//		rprintf("%d\n",sineShape[i]);
	}

	for(i=0;i<SYNTH_OSC_COUNT;++i)
	{
		wtosc_init(&wtsynth.osc[i],i,0x7000);
		wtosc_setSampleData(&wtsynth.osc[i],sineShape,600);
	}
	
	PWMPR=0;
	PWMPC=0;
	PWMTCR=1;
	
	T0TC=0;	
	T0MR0=CPU_FREQ/WTOSC_MASTER_CLOCK;
	T0MCR=3; // int & reset for MR0

	T0PR=0;
	T0PC=0;
	T0TCR=1;
	
	// setup TIMER0 vector as FIQ
	VICIntSelect|=1<<4;
	VICIntEnable|=1<<4; // enable interrupt
}

void wtsynth_setParameters(int8_t osc, uint16_t cv, uint16_t increment)
{
	 wtosc_setParameters(&wtsynth.osc[osc],cv,increment);
}

struct
{
	struct fat16_dir_entry_struct dir_entry;
	struct fat16_dir_struct* dd;
} synth;

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
		{
			wtosc_init(&wtsynth.osc[i],i,0x7000);
			wtosc_setSampleData(&wtsynth.osc[i],data,600);
		}
	}
}

void synth_init(void)
{
	wtsynth_init();
	dacspi_init();

//    if(fat16_get_dir_entry_of_path(fs, "/WAVEDATA/AKWF_bw_perfectwaves", &synth.dir_entry))
    if(fat16_get_dir_entry_of_path(fs, "/WAVEDATA/AKWF_hvoice", &synth.dir_entry))
	{
		synth.dd = fat16_open_dir(fs, &synth.dir_entry);
	}
}

void synth_update(void)
{
	int key;
	
	static int m=1;
	static int note=33;
	static int incr=0;
	
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
		--note;
		m=1;
		break;
	case '*':
		++incr;
		m=1;
		break;
	case '/':
		--incr;
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
	}

	if(m)
	{
		rprintf("note %d %d\n",incr,note);
		
		for(int i=0;i<SYNTH_VOICE_COUNT/2;++i)
		{
			wtsynth_setParameters(i*4+0,((note+0)<<8),incr);
			wtsynth_setParameters(i*4+1,((note+0)<<8)+6,incr);
			wtsynth_setParameters(i*4+2,((note+0)<<8)+12,incr);
			wtsynth_setParameters(i*4+3,((note+0)<<8)+18,incr);
		}
		
		m=0;
	}
}

