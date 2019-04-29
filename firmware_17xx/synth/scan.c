////////////////////////////////////////////////////////////////////////////////
// Potnetiometers / keypad scanner
////////////////////////////////////////////////////////////////////////////////

#include "scan.h"

#include "ui.h"
#include "LPC17xx.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_gpio.h"

#define ADC_BITS 10
#define ADC_QUANTUM (65536 / (1<<(ADC_BITS)))

#define POT_SAMPLES 5
#define POT_THRESHOLD (ADC_QUANTUM*6)
#define POT_TIMEOUT_THRESHOLD (ADC_QUANTUM*3)
#define POT_TIMEOUT (TICKER_1S)

#define DEBOUNCE_THRESHOLD 5

#define POTSCAN_PIN_CS (24-24)
#define POTSCAN_PIN_OUT (25-24)
#define POTSCAN_PIN_ADDR (26-24)
#define POTSCAN_PIN_CLK (27-24)

#define POTSCAN_MASK ((1<<POTSCAN_PIN_CS)|(1<<POTSCAN_PIN_ADDR)|(1<<POTSCAN_PIN_CLK))

#define POTSCAN_PRE_DIV 12
#define POTSCAN_RATE 500

#define POTSCAN_TICKS (16*2+10) // 32 writes, 10 reads
#define POTSCAN_LLI_PER_POT 21

#define POTSCAN_DMACONFIG \
		GPDMA_DMACCxConfig_E | \
		GPDMA_DMACCxConfig_SrcPeripheral(12) | \
		GPDMA_DMACCxConfig_TransferType(2)

static struct
{
	int16_t curPotSample;

	uint8_t potCommands[POT_COUNT][22];
	uint8_t potBits[POT_COUNT][ADC_BITS];
	
	uint16_t potSamples[POT_COUNT][POT_SAMPLES];
	uint16_t potValue[POT_COUNT];
	uint32_t potLockTimeout[POT_COUNT];
	uint32_t potLockValue[POT_COUNT];

	int8_t keypadState[16];
} scan EXT_RAM;

static const uint8_t keypadButtonCode[16]=
{
	0x01,0x02,0x04,0x11,0x12,0x14,0x21,0x22,0x24,0x32,
	0x08,0x18,0x28,0x38,
	0x34,0x31
};

static const uint8_t potCommandsConst[POT_COUNT][sizeof(scan.potCommands[0])]=
{
#define C0O0 0
#define C0O1 (1<<POTSCAN_PIN_ADDR)
#define C1O0 (1<<POTSCAN_PIN_CLK)
#define C1O1 ((1<<POTSCAN_PIN_CLK) | (1<<POTSCAN_PIN_ADDR))
	
#define ONE_POT(pot) { \
		((pot)&8)?C1O1:C1O0,((pot)&8)?C0O1:C0O0, \
		((pot)&4)?C1O1:C1O0,((pot)&4)?C0O1:C0O0, \
		((pot)&2)?C1O1:C1O0,((pot)&2)?C0O1:C0O0, \
		((pot)&1)?C1O1:C1O0,((pot)&1)?C0O1:C0O0, \
		C1O0,C0O0,C1O0,C0O0,C1O0,C0O0,C1O0,C0O0,C1O0,C0O0,C1O0,C0O0,C1O0,C0O0 \
	},
	
	ONE_POT(0) ONE_POT(1) ONE_POT(2) ONE_POT(3) ONE_POT(4) ONE_POT(5) ONE_POT(6) ONE_POT(7) ONE_POT(8) ONE_POT(9)
		
#undef ONE_POT
};

static const GPDMA_LLI_Type scanLLIs[POT_COUNT][POTSCAN_LLI_PER_POT]=
{
#define NEXTPOT(pot,tck) (((pot)*POTSCAN_LLI_PER_POT+(tck)+1)/POTSCAN_LLI_PER_POT)	
#define ONE_LLI(src,dst,siz,pot,tck,flag) { \
		(src), \
		(dst), \
		(uint32_t)&scanLLIs[NEXTPOT(pot,tck)%POT_COUNT][((tck)+1)%POTSCAN_LLI_PER_POT], \
		GPDMA_DMACCxControl_TransferSize(siz)|GPDMA_DMACCxControl_SI|(flag) \
	},
	
#define ONE_TICK(idx,wrt,siz,pot,tck) ONE_LLI( \
		(wrt)?((uint32_t)&scan.potCommands[pot][idx]):((uint32_t)&LPC_GPIO0->FIOPIN3), \
		(wrt)?((uint32_t)&LPC_GPIO0->FIOPIN3):((uint32_t)&scan.potBits[(pot+POT_COUNT-1)%POT_COUNT][idx]), \
		siz, pot, tck, \
		(wrt)?0:GPDMA_DMACCxControl_Prot3 \
	)
	
#define ONE_POT(pot) { \
		ONE_TICK(0,1,2,pot,0) ONE_TICK(0,0,1,pot,1) \
		ONE_TICK(2,1,2,pot,2) ONE_TICK(1,0,1,pot,3) \
		ONE_TICK(4,1,2,pot,4) ONE_TICK(2,0,1,pot,5) \
		ONE_TICK(6,1,2,pot,6) ONE_TICK(3,0,1,pot,7) \
		ONE_TICK(8,1,2,pot,8) ONE_TICK(4,0,1,pot,9) \
		ONE_TICK(8,1,2,pot,10) ONE_TICK(5,0,1,pot,11) \
		ONE_TICK(8,1,2,pot,12) ONE_TICK(6,0,1,pot,13) \
		ONE_TICK(8,1,2,pot,14) ONE_TICK(7,0,1,pot,15) \
		ONE_TICK(8,1,2,pot,16) ONE_TICK(8,0,1,pot,17) \
		ONE_TICK(8,1,2,pot,18) ONE_TICK(9,0,1,pot,19) \
		ONE_TICK(8,1,12,pot,20) \
	},
	
	ONE_POT(0) ONE_POT(1) ONE_POT(2) ONE_POT(3) ONE_POT(4) ONE_POT(5) ONE_POT(6) ONE_POT(7) ONE_POT(8) ONE_POT(9)
};

static void readPots(void)
{
	uint16_t new;
	int i,pot;
	uint16_t tmp[POT_SAMPLES];
	
	scan.curPotSample=(scan.curPotSample+1)%POT_SAMPLES;
	
	for(pot=0;pot<POT_COUNT;++pot)
	{
		// read pot from TLV1543 ADC (scanned using GPDMACH1)

		new=0;
		for(i=0;i<ADC_BITS;++i)
			new|=(((uint32_t)scan.potBits[pot][i]&(1<<POTSCAN_PIN_OUT))>>POTSCAN_PIN_OUT)<<(ADC_BITS-1-i);

		scan.potSamples[pot][scan.curPotSample]=(MAX(0,MIN(999,new-12))*UINT16_MAX)/999;
		
		// sort values

		memcpy(&tmp[0],&scan.potSamples[pot][0],POT_SAMPLES*sizeof(uint16_t));
		qsort(tmp,POT_SAMPLES,sizeof(uint16_t),uint16Compare);		
		
		// median
	
		new=tmp[POT_SAMPLES/2];
		
		// ignore small changes

		if(abs(new-scan.potValue[pot])>=POT_THRESHOLD || currentTick<scan.potLockTimeout[pot])
		{
			if(abs(new-scan.potLockValue[pot])>=POT_TIMEOUT_THRESHOLD)
			{
				scan.potLockTimeout[pot]=currentTick+POT_TIMEOUT;
				scan.potLockValue[pot]=new;
			}

			scan.potValue[pot]=new;
			
			ui_scanEvent(-pot-1);
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
		GPIO_SetValue(0,0b1111<<19);
		GPIO_ClearValue(0,(8>>row)<<19);
		delay_us(10);
		col[row]=0;
		col[row]|=((GPIO_ReadValue(0)>>10)&1)?0:1;
		col[row]|=((GPIO_ReadValue(4)>>29)&1)?0:2;
		col[row]|=((GPIO_ReadValue(4)>>28)&1)?0:4;
		col[row]|=((GPIO_ReadValue(2)>>13)&1)?0:8;
	}
	
	for(key=0;key<16;++key)
	{
		newState=(col[keypadButtonCode[key]>>4]&keypadButtonCode[key])?1:0;
	
		if(newState && scan.keypadState[key])
		{
			scan.keypadState[key]=MIN(INT8_MAX,scan.keypadState[key]+1);

			if(scan.keypadState[key]==DEBOUNCE_THRESHOLD)
				ui_scanEvent(key);
		}
		else
		{
			scan.keypadState[key]=newState;
		}
	}
}

uint16_t scan_getPotValue(int8_t pot)
{
	return scan.potValue[pot]&~(ADC_QUANTUM-1);
}

void scan_resetPotLocking(void)
{
	memset(&scan.potLockTimeout[0],0,POT_COUNT*sizeof(uint32_t));
}

void scan_init(void)
{
	memset(&scan,0,sizeof(scan));
	
	memcpy(scan.potCommands,potCommandsConst,sizeof(scan.potCommands));

	// init TLV1543 ADC
	
	PINSEL_SetI2C0Pins(PINSEL_I2C_Fast_Mode, DISABLE);
	
	PINSEL_SetPinFunc(0,24+POTSCAN_PIN_CS,0); // CS
	PINSEL_SetPinFunc(0,24+POTSCAN_PIN_OUT,0); // OUT
	PINSEL_SetPinFunc(0,24+POTSCAN_PIN_ADDR,0); // ADDR
	PINSEL_SetPinFunc(0,24+POTSCAN_PIN_CLK,0); // CLK
	PINSEL_SetPinFunc(0,24+4,0); // EOC
	
	GPIO_SetValue(0,((1<<POTSCAN_PIN_ADDR)|(1<<POTSCAN_PIN_CLK))<<24);
	GPIO_SetDir(0,POTSCAN_MASK<<24,1);
	LPC_GPIO0->FIOMASK3&=~POTSCAN_MASK;

	// init keypad
	
	PINSEL_SetPinFunc(0,22,0); // R1
	PINSEL_SetPinFunc(0,21,0); // R2
	PINSEL_SetPinFunc(0,20,0); // R3
	PINSEL_SetPinFunc(0,19,0); // R4
	PINSEL_SetPinFunc(0,10,0); // C1
	PINSEL_SetPinFunc(4,29,0); // C2
	PINSEL_SetPinFunc(4,28,0); // C3
	PINSEL_SetPinFunc(2,13,0); // C4
	PINSEL_SetResistorMode(0,10,PINSEL_PINMODE_PULLUP);
	PINSEL_SetResistorMode(4,29,PINSEL_PINMODE_PULLUP);
	PINSEL_SetResistorMode(4,28,PINSEL_PINMODE_PULLUP);
	PINSEL_SetResistorMode(2,13,PINSEL_PINMODE_PULLUP);

	GPIO_SetValue(0,0b1111ul<<19);
	GPIO_SetDir(0,0b1111ul<<19,1);
	
	// init pot scan TMR / DMA
	
	CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCGPDMA,ENABLE);

	DMAREQSEL|=0x10;
	LPC_GPDMA->DMACConfig=GPDMA_DMACConfig_E;

	TIM_TIMERCFG_Type tim;
	
	tim.PrescaleOption=TIM_PRESCALE_TICKVAL;
	tim.PrescaleValue=POTSCAN_PRE_DIV-1;
	
	TIM_Init(LPC_TIM2,TIM_TIMER_MODE,&tim);
	
	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_TIMER2,CLKPWR_PCLKSEL_CCLK_DIV_1);
	
	TIM_MATCHCFG_Type tm;
	
	tm.MatchChannel=0;
	tm.IntOnMatch=DISABLE;
	tm.ResetOnMatch=ENABLE;
	tm.StopOnMatch=DISABLE;
	tm.ExtMatchOutputType=0;
	tm.MatchValue=SYNTH_MASTER_CLOCK/POTSCAN_PRE_DIV/(POT_COUNT*POTSCAN_TICKS*POTSCAN_RATE);

	TIM_ConfigMatch(LPC_TIM2,&tm);
	
	// start
	
	TIM_Cmd(LPC_TIM2,ENABLE);
	
	// load first LLI and start GPDMA
	
	LPC_GPDMACH1->DMACCSrcAddr=scanLLIs[0][0].SrcAddr;
	LPC_GPDMACH1->DMACCDestAddr=scanLLIs[0][0].DstAddr;
	LPC_GPDMACH1->DMACCLLI=scanLLIs[0][0].NextLLI;
	LPC_GPDMACH1->DMACCControl=scanLLIs[0][0].Control;

	LPC_GPDMACH1->DMACCConfig=POTSCAN_DMACONFIG;

	rprintf(0,"pots scan at %d Hz, tick rate %d Hz\n",POTSCAN_RATE,SYNTH_MASTER_CLOCK/POTSCAN_PRE_DIV/tm.MatchValue);
}

void scan_update(void)
{
	readKeypad();
	readPots();
}
