///////////////////////////////////////////////////////////////////////////////
// OverCyler, 2*DAC/VCF/VCA polyphonic wavetable synthesizer
// based on SparkFun's ARM Bootloader
///////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <math.h>

#include "LPC214x.h"

//UART0 Debugging
#include "serial.h"
#include "rprintf.h"

//basic system stuff
#include "system.h"

//SD Logging
#include "rootdir.h"
#include "sd_raw.h"

//USB
#include "main_msc.h"
#include "type.h"

#define CPU_FREQ 60000000 // hz

#define OC_VOICE_COUNT 2
#define OC_OSC_COUNT (OC_VOICE_COUNT*2)

#define DACSPI_SPEED_RATIO 6

#define WTOSC_MASTER_CLOCK CPU_FREQ
#define WTOSC_CV_OCTAVE (256*12)
#define WTOSC_MAX_SAMPLES 600 // samples
#define WTOSC_MAX_SAMPLE_RATE (CPU_FREQ/(35*OC_OSC_COUNT*(1+DACSPI_SPEED_RATIO)))

extern volatile uint16_t * dacspi_commands;

///////////////////////////////////////////////////////////////////////////////
// Wavetable oscillator
///////////////////////////////////////////////////////////////////////////////

struct wtosc_s
{
	uint16_t data[WTOSC_MAX_SAMPLES];	
	
	uint16_t controlData;

	int32_t counter;
	uint16_t cv;
	uint16_t output;
	int16_t samplePhase;

	volatile uint32_t samplePeriod;
	volatile uint16_t increment;
	volatile int16_t sampleCount;
};

struct
{
	struct wtosc_s osc[OC_OSC_COUNT];
	uint32_t lastCounter;
} wtsynth;

void wtosc_init(struct wtosc_s * o, uint16_t controlData);
void wtosc_setSampleData(struct wtosc_s * o, uint16_t * data, uint16_t sampleCount);
void wtosc_setParameters(struct wtosc_s * o, uint16_t cv, uint16_t increment);
void wtosc_update(struct wtosc_s * o, uint32_t ticks);

void wtsynth_init(void);
void wtsynth_setParameters(int8_t osc, uint16_t cv, uint16_t increment);
uint16_t wtsynth_getOutput(int8_t osc);
void wtsynth_update(void);

void wtosc_init(struct wtosc_s * o, uint16_t controlData)
{
	memset(o,0,sizeof(o));
	o->samplePeriod=WTOSC_MAX_SAMPLES;
	o->controlData=controlData;
}

void wtosc_setSampleData(struct wtosc_s * o, uint16_t * data, uint16_t sampleCount)
{
	BLOCK_INT
	{
		memset(o->data,0,WTOSC_MAX_SAMPLES*sizeof(uint16_t));

		if(sampleCount>WTOSC_MAX_SAMPLES)
			return;

		int i;
		for(i=0;i<sampleCount;++i)
			o->data[i]=(data[i]>>4)|o->controlData;

		o->sampleCount=sampleCount;
	}
}

void wtosc_setParameters(struct wtosc_s * o, uint16_t cv, uint16_t increment)
{
	double freq,note,rate;
	int subSample=1;
	
	note=cv/256.0; // midi note
	freq=pow(2.0,(note-69.0)/12.0)*440.0; // note frequency
	rate=freq*o->sampleCount; // sample rate

	while((rate/subSample)>(double)WTOSC_MAX_SAMPLE_RATE)
	{
		subSample*=2;
		rprintf("%d\n",(int)rate/subSample);
	}
	
	increment+=subSample;
	
	BLOCK_INT
	{
		o->samplePeriod=(double)WTOSC_MASTER_CLOCK*increment/rate;	
		o->increment=increment;
		o->cv=cv;
	}

	rprintf("inc %d cv %x per %d\n",o->increment,o->cv,o->samplePeriod);
}

void wtosc_update(struct wtosc_s * o, uint32_t ticks)
{
	o->counter-=ticks;
	
	if(o->counter<=0)
	{
		o->counter+=o->samplePeriod;
		
		o->samplePhase+=o->increment;
		if(o->samplePhase>=o->sampleCount)
			o->samplePhase-=o->sampleCount;
	
		o->output=o->data[o->samplePhase];
	}
}

void wtsynth_init(void)
{
	int i;
	uint16_t sineShape[600];
		
	for(i=0;i<600;++i)
	{
		sineShape[i]=(sin((i/300.0)*M_PI)+1.0)/2.0*65535.0;
//		rprintf("%d\n",sineShape[i]);
	}

	for(i=0;i<OC_OSC_COUNT;++i)
	{
		wtosc_init(&wtsynth.osc[i],0x7000);
		wtosc_setSampleData(&wtsynth.osc[i],sineShape,600);
	}
	
	// use PWM timer to keep track of time	
	PWMPR=0;
	PWMPC=0;
	PWMTCR=1;
}

void wtsynth_setParameters(int8_t osc, uint16_t cv, uint16_t increment)
{
	 wtosc_setParameters(&wtsynth.osc[osc],cv,increment);
}

inline uint16_t wtsynth_getOutput(int8_t osc)
{
	return wtsynth.osc[osc].output;
}

void wtsynth_update(void)
{
	uint32_t ctr;
	uint32_t tick;
	int8_t i;

	ctr=PWMTC;
	tick=ctr-wtsynth.lastCounter;
	wtsynth.lastCounter=ctr;

	for(i=0;i<OC_OSC_COUNT;++i)
	{
		wtosc_update(&wtsynth.osc[i],tick);
		dacspi_commands[i]=wtsynth.osc[i].output;
	}
}

///////////////////////////////////////////////////////////////////////////////
// 12bit DACs communication through SPI
///////////////////////////////////////////////////////////////////////////////

void dacspi_init(void);
void dacspi_advanceInterrupt(void);

struct
{
	int8_t currentChannel;
	
} dacspi;


void dacspi_init(void)
{
	memset(&dacspi,0,sizeof(dacspi));

	// SSP
	SSPCPSR=2; // 1/2 APB clock
	SSPIMSC=0x08; // enable TX fifo int
	SSPCR0=0x0f | (DACSPI_SPEED_RATIO<<8); // 16bit; spi; mode 0,0;
	SSPCR1=0x02; // normal; enable; master
	
	// pins
	PINSEL1&=~0x03cc;
	PINSEL1|=0x0288;

	// setup SSP vector as FIQ
//	VICVectAddr0=(unsigned long)&dacspi_refillInterrupt; // set interrupt vector in 0
	VICIntSelect|=1<<11;
//	VICVectCntl0=0x20 | 11; // use it
	VICIntEnable|=1<<11; // enable interrupt
}

void dacspi_advanceInterrupt(void)
{
	wtsynth_update();
}

///////////////////////////////////////////////////////////////////////////////
// Synthesizer
///////////////////////////////////////////////////////////////////////////////

void synth_init(void);
void synth_update(void);

void synth_init(void)
{
	wtsynth_init();
	dacspi_init();
}

void synth_update(void)
{
	int key;
	
	static int m=1;
	static int note=47;
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
	}

	if(m)
	{
		rprintf("note %d %d\n",incr,note);
		
		for(int i=0;i<OC_VOICE_COUNT;++i)
		{
			wtsynth_setParameters(i*2+0,((note+0)<<8),incr);
			wtsynth_setParameters(i*2+1,((note+12)<<8),incr);
		}
		
		m=0;
	}
}

///////////////////////////////////////////////////////////////////////////////
// OverCycler main code
///////////////////////////////////////////////////////////////////////////////

int main (void)
{
	boot_up();
	
	disableInts();
	
	// init Portios
	SCS=0x03; // select the "fast" version of the I/O ports
	FIO0MASK=0;
	FIO1MASK=0;

	synth_init();
	
	enableInts();
	
	for(;;)
	{
		synth_update();
	}
}
