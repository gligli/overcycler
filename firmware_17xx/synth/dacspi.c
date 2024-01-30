///////////////////////////////////////////////////////////////////////////////
// 12bit voice DACs communication through SPI
///////////////////////////////////////////////////////////////////////////////

#include "dacspi.h"

#include "LPC177x_8x.h"
#include "lpc177x_8x_gpdma.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_pinsel.h"

#define DMA_CHANNEL_UART2_TX__T3_MAT_1 14

#define SPIMUX_PORT_ABC 0
#define SPIMUX_PIN_A 15
#define SPIMUX_PIN_B 16
#define SPIMUX_PIN_C 5

#define SPIMUX_VAL(c,b,a) (((a)<<SPIMUX_PIN_A)|((b)<<SPIMUX_PIN_B)|((c)<<SPIMUX_PIN_C))

#define DACSPI_CMD_SET_A 0x3000
#define DACSPI_CMD_SET_B 0xb000

#define DACSPI_DMACONFIG \
		GPDMA_DMACCxConfig_E | \
		GPDMA_DMACCxConfig_DestPeripheral(DMA_CHANNEL_UART2_TX__T3_MAT_1) | \
		GPDMA_DMACCxConfig_TransferType(1) | \
		GPDMA_DMACCxConfig_ITC

#define DACSPI_BUFFER_COUNT 32
#define DACSPI_CV_COUNT 16

#define DACSPI_CV_CHANNEL 6

static EXT_RAM GPDMA_LLI_Type lli[DACSPI_BUFFER_COUNT*DACSPI_CHANNEL_COUNT][3];
static EXT_RAM volatile uint8_t marker;
static EXT_RAM uint8_t markerSource[DACSPI_BUFFER_COUNT];

static const uint8_t channelWaitStates[DACSPI_CHANNEL_COUNT] =
{
	4,4,4,4,4,4,6 // change DACSPI_WAIT_STATES_COUNT accordingly (sum of this array)
};

static const uint32_t spiMuxCommandsConst[DACSPI_CHANNEL_COUNT*2][3] =
{
	{SPIMUX_VAL(0,1,0),(uint32_t)&LPC_GPIO0->FIOCLR,5},
	{SPIMUX_VAL(1,0,0),(uint32_t)&LPC_GPIO0->FIOCLR,1},
	{SPIMUX_VAL(0,1,0),(uint32_t)&LPC_GPIO0->FIOSET,3},
	{SPIMUX_VAL(0,0,1),(uint32_t)&LPC_GPIO0->FIOCLR,2},
	{SPIMUX_VAL(1,0,0),(uint32_t)&LPC_GPIO0->FIOSET,6},
	{SPIMUX_VAL(0,1,0),(uint32_t)&LPC_GPIO0->FIOCLR,4},
	{SPIMUX_VAL(1,0,0),(uint32_t)&LPC_GPIO0->FIOCLR,0},
	{SPIMUX_VAL(1,0,1),(uint32_t)&LPC_GPIO0->FIOSET,5},
	{SPIMUX_VAL(1,0,0),(uint32_t)&LPC_GPIO0->FIOCLR,1},
	{SPIMUX_VAL(0,1,0),(uint32_t)&LPC_GPIO0->FIOSET,3},
	{SPIMUX_VAL(0,0,1),(uint32_t)&LPC_GPIO0->FIOCLR,2},
	{SPIMUX_VAL(1,0,0),(uint32_t)&LPC_GPIO0->FIOSET,6},
	{SPIMUX_VAL(0,1,0),(uint32_t)&LPC_GPIO0->FIOCLR,4},
	{SPIMUX_VAL(0,1,1),(uint32_t)&LPC_GPIO0->FIOSET,7},
};


static const uint32_t oscChannelCommand[SYNTH_VOICE_COUNT*2] = 
{
	DACSPI_CMD_SET_A,DACSPI_CMD_SET_B,
	DACSPI_CMD_SET_A,DACSPI_CMD_SET_B,
	DACSPI_CMD_SET_A,DACSPI_CMD_SET_B,
	DACSPI_CMD_SET_A,DACSPI_CMD_SET_B,
	DACSPI_CMD_SET_A,DACSPI_CMD_SET_B,
	DACSPI_CMD_SET_A,DACSPI_CMD_SET_B,
};

static struct
{
	uint16_t oscCommands[DACSPI_BUFFER_COUNT][SYNTH_VOICE_COUNT*2];
	uint16_t cvCommands[DACSPI_CV_COUNT];
	uint32_t spiMuxCommands[DACSPI_CHANNEL_COUNT*2][3];
} dacspi EXT_RAM;


__attribute__ ((used)) void DMA_IRQHandler(void)
{
	LPC_GPDMA->IntTCClear=LPC_GPDMA->IntTCStat; // acknowledge interrupt

	int8_t secondHalfPlaying=marker>=DACSPI_BUFFER_COUNT/2;
	
	// update synth @ 5Khz
	synth_updateCVsEvent();

	// when second half is playing, update first and vice-versa
	synth_updateDACsEvent(secondHalfPlaying?0:DACSPI_BUFFER_COUNT/2,DACSPI_BUFFER_COUNT/2);
}

void buildLLIs(int buffer, int channel)
{
	int lliPos=buffer*DACSPI_CHANNEL_COUNT+channel;
	int muxIndex=lliPos%(DACSPI_CHANNEL_COUNT*2);
	int muxChannel=dacspi.spiMuxCommands[muxIndex][2];
	
	lli[lliPos][0].SrcAddr=(uint32_t)&dacspi.spiMuxCommands[muxIndex][0];
	lli[lliPos][0].DstAddr=dacspi.spiMuxCommands[muxIndex][1];
	lli[lliPos][0].NextLLI=(uint32_t)&lli[lliPos][1];
	lli[lliPos][0].Control=
		GPDMA_DMACCxControl_TransferSize(1) |
		GPDMA_DMACCxControl_SWidth(2) |
		GPDMA_DMACCxControl_DWidth(2);

	lli[lliPos][1].DstAddr=(uint32_t)&LPC_SSP0->DR;
	lli[lliPos][1].NextLLI=(uint32_t)&lli[lliPos][2];
	
	if(muxChannel==0 || muxChannel==7)
	{
		if(muxChannel==0)
			lli[lliPos][1].SrcAddr=(uint32_t)&dacspi.cvCommands[(buffer>>1)&7];
		else
			lli[lliPos][1].SrcAddr=(uint32_t)&dacspi.cvCommands[8+((buffer>>1)&7)];
		lli[lliPos][1].Control=
			GPDMA_DMACCxControl_TransferSize(1) |
			GPDMA_DMACCxControl_SWidth(1) |
			GPDMA_DMACCxControl_DWidth(1);
	}
	else
	{
		lli[lliPos][1].SrcAddr=(uint32_t)&dacspi.oscCommands[buffer][(muxChannel-1)*2];
		lli[lliPos][1].Control=
			GPDMA_DMACCxControl_TransferSize(2) |
			GPDMA_DMACCxControl_SWidth(1) |
			GPDMA_DMACCxControl_DWidth(1) |
			GPDMA_DMACCxControl_SI;
	}
	
	lli[lliPos][2].SrcAddr=(uint32_t)&markerSource[buffer];
	lli[lliPos][2].DstAddr=(uint32_t)&marker;
	lli[lliPos][2].NextLLI=(uint32_t)&lli[(lliPos+1)%(DACSPI_BUFFER_COUNT*DACSPI_CHANNEL_COUNT)][0];
	lli[lliPos][2].Control=
		GPDMA_DMACCxControl_TransferSize(channelWaitStates[channel]) |
		GPDMA_DMACCxControl_SWidth(0) |
		GPDMA_DMACCxControl_DWidth(0);
}

FORCEINLINE void dacspi_setOscValue(int32_t buffer, int channel, uint16_t value)
{
	dacspi.oscCommands[buffer][channel]=(value>>4)|oscChannelCommand[channel];
}

FORCEINLINE void dacspi_setCVValue(int channel, uint16_t value)
{
	dacspi.cvCommands[channel]=(value>>4)|((channel&7)<<12);
}

void dacspi_init(void)
{
	int i,j;
	
	memset(&dacspi,0,sizeof(dacspi));

	memcpy(dacspi.spiMuxCommands,spiMuxCommandsConst,sizeof(spiMuxCommandsConst));
	
	// init SPI mux

	GPIO_SetDir(SPIMUX_PORT_ABC,1<<SPIMUX_PIN_A,1); // A
	GPIO_SetDir(SPIMUX_PORT_ABC,1<<SPIMUX_PIN_B,1); // B
	GPIO_SetDir(SPIMUX_PORT_ABC,1<<SPIMUX_PIN_C,1); // C
	LPC_GPIO0->FIOMASK&=~SPIMUX_VAL(1,1,1);
	LPC_GPIO0->FIOSET=SPIMUX_VAL(1,1,1);
	
	// SSP pins

	PINSEL_ConfigPin(1,20,5);
	PINSEL_ConfigPin(1,21,3);
	PINSEL_ConfigPin(1,24,5);
	
	LPC_GPIO1->FIODIR2|=0x20;
	LPC_GPIO1->FIOMASK2&=~0x20;
	
	// SSP

	SSP_CFG_Type SSP_ConfigStruct;
	SSP_ConfigStructInit(&SSP_ConfigStruct);
	SSP_ConfigStruct.Databit=SSP_DATABIT_16;
	SSP_ConfigStruct.ClockRate=30000000;
	SSP_Init(LPC_SSP0,&SSP_ConfigStruct);
	SSP_DMACmd(LPC_SSP0,SSP_DMA_TX,ENABLE);
	SSP_Cmd(LPC_SSP0,ENABLE);

	// prepare LLIs

	for(j=0;j<DACSPI_BUFFER_COUNT;++j)
	{
		markerSource[j]=j;
		for(i=0;i<DACSPI_CHANNEL_COUNT;++i)
			buildLLIs(j,i);
	}

	// interrupt triggers
	
	lli[(1)*DACSPI_CHANNEL_COUNT][0].Control|=GPDMA_DMACCxControl_I;
	lli[(DACSPI_BUFFER_COUNT/2+1)*DACSPI_CHANNEL_COUNT][0].Control|=GPDMA_DMACCxControl_I;
	
	//
	
	CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCGPDMA,ENABLE);

	LPC_SC->DMAREQSEL|=1<<DMA_CHANNEL_UART2_TX__T3_MAT_1;
	LPC_GPDMA->Config=GPDMA_DMACConfig_E;

	TIM_TIMERCFG_Type tim;
	
	tim.PrescaleOption=TIM_PRESCALE_TICKVAL;
	tim.PrescaleValue=1;
	
	TIM_Init(LPC_TIM3,TIM_TIMER_MODE,&tim);
	
	TIM_MATCHCFG_Type tm;
	
	tm.MatchChannel=0;
	tm.IntOnMatch=DISABLE;
	tm.ResetOnMatch=ENABLE;
	tm.StopOnMatch=DISABLE;
	tm.ExtMatchOutputType=0;
	tm.MatchValue=DACSPI_TIMER_MATCH;

	TIM_ConfigMatch(LPC_TIM3,&tm);
	
	NVIC_SetPriority(DMA_IRQn,1);
	NVIC_EnableIRQ(DMA_IRQn);

	// start
	
	TIM_Cmd(LPC_TIM3,ENABLE);
	
	LPC_GPDMACH0->CSrcAddr=lli[0][0].SrcAddr;
	LPC_GPDMACH0->CDestAddr=lli[0][0].DstAddr;
	LPC_GPDMACH0->CLLI=lli[0][0].NextLLI;
	LPC_GPDMACH0->CControl=lli[0][0].Control;

	LPC_GPDMACH0->CConfig=DACSPI_DMACONFIG;
	
	rprintf(0,"sampling at %d Hz\n",SYNTH_MASTER_CLOCK/DACSPI_TICK_RATE);
}