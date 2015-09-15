///////////////////////////////////////////////////////////////////////////////
// 12bit voice DACs communication through SPI
///////////////////////////////////////////////////////////////////////////////

#include "dacspi.h"
#include "LPC17xx.h"
#include "lpc17xx_gpdma.h"

#define DACSPI_CMD_SET_A 0x7000
#define DACSPI_CMD_SET_B 0xf000
#define DACSPI_CMD_SET_REF 0x7000

#define DACSPI_DMACONFIG \
			GPDMA_DMACCxConfig_E | \
			GPDMA_DMACCxConfig_DestPeripheral(14) | \
			GPDMA_DMACCxConfig_TransferType(1) | \
			GPDMA_DMACCxConfig_ITC

#define DACSPI_BUFFER_COUNT 48
#define DACSPI_CV_COUNT 16

#define DACSPI_CV_CHANNEL_1 3
#define DACSPI_CV_CHANNEL_2 7
#define DACSPI_CARD_CV_LDAC_MASK 0xbf
#define DACSPI_VCA_CV_LDAC_MASK 0xef

static EXT_RAM GPDMA_LLI_Type lli[DACSPI_BUFFER_COUNT*DACSPI_CHANNEL_COUNT][3];
static EXT_RAM volatile uint8_t marker;
static EXT_RAM volatile uint8_t dummy;
static EXT_RAM uint8_t markerSource[DACSPI_BUFFER_COUNT];

static const uint32_t abcCommands[8] =
{
	((0<<CVMUX_PIN_A)|(1<<CVMUX_PIN_CARD0))>>16,
	((1<<CVMUX_PIN_A)|(1<<CVMUX_PIN_CARD0))>>16,
	((2<<CVMUX_PIN_A)|(1<<CVMUX_PIN_CARD0))>>16,
	((3<<CVMUX_PIN_A)|(1<<CVMUX_PIN_CARD0))>>16,
	((4<<CVMUX_PIN_A)|(1<<CVMUX_PIN_CARD0))>>16,
	((5<<CVMUX_PIN_A)|(1<<CVMUX_PIN_CARD0))>>16,
	((6<<CVMUX_PIN_A)|(1<<CVMUX_PIN_CARD0))>>16,
	((7<<CVMUX_PIN_A)|(1<<CVMUX_PIN_CARD0))>>16,
};

static EXT_RAM struct
{
	uint16_t voiceCommands[DACSPI_BUFFER_COUNT][SYNTH_VOICE_COUNT][2];
	uint16_t cvCommands[DACSPI_CV_COUNT];
} dacspi;


__attribute__ ((used)) void DMA_IRQHandler(void)
{
	int8_t secondHalfPlaying=marker>=DACSPI_BUFFER_COUNT/2;
	
	LPC_GPDMA->DMACIntTCClear=LPC_GPDMA->DMACIntTCStat;

	// when second half is playing, update first and vice-versa
	synth_updateDACsEvent(secondHalfPlaying?0:DACSPI_BUFFER_COUNT/2,DACSPI_BUFFER_COUNT/2);
}

void buildLLIs(int buffer, int channel)
{
	int lliPos=buffer*DACSPI_CHANNEL_COUNT+channel;
	
	if(channel==DACSPI_CV_CHANNEL_1)
	{
		lli[lliPos][1].SrcAddr=(uint32_t)&dacspi.cvCommands[(buffer&3)<<1];
	}
	else
	{
		lli[lliPos][1].SrcAddr=(uint32_t)&dacspi.voiceCommands[buffer][channel][0];
	}
	
	lli[lliPos][1].DstAddr=(uint32_t)&LPC_SSP0->DR;
	lli[lliPos][1].NextLLI=(uint32_t)&lli[lliPos][2];
	lli[lliPos][1].Control=
		GPDMA_DMACCxControl_TransferSize(2) |
		GPDMA_DMACCxControl_SWidth(1) |
		GPDMA_DMACCxControl_DWidth(1) |
		GPDMA_DMACCxControl_SI;

	lli[lliPos][0].SrcAddr=(uint32_t)&abcCommands[channel];
	lli[lliPos][0].DstAddr=(uint32_t)&LPC_GPIO0->FIOPIN2;
	lli[lliPos][0].NextLLI=(uint32_t)&lli[lliPos][1];
	lli[lliPos][0].Control=
		GPDMA_DMACCxControl_TransferSize(1) |
		GPDMA_DMACCxControl_SWidth(0) |
		GPDMA_DMACCxControl_DWidth(0);

	lli[lliPos][2].SrcAddr=(uint32_t)&markerSource[buffer];
	lli[lliPos][2].DstAddr=(uint32_t)&marker;
	lli[lliPos][2].NextLLI=(uint32_t)&lli[(lliPos+1)%(DACSPI_BUFFER_COUNT*DACSPI_CHANNEL_COUNT)][0];
	lli[lliPos][2].Control=
		GPDMA_DMACCxControl_TransferSize(DACSPI_STEP_COUNT) |
		GPDMA_DMACCxControl_SWidth(0) |
		GPDMA_DMACCxControl_DWidth(0);
}

FORCEINLINE void dacspi_setVoiceValue(int32_t buffer, int voice, int ab, uint16_t value)
{
	value>>=4;
	
	if(ab)
		dacspi.voiceCommands[buffer][voice][ab]=value|DACSPI_CMD_SET_B;
	else
		dacspi.voiceCommands[buffer][voice][ab]=value|DACSPI_CMD_SET_A;
}

FORCEINLINE void dacspi_setCVValue(uint16_t value, int channel)
{
	value>>=4;
	
	switch(channel)
	{
	case 0:
	case 1:
	case 2:
	case 3:
		dacspi.cvCommands[channel]=value|((channel+1)<<12);
		break;
	case 24:
		dacspi.cvCommands[4]=value|(5<<12);
		break;
	case 30:
		dacspi.cvCommands[5]=value|(6<<12);
		break;
	}
}

void dacspi_init(void)
{
	int i,j;
	
	memset(&dacspi,0,sizeof(dacspi));

	// SSP

	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_SSP0,CLKPWR_PCLKSEL_CCLK_DIV_1);
	CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCSSP0,ENABLE);

	LPC_SSP0->CPSR=8; // SYNTH_MASTER_CLOCK / 8
	LPC_SSP0->CR0=0x0f; // 16Bit SPI(0,0)
	LPC_SSP0->CR1=2; // Enable
	LPC_SSP0->DMACR=3; // DMA

	// SSP pins

	PINSEL_SetPinFunc(1,20,3);
	PINSEL_SetPinFunc(1,21,3);
	PINSEL_SetPinFunc(1,24,3);
	
	LPC_GPIO1->FIODIR2|=0x20;
	LPC_GPIO1->FIOMASK2&=~0x20;
	
	// prepare LLIs

	LPC_GPIO0->FIOMASK2=~(((7<<CVMUX_PIN_A)|(1<<CVMUX_PIN_CARD0))>>16);
	LPC_GPIO4->FIOMASK3=~(((1<<CVMUX_PIN_CARD2)|(1<<CVMUX_PIN_VCA))>>24);

	for(j=0;j<DACSPI_BUFFER_COUNT;++j)
	{
		markerSource[j]=j;
		for(i=0;i<DACSPI_CHANNEL_COUNT;++i)
			buildLLIs(j,i);
	}

	// interrupt triggers
	
	lli[DACSPI_BUFFER_COUNT*DACSPI_CHANNEL_COUNT*1/16][0].Control|=GPDMA_DMACCxControl_I;
	lli[DACSPI_BUFFER_COUNT*DACSPI_CHANNEL_COUNT*9/16][0].Control|=GPDMA_DMACCxControl_I;
	
	//
	
	CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCGPDMA,ENABLE);

	DMAREQSEL=0x40;
	LPC_GPDMA->DMACConfig=GPDMA_DMACConfig_E;

	TIM_TIMERCFG_Type tim;
	
	tim.PrescaleOption=TIM_PRESCALE_TICKVAL;
	tim.PrescaleValue=1;
	
	TIM_Init(LPC_TIM3,TIM_TIMER_MODE,&tim);
	
	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_TIMER3,CLKPWR_PCLKSEL_CCLK_DIV_1);
	
	TIM_MATCHCFG_Type tm;
	
	tm.MatchChannel=0;
	tm.IntOnMatch=DISABLE;
	tm.ResetOnMatch=ENABLE;
	tm.StopOnMatch=DISABLE;
	tm.ExtMatchOutputType=0;
	tm.MatchValue=DACSPI_TIMER_MATCH;

	TIM_ConfigMatch(LPC_TIM3,&tm);
	
	NVIC_SetPriority(DMA_IRQn,0);
	NVIC_EnableIRQ(DMA_IRQn);

	// start
	
	TIM_Cmd(LPC_TIM3,ENABLE);
	
	LPC_GPDMACH0->DMACCSrcAddr=lli[0][0].SrcAddr;
	LPC_GPDMACH0->DMACCDestAddr=lli[0][0].DstAddr;
	LPC_GPDMACH0->DMACCLLI=lli[0][0].NextLLI;
	LPC_GPDMACH0->DMACCControl=lli[0][0].Control;

	LPC_GPDMACH0->DMACCConfig=DACSPI_DMACONFIG;
	
	rprintf(0,"sampling at %d Hz\n",SYNTH_MASTER_CLOCK/DACSPI_TICK_RATE);
}