////////////////////////////////////////////////////////////////////////////////
// Potentiometers / keypad scanner
////////////////////////////////////////////////////////////////////////////////

#include "scan.h"

#include "LPC177x_8x.h"
#include "lpc177x_8x_gpdma.h"
#include "lpc177x_8x_ssp.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_timer.h"
#include "dacspi.h"
#include "main.h"

#define SCAN_ADC_BITS 10

#define DMA_CHANNEL_SSP1_TX__T2_MAT_0 4

#define KEYPAD_DEBOUNCE_THRESHOLD 2

#define POT_SAMPLES 5
#define POT_UNLOCK_THRESHOLD SCAN_POT_TO_16BITS(6)
#define POT_TIMEOUT_THRESHOLD SCAN_POT_TO_16BITS(3)
#define POT_TIMEOUT (TICKER_HZ)

#define POTSCAN_PIN_CS 8
#define POTSCAN_PIN_DOUT 4
#define POTSCAN_PIN_DIN 1
#define POTSCAN_PIN_CLK 0

#define POTSCAN_PRE_DIV 12
#define POTSCAN_SPI_FREQUENCY 80000
#define POTSCAN_FREQUENCY (POTSCAN_SPI_FREQUENCY/(SCAN_ADC_BITS*SCAN_POT_COUNT*2))

#define POTSCAN_DMACONFIG \
		GPDMA_DMACCxConfig_E | \
		GPDMA_DMACCxConfig_SrcPeripheral(DMA_CHANNEL_SSP1_TX__T2_MAT_0) | \
		GPDMA_DMACCxConfig_TransferType(2)

#define MIXSCAN_SPI_FREQUENCY 8000000
#define MIXSCAN_ADC_CHANNEL 10

static EXT_RAM GPDMA_LLI_Type lli[POT_SAMPLES*SCAN_POT_COUNT][2];

static struct
{
	uint16_t potCommands[SCAN_POT_COUNT];
	uint16_t potSamples[POT_SAMPLES*SCAN_POT_COUNT];
	uint16_t potValue[SCAN_POT_COUNT];
	uint32_t potLockTimeout[SCAN_POT_COUNT];
	uint16_t potLockValue[SCAN_POT_COUNT];

	int8_t keypadState[kbCount];
	
	scan_event_callback_t eventCallback;
	
	int8_t pendingSPIFlush;
} scan EXT_RAM;

static const uint8_t keypadButtonCode[kbCount]=
{
	0x32,0x01,0x02,0x04,0x11,0x12,0x14,0x21,0x22,0x24,
	0x08,0x18,0x28,0x38,
	0x34,0x31
};

static void buildLLIs(int pot, int smp)
{
	int lliPos=smp*SCAN_POT_COUNT+pot;
	
	lli[lliPos][0].SrcAddr=(uint32_t)&scan.potCommands[pot];
	lli[lliPos][0].DstAddr=(uint32_t)&LPC_SSP2->DR;
	lli[lliPos][0].NextLLI=(uint32_t)&lli[lliPos][1];
	lli[lliPos][0].Control=
		GPDMA_DMACCxControl_TransferSize(1) |
		GPDMA_DMACCxControl_SWidth(1) |
		GPDMA_DMACCxControl_DWidth(1);

	lli[lliPos][1].SrcAddr=(uint32_t)&LPC_SSP2->DR;
	lli[lliPos][1].DstAddr=(uint32_t)&scan.potSamples[(lliPos+POT_SAMPLES*SCAN_POT_COUNT-1)%(POT_SAMPLES*SCAN_POT_COUNT)];
	lli[lliPos][1].NextLLI=(uint32_t)&lli[(lliPos+1)%(POT_SAMPLES*SCAN_POT_COUNT)][0];
	lli[lliPos][1].Control=
		GPDMA_DMACCxControl_TransferSize(1) |
		GPDMA_DMACCxControl_SWidth(1) |
		GPDMA_DMACCxControl_DWidth(1);
}

static void readPots(void)
{
	uint16_t new;
	int pot;
	uint16_t tmpSmp[POT_SAMPLES];

	if(scan.pendingSPIFlush)
	{
		// ensure the FIFO stays empty for proper lli function
		while(SSP_GetStatus(LPC_SSP2,SSP_STAT_RXFIFO_NOTEMPTY)==SET)
		{
			// flush received data
			SSP_ReceiveData(LPC_SSP2);
		}
		
		// enough for a full loop of the llis
		delay_ms(20);
		
		scan.pendingSPIFlush=0;
	}
	
	// read pots from TLV2556 ADC

	for(pot=0;pot<SCAN_POT_COUNT;++pot)
	{
		// convert 10 bits samples to 0..999 and back to 16 bits full scale

		for(int smp=0;smp<POT_SAMPLES;++smp)
		{
			tmpSmp[smp]=scan.potSamples[smp*SCAN_POT_COUNT+pot];
			tmpSmp[smp]=SCAN_POT_TO_16BITS(MAX(0,MIN(SCAN_POT_MAX_VALUE,tmpSmp[smp]-12)));
		}
		
		// sort values

		qsort(tmpSmp,POT_SAMPLES,sizeof(uint16_t),uint16Compare);
		
		// median
	
		new=tmpSmp[POT_SAMPLES/2];
		
		// ignore small changes

		if(abs(new-scan.potValue[pot])>=POT_UNLOCK_THRESHOLD || currentTick<scan.potLockTimeout[pot])
		{
			if(currentTick>=scan.potLockTimeout[pot])
			{
				// out of lock -> current value must be taken into account
				if(scan.eventCallback) scan.eventCallback(-pot-1);
			}			
			
			if(abs(new-scan.potLockValue[pot])>=POT_TIMEOUT_THRESHOLD)
			{
				scan.potLockTimeout[pot]=currentTick+POT_TIMEOUT;
				scan.potLockValue[pot]=new;
			}

			scan.potValue[pot]=new;
			
			if(scan.eventCallback) scan.eventCallback(-pot-1);
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
	
	for(key=0;key<kbCount;++key)
	{
		newState=(col[keypadButtonCode[key]>>4]&keypadButtonCode[key])?1:0;
	
		if(newState && scan.keypadState[key])
		{
			scan.keypadState[key]=MIN(INT8_MAX,scan.keypadState[key]+1);

			if(scan.keypadState[key]==KEYPAD_DEBOUNCE_THRESHOLD)
				if(scan.eventCallback) scan.eventCallback(key);
		}
		else
		{
			scan.keypadState[key]=newState;
		}
	}
}

static uint16_t readADC(uint16_t channel)
{
	while(SSP_GetStatus(LPC_SSP2,SSP_STAT_TXFIFO_EMPTY)==RESET)
	{
		// wait until previous data is transferred
	}
	
	SSP_SendData(LPC_SSP2,channel<<(SCAN_ADC_BITS-4));

	while(SSP_GetStatus(LPC_SSP2,SSP_STAT_RXFIFO_NOTEMPTY)==RESET)
	{
		// wait for new data
	}

	uint16_t res=SSP_ReceiveData(LPC_SSP2);
	
	// wait TLV2556 tConvert
	
	delay_us(6);

	return res;
}

void scan_setMode(int8_t isSmpMasterMixMode)
{
	TIM_TIMERCFG_Type tim;
	TIM_MATCHCFG_Type tm;

	// reset
	TIM_Cmd(LPC_TIM2,DISABLE);
	LPC_GPDMACH1->CConfig=0;
	SSP_Cmd(LPC_SSP2,DISABLE);

	// init SSP
	SSP_CFG_Type SSP_ConfigStruct;
	SSP_ConfigStructInit(&SSP_ConfigStruct);
	SSP_ConfigStruct.Databit=SSP_CR0_DSS(SCAN_ADC_BITS);
	SSP_ConfigStruct.ClockRate=isSmpMasterMixMode?MIXSCAN_SPI_FREQUENCY:POTSCAN_SPI_FREQUENCY;
	SSP_Init(LPC_SSP2,&SSP_ConfigStruct);
	SSP_Cmd(LPC_SSP2,ENABLE);

	// config CFGR2 for external reference

	SSP_SendData(LPC_SSP2,0b11111100<<(SCAN_ADC_BITS-8));

	while(SSP_GetStatus(LPC_SSP2,SSP_STAT_TXFIFO_EMPTY)==RESET ||
			SSP_GetStatus(LPC_SSP2,SSP_STAT_RXFIFO_NOTEMPTY)==RESET)
	{
		// wait until previous sent data is transferred & wait for new received data
	}

	while(SSP_GetStatus(LPC_SSP2,SSP_STAT_RXFIFO_NOTEMPTY)==SET)
	{
		// flush received data
		SSP_ReceiveData(LPC_SSP2);
	}

	if(isSmpMasterMixMode)
	{
		// init timer
		tim.PrescaleOption=TIM_PRESCALE_TICKVAL;
		tim.PrescaleValue=1;

		tm.MatchChannel=0;
		tm.IntOnMatch=ENABLE;
		tm.ResetOnMatch=ENABLE;
		tm.StopOnMatch=DISABLE;
		tm.ExtMatchOutputType=0;
		tm.MatchValue=DACSPI_TICK_RATE-1; // same rate as dacspi
	}
	else
	{
		// init timer
		tim.PrescaleOption=TIM_PRESCALE_TICKVAL;
		tim.PrescaleValue=POTSCAN_PRE_DIV-1;

		tm.MatchChannel=0;
		tm.IntOnMatch=DISABLE;
		tm.ResetOnMatch=ENABLE;
		tm.StopOnMatch=DISABLE;
		tm.ExtMatchOutputType=0;
		tm.MatchValue=SYNTH_MASTER_CLOCK/POTSCAN_PRE_DIV/(SCAN_POT_COUNT*POTSCAN_FREQUENCY)-1;

		// init GPDMA channel
		LPC_GPDMACH1->CSrcAddr=lli[0][0].SrcAddr;
		LPC_GPDMACH1->CDestAddr=lli[0][0].DstAddr;
		LPC_GPDMACH1->CLLI=lli[0][0].NextLLI;
		LPC_GPDMACH1->CControl=lli[0][0].Control;

		LPC_GPDMACH1->CConfig=POTSCAN_DMACONFIG;
		
		scan.pendingSPIFlush=1;
	}

	TIM_Init(LPC_TIM2,TIM_TIMER_MODE,&tim);
	TIM_ConfigMatch(LPC_TIM2,&tm);
	TIM_ClearIntPending(LPC_TIM2,TIM_MR0_INT);
	TIM_Cmd(LPC_TIM2,ENABLE);
}

void scan_sampleMasterMix(uint16_t sampleCount, uint8_t * buffer)
{
	int32_t mini=UINT16_MAX,maxi=0,extents;
	uint8_t *buf;
	
	// ensure no spurious reads from other channels
	
	readADC(MIXSCAN_ADC_CHANNEL);
	readADC(MIXSCAN_ADC_CHANNEL);
	
	// sample master mix at dacspi tickrate

	buf=buffer;
	for(uint16_t sc=0;sc<sampleCount;++sc)
	{
		while(!(LPC_TIM2->IR&TIM_IR_CLR(TIM_MR0_INT)))
			/* nothing */;
		
		TIM_ClearIntPending(LPC_TIM2,TIM_MR0_INT);
		
		uint8_t sample=readADC(MIXSCAN_ADC_CHANNEL)>>(SCAN_ADC_BITS-8);
		
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
		uint8_t sample=*buf;
		
		sample=(((int32_t)sample-mini)<<8)/extents;
		
		*buf++=sample;
	}
}

uint16_t scan_getPotValue(int8_t pot)
{
	return scan.potValue[pot];
}

void scan_resetPotLocking(void)
{
	memset(&scan.potLockTimeout[0],0,SCAN_POT_COUNT*sizeof(uint32_t));
}

void scan_setScanEventCallback(scan_event_callback_t callback)
{
	scan.eventCallback=callback;
}

void scan_init(void)
{
	memset(&scan,0,sizeof(scan));
	
	// prepare LLIs

	for(int pot=0;pot<SCAN_POT_COUNT;++pot)
	{
		scan.potCommands[pot]=pot<<(SCAN_ADC_BITS-4);
		for(int smp=0;smp<POT_SAMPLES;++smp)
			buildLLIs(pot,smp);
	}

	// init TLV2556 ADC
	
	PINSEL_ConfigPin(1,POTSCAN_PIN_CS,4); // CS
	PINSEL_ConfigPin(1,POTSCAN_PIN_DOUT,4); // DOUT
	PINSEL_ConfigPin(1,POTSCAN_PIN_DIN,4); // DIN
	PINSEL_ConfigPin(1,POTSCAN_PIN_CLK,4); // CLK
	
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
	
	// init pot scan TMR / DMA
	
	CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCGPDMA,ENABLE);

	LPC_SC->DMAREQSEL|=1<<DMA_CHANNEL_SSP1_TX__T2_MAT_0;
//	LPC_GPDMA->DMACConfig=GPDMA_DMACConfig_E;

	// start
	
	scan_setMode(0);	
	
	rprintf(0,"pots scan at %d Hz, spi %d Hz\n",POTSCAN_FREQUENCY,POTSCAN_SPI_FREQUENCY);
 }

void scan_update(void)
{
	readKeypad();
	readPots();
}
