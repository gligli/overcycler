////////////////////////////////////////////////////////////////////////////////
// Potentiometers / keypad scanner
////////////////////////////////////////////////////////////////////////////////

#include "scan.h"

#include "ui.h"
#include "LPC177x_8x.h"
#include "lpc177x_8x_ssp.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_timer.h"
#include "dacspi.h"
#include "main.h"

#define ADC_CHANNEL_MASTER_MIX 10

#define POT_SAMPLES 5
#define POT_THRESHOLD (SCAN_ADC_QUANTUM*3)
#define POT_TIMEOUT (TICKER_1S)

#define DEBOUNCE_THRESHOLD 2

#define POTSCAN_PIN_CS 8
#define POTSCAN_PIN_DOUT 4
#define POTSCAN_PIN_DIN 1
#define POTSCAN_PIN_CLK 0

static struct
{
	int16_t curPotSample;

	uint16_t potSamples[SCAN_POT_COUNT][POT_SAMPLES];
	uint16_t potValue[SCAN_POT_COUNT];
	uint32_t potLockTimeout[SCAN_POT_COUNT];

	int8_t keypadState[16];
} scan EXT_RAM;

static uint8_t keypadButtonCode[16]=
{
	0x32,0x01,0x02,0x04,0x11,0x12,0x14,0x21,0x22,0x24,
	0x08,0x18,0x28,0x38,
	0x34,0x31
};

static uint16_t readADC(uint16_t channel)
{
	while(SSP_GetStatus(LPC_SSP2,SSP_STAT_TXFIFO_EMPTY)==RESET)
	{
		// wait until previous data is transferred
	}
	
	SSP_SendData(LPC_SSP2,channel<<8);

	while(SSP_GetStatus(LPC_SSP2,SSP_STAT_RXFIFO_NOTEMPTY)==RESET)
	{
		// wait for new data
	}

	uint16_t res=SSP_ReceiveData(LPC_SSP2)<<4; // convert to 16 bits
	
	// wait TLV2556 tConvert
	
	delay_us(6);

	return res;
}

static void readPots(void)
{
	uint16_t new;
	int pot;
	uint16_t tmpVal[SCAN_POT_COUNT+1];
	uint16_t tmpSmp[POT_SAMPLES];
	int8_t isUnlockable;
	
	// read pots from TLV2556 ADC

	for(pot=0;pot<=SCAN_POT_COUNT;++pot)
	{
		tmpVal[pot]=readADC(pot);
	}
		
	scan.curPotSample=(scan.curPotSample+1)%POT_SAMPLES;
	
	for(pot=0;pot<SCAN_POT_COUNT;++pot)
	{
		// reads are off by one by TLV2556 spec

		new=tmpVal[pot+1]>>(16-SCAN_ADC_BITS);

		scan.potSamples[pot][scan.curPotSample]=(MAX(0,MIN(999,new-12))*(UINT16_MAX*64/999))>>6;
		
		// sort values

		memcpy(&tmpSmp[0],&scan.potSamples[pot][0],POT_SAMPLES*sizeof(uint16_t));
		qsort(tmpSmp,POT_SAMPLES,sizeof(uint16_t),uint16Compare);		
		
		// median
	
		new=tmpSmp[POT_SAMPLES/2];
		
		// ignore small changes

		isUnlockable=abs(new-scan.potValue[pot])>=POT_THRESHOLD;
		
		if(isUnlockable || currentTick<scan.potLockTimeout[pot])
		{
			if(isUnlockable)
			{
				if(new!=scan.potValue[pot] && currentTick>=scan.potLockTimeout[pot])
				{
					// out of lock -> current value must be taken into account
					ui_scanEvent(-pot-1,NULL);
				}

				scan.potLockTimeout[pot]=currentTick+POT_TIMEOUT;
			}
			
			scan.potValue[pot]=new;
			ui_scanEvent(-pot-1,NULL);
		}
	}
}

static void readKeypad(void)
{
	int key;
	int row,col[4];
	int8_t newState;

	for(row=0;row<4;++row)
	{
		LPC_GPIO0->FIOSETH=0b1111<<3;
		LPC_GPIO0->FIOCLRH=(8>>row)<<3;
		delay_us(10);
		col[row]=0;
		
		col[row]|=((LPC_GPIO0->FIOPIN1>>2)&1)?0:1;
		col[row]|=((LPC_GPIO4->FIOPIN3>>5)&1)?0:2;
		col[row]|=((LPC_GPIO4->FIOPIN3>>4)&1)?0:4;
		col[row]|=((LPC_GPIO2->FIOPIN1>>5)&1)?0:8;
	}
	
	for(key=0;key<16;++key)
	{
		newState=(col[keypadButtonCode[key]>>4]&keypadButtonCode[key])?1:0;
	
		if(newState && scan.keypadState[key])
		{
			scan.keypadState[key]=MIN(INT8_MAX,scan.keypadState[key]+1);

			if(scan.keypadState[key]==DEBOUNCE_THRESHOLD)
				ui_scanEvent(key,NULL);
		}
		else
		{
			scan.keypadState[key]=newState;
		}
	}
}

void scan_sampleMasterMix(uint16_t sampleCount, uint16_t * buffer)
{
	TIM_TIMERCFG_Type tim;
	TIM_MATCHCFG_Type tm;
	int32_t mini=UINT16_MAX,maxi=0,extents;
	uint16_t *buf;
		
	// init timer
	
	tim.PrescaleOption=TIM_PRESCALE_TICKVAL;
	tim.PrescaleValue=1;
	TIM_Init(LPC_TIM2,TIM_TIMER_MODE,&tim);
	
	tm.MatchChannel=0;
	tm.IntOnMatch=ENABLE;
	tm.ResetOnMatch=ENABLE;
	tm.StopOnMatch=DISABLE;
	tm.ExtMatchOutputType=0;
	tm.MatchValue=DACSPI_TICK_RATE-1; // same rate as dacspi
	TIM_ConfigMatch(LPC_TIM2,&tm);

	TIM_ClearIntPending(LPC_TIM2,TIM_MR0_INT);
	TIM_Cmd(LPC_TIM2,ENABLE);
	
	readADC(ADC_CHANNEL_MASTER_MIX); // ensure no spurious reads from other channels
	readADC(ADC_CHANNEL_MASTER_MIX);
	
	// sample master mix at dacspi tickrate

	buf=buffer;
	for(uint16_t sc=0;sc<sampleCount;++sc)
	{
		while(!(LPC_TIM2->IR&TIM_IR_CLR(TIM_MR0_INT)))
			/* nothing */;
		
		TIM_ClearIntPending(LPC_TIM2,TIM_MR0_INT);
		
		uint16_t sample=readADC(ADC_CHANNEL_MASTER_MIX);
		
		mini=MIN(mini,sample);
		maxi=MAX(maxi,sample);
		
		*buf++=sample;
	}
	
//	rprintf(0,"scan_sampleMasterMix min %d max %d\n",mini,maxi);
	
	// extend waveform to 0..UINT16_MAX
	
	extents=maxi-mini+1;
	buf=buffer;
	for(uint16_t sc=0;sc<sampleCount;++sc)
	{
		uint16_t sample=*buf;
		
		sample=(((int32_t)sample-mini)<<16)/extents;
		
		*buf++=sample;
	}

	readADC(0); // restore state for readPots
	readADC(0);
}

uint16_t scan_getPotValue(int8_t pot)
{
	return scan.potValue[pot]&~(SCAN_ADC_QUANTUM-1);
}

void scan_resetPotLocking(void)
{
	memset(&scan.potLockTimeout[0],0,SCAN_POT_COUNT*sizeof(uint32_t));
}

void scan_init(int8_t isTunerMode)
{
	memset(&scan,0,sizeof(scan));
	
	// init TLV2556 ADC
	
	PINSEL_ConfigPin(1,POTSCAN_PIN_CS,4); // CS
	PINSEL_ConfigPin(1,POTSCAN_PIN_DOUT,4); // DOUT
	PINSEL_ConfigPin(1,POTSCAN_PIN_DIN,4); // DIN
	PINSEL_ConfigPin(1,POTSCAN_PIN_CLK,4); // CLK
	
	SSP_CFG_Type SSP_ConfigStruct;
	SSP_ConfigStructInit(&SSP_ConfigStruct);
	SSP_ConfigStruct.Databit=SSP_DATABIT_12;
	SSP_ConfigStruct.ClockRate=isTunerMode?8000000:80000;
	SSP_Init(LPC_SSP2,&SSP_ConfigStruct);
	SSP_Cmd(LPC_SSP2,ENABLE);
	
	readADC(0b1111); // config CFGR2
	
	// init keypad
	
	PINSEL_ConfigPin(0,22,0); // R1
	PINSEL_ConfigPin(0,21,0); // R2
	PINSEL_ConfigPin(0,20,0); // R3
	PINSEL_ConfigPin(0,19,0); // R4
	PINSEL_ConfigPin(0,10,0); // C1
	PINSEL_ConfigPin(4,29,0); // C2
	PINSEL_ConfigPin(4,28,0); // C3
	PINSEL_ConfigPin(2,13,0); // C4
	PINSEL_SetPinMode(0,10,PINSEL_BASICMODE_PULLUP);
	PINSEL_SetPinMode(4,29,PINSEL_BASICMODE_PULLUP);
	PINSEL_SetPinMode(4,28,PINSEL_BASICMODE_PULLUP);
	PINSEL_SetPinMode(2,13,PINSEL_BASICMODE_PULLUP);

	GPIO_SetValue(0,0b1111ul<<19);
	GPIO_SetDir(0,0b1111ul<<19,1);
}

void scan_update(void)
{
	readKeypad();
	readPots();
}
