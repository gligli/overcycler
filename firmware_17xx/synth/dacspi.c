///////////////////////////////////////////////////////////////////////////////
// 12bit voice DACs communication through SPI
///////////////////////////////////////////////////////////////////////////////

#include "dacspi.h"
#include "LPC17xx.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_gpio.h"

#define SPIMUX_PORT_ABC 0
#define SPIMUX_PIN_A 15
#define SPIMUX_PIN_B 16
#define SPIMUX_PIN_C 5

#define SPIMUX_MASK ((1<<SPIMUX_PIN_A)|(1<<SPIMUX_PIN_B)|(1<<SPIMUX_PIN_C))

#define DACSPI_CMD_SET_A 0x7000
#define DACSPI_CMD_SET_B 0xf000
#define DACSPI_CMD_SET_REF 0x7000

#define DACSPI_DMACONFIG \
			GPDMA_DMACCxConfig_E | \
			GPDMA_DMACCxConfig_DestPeripheral(14) | \
			GPDMA_DMACCxConfig_TransferType(1) | \
			GPDMA_DMACCxConfig_ITC

#define DACSPI_BUFFER_COUNT 64
#define DACSPI_CV_COUNT 16

#define DACSPI_CV_CHANNEL 6
#define DACSPI_CARD_CV_LDAC_MASK 0xbf
#define DACSPI_VCA_CV_LDAC_MASK 0xef

static EXT_RAM GPDMA_LLI_Type lli[DACSPI_BUFFER_COUNT*DACSPI_CHANNEL_COUNT][4];
static EXT_RAM volatile uint8_t marker;
static EXT_RAM uint8_t markerSource[DACSPI_BUFFER_COUNT];

static const uint8_t channelCPSR[DACSPI_CHANNEL_COUNT] =
{
	4,4,4,4,4,4,8
};

static const uint8_t channelWaitStates[DACSPI_CHANNEL_COUNT] =
{
	4,4,4,4,4,5,10 // change DACSPI_WAIT_STATES_COUNT accordingly (sum of this array)
};

static uint32_t spiMuxCommands[DACSPI_CHANNEL_COUNT*2][3] =
{
	{(0<<SPIMUX_PIN_C)|(1<<SPIMUX_PIN_B)|(1<<SPIMUX_PIN_A), (uint32_t)&LPC_GPIO0->FIOCLR, 4},
	{(0<<SPIMUX_PIN_C)|(0<<SPIMUX_PIN_B)|(1<<SPIMUX_PIN_A), (uint32_t)&LPC_GPIO0->FIOSET, 5},
	{(1<<SPIMUX_PIN_C)|(0<<SPIMUX_PIN_B)|(1<<SPIMUX_PIN_A), (uint32_t)&LPC_GPIO0->FIOCLR, 0},
	{(0<<SPIMUX_PIN_C)|(0<<SPIMUX_PIN_B)|(1<<SPIMUX_PIN_A), (uint32_t)&LPC_GPIO0->FIOSET, 1},
	{(0<<SPIMUX_PIN_C)|(1<<SPIMUX_PIN_B)|(0<<SPIMUX_PIN_A), (uint32_t)&LPC_GPIO0->FIOSET, 3},
	{(0<<SPIMUX_PIN_C)|(0<<SPIMUX_PIN_B)|(1<<SPIMUX_PIN_A), (uint32_t)&LPC_GPIO0->FIOCLR, 2},
	{(1<<SPIMUX_PIN_C)|(0<<SPIMUX_PIN_B)|(0<<SPIMUX_PIN_A), (uint32_t)&LPC_GPIO0->FIOSET, 6},
	{(0<<SPIMUX_PIN_C)|(1<<SPIMUX_PIN_B)|(1<<SPIMUX_PIN_A), (uint32_t)&LPC_GPIO0->FIOCLR, 4},
	{(0<<SPIMUX_PIN_C)|(0<<SPIMUX_PIN_B)|(1<<SPIMUX_PIN_A), (uint32_t)&LPC_GPIO0->FIOSET, 5},
	{(1<<SPIMUX_PIN_C)|(0<<SPIMUX_PIN_B)|(1<<SPIMUX_PIN_A), (uint32_t)&LPC_GPIO0->FIOCLR, 0},
	{(0<<SPIMUX_PIN_C)|(0<<SPIMUX_PIN_B)|(1<<SPIMUX_PIN_A), (uint32_t)&LPC_GPIO0->FIOSET, 1},
	{(0<<SPIMUX_PIN_C)|(1<<SPIMUX_PIN_B)|(0<<SPIMUX_PIN_A), (uint32_t)&LPC_GPIO0->FIOSET, 3},
	{(0<<SPIMUX_PIN_C)|(0<<SPIMUX_PIN_B)|(1<<SPIMUX_PIN_A), (uint32_t)&LPC_GPIO0->FIOCLR, 2},
	{(1<<SPIMUX_PIN_C)|(0<<SPIMUX_PIN_B)|(1<<SPIMUX_PIN_A), (uint32_t)&LPC_GPIO0->FIOSET, 7},
};

static EXT_RAM struct
{
	uint16_t voiceCommands[DACSPI_BUFFER_COUNT][SYNTH_VOICE_COUNT][2];
	uint16_t cvCommands[DACSPI_CV_COUNT];
} dacspi;


__attribute__ ((used)) void DMA_IRQHandler(void)
{
	int8_t secondHalfPlaying=(marker&0xff)>=DACSPI_BUFFER_COUNT/2;
	
	LPC_GPDMA->DMACIntTCClear=LPC_GPDMA->DMACIntTCStat;

	// when second half is playing, update first and vice-versa
	synth_updateDACsEvent(secondHalfPlaying?0:DACSPI_BUFFER_COUNT/2,DACSPI_BUFFER_COUNT/2);
}

void buildLLIs(int buffer, int channel)
{
	int lliPos=buffer*DACSPI_CHANNEL_COUNT+channel;
	int muxIndex=lliPos%(DACSPI_CHANNEL_COUNT*2);
	int muxChannel=spiMuxCommands[muxIndex][2];
	
	if(channel==DACSPI_CV_CHANNEL)
	{
		if(muxChannel==6)
			lli[lliPos][1].SrcAddr=(uint32_t)&dacspi.cvCommands[buffer&6];
		else
			lli[lliPos][1].SrcAddr=(uint32_t)&dacspi.cvCommands[8+(buffer&6)];
	}
	else
	{
		lli[lliPos][1].SrcAddr=(uint32_t)&dacspi.voiceCommands[buffer][muxChannel][0];
	}
	
	lli[lliPos][0].SrcAddr=(uint32_t)&spiMuxCommands[muxIndex][0];
	lli[lliPos][0].DstAddr=spiMuxCommands[muxIndex][1];
	lli[lliPos][0].NextLLI=(uint32_t)&lli[lliPos][1];
	lli[lliPos][0].Control=
		GPDMA_DMACCxControl_TransferSize(1) |
		GPDMA_DMACCxControl_SWidth(2) |
		GPDMA_DMACCxControl_DWidth(2);

	lli[lliPos][1].DstAddr=(uint32_t)&LPC_SSP0->DR;
	lli[lliPos][1].NextLLI=(uint32_t)&lli[lliPos][2];
	lli[lliPos][1].Control=
		GPDMA_DMACCxControl_TransferSize(2) |
		GPDMA_DMACCxControl_SWidth(1) |
		GPDMA_DMACCxControl_DWidth(1) |
		GPDMA_DMACCxControl_SI;

	lli[lliPos][2].SrcAddr=(uint32_t)&markerSource[buffer];
	lli[lliPos][2].DstAddr=(uint32_t)&marker;
	lli[lliPos][2].NextLLI=(uint32_t)&lli[lliPos][3];
	lli[lliPos][2].Control=
		GPDMA_DMACCxControl_TransferSize(channelWaitStates[channel]) |
		GPDMA_DMACCxControl_SWidth(0) |
		GPDMA_DMACCxControl_DWidth(0);

	lli[lliPos][3].SrcAddr=(uint32_t)&channelCPSR[(channel+1)%DACSPI_CHANNEL_COUNT];
	lli[lliPos][3].DstAddr=(uint32_t)&LPC_SSP0->CPSR;
	lli[lliPos][3].NextLLI=(uint32_t)&lli[(lliPos+1)%(DACSPI_BUFFER_COUNT*DACSPI_CHANNEL_COUNT)][0];
	lli[lliPos][3].Control=
		GPDMA_DMACCxControl_TransferSize(1) |
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
		dacspi.cvCommands[4]=value|(5<<12);
		break;
	case 6:
		dacspi.cvCommands[5]=value|(6<<12);
		break;
	case 7:
		dacspi.cvCommands[3]=value|(4<<12);
		break;
	case 8:
		dacspi.cvCommands[2]=value|(3<<12);
		break;
	case 14:
		dacspi.cvCommands[0]=value|(1<<12);
		break;
	case 15:
		dacspi.cvCommands[1]=value|(2<<12);
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

	LPC_SSP0->CPSR=8; // SYNTH_MASTER_CLOCK / n
	LPC_SSP0->CR0=0x0f; // 16Bit SPI(0,0)
	LPC_SSP0->CR1=2; // Enable
	LPC_SSP0->DMACR=3; // DMA

	// SSP pins

	PINSEL_SetPinFunc(1,20,3);
	PINSEL_SetPinFunc(1,21,3);
	PINSEL_SetPinFunc(1,24,3);
	
	LPC_GPIO1->FIODIR2|=0x20;
	LPC_GPIO1->FIOMASK2&=~0x20;
	
	// init SPI mux

	GPIO_SetDir(SPIMUX_PORT_ABC,1<<SPIMUX_PIN_A,1); // A
	GPIO_SetDir(SPIMUX_PORT_ABC,1<<SPIMUX_PIN_B,1); // B
	GPIO_SetDir(SPIMUX_PORT_ABC,1<<SPIMUX_PIN_C,1); // C
	LPC_GPIO0->FIOMASK&=~SPIMUX_MASK;
	LPC_GPIO0->FIOSET=SPIMUX_MASK;
	
	// prepare LLIs

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