///////////////////////////////////////////////////////////////////////////////
// 12bit voice DACs communication through SPI
///////////////////////////////////////////////////////////////////////////////

#include "dacspi.h"
#include "LPC17xx.h"
#include "lpc17xx_gpdma.h"

static const uint8_t voice_ldac_mask[SYNTH_VOICE_COUNT]=
{
	0xfd,0xfe,0xfb,0xf7,0xef,0x7f,
};

#define DACSPI_CMD_SET_A 0x7000
#define DACSPI_CMD_SET_B 0xf000
#define DACSPI_CMD_SET_REF 0x7000

#define DACSPI_DMACONFIG \
			GPDMA_DMACCxConfig_E | \
			GPDMA_DMACCxConfig_DestPeripheral(14) | \
			GPDMA_DMACCxConfig_TransferType(1) | \
			GPDMA_DMACCxConfig_ITC

#define DACSPI_BUFFER_COUNT 48

#define DACSPI_CV_CHANNEL_1 6
#define DACSPI_CV_CHANNEL_2 7
#define DACSPI_CARD_CV_LDAC_MASK 0xbf
#define DACSPI_VCA_CV_LDAC_MASK 0xef

static EXT_RAM GPDMA_LLI_Type lli[DACSPI_BUFFER_COUNT*DACSPI_CHANNEL_COUNT][2];
static EXT_RAM GPDMA_LLI_Type cardLli[DACSPI_BUFFER_COUNT];
static EXT_RAM GPDMA_LLI_Type vcaLli[DACSPI_BUFFER_COUNT];
static EXT_RAM GPDMA_LLI_Type markerLli[DACSPI_BUFFER_COUNT];

static EXT_RAM volatile uint8_t marker;
static EXT_RAM volatile uint8_t dummy;
static EXT_RAM uint8_t markerSource[DACSPI_BUFFER_COUNT];

static const uint32_t deselectCommands[3][2] =
{
	{(uint32_t)&LPC_GPIO0->FIOSET,1<<CVMUX_PIN_CARD0},
	{(uint32_t)&LPC_GPIO0->FIOSET,1<<CVMUX_PIN_CARD1},
	{(uint32_t)&LPC_GPIO4->FIOSET,1<<CVMUX_PIN_CARD2},
};

static const uint32_t selectCommands[3][2] =
{
	{(uint32_t)&LPC_GPIO0->FIOCLR,1<<CVMUX_PIN_CARD0},
	{(uint32_t)&LPC_GPIO0->FIOCLR,1<<CVMUX_PIN_CARD1},
	{(uint32_t)&LPC_GPIO4->FIOCLR,1<<CVMUX_PIN_CARD2},
};

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
	uint16_t cvCommands[32];
	uint8_t channelSteps[DACSPI_BUFFER_COUNT][DACSPI_CHANNEL_COUNT][DACSPI_STEP_COUNT];
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
	int cardCV,vcaDAC,stdSteps;	
	int lliPos=buffer*DACSPI_CHANNEL_COUNT+channel;
	
	cardCV=buffer>>1;
	vcaDAC=buffer&3;
	stdSteps=1;
	
	memset(&dacspi.channelSteps[buffer][channel][0],0xff,DACSPI_STEP_COUNT);

	if(channel==DACSPI_CV_CHANNEL_1 || channel==DACSPI_CV_CHANNEL_2)
	{
		// insert VCA accessed between both parts of card access to avoid conflict on the address (ABC) bus
		int isCard=channel==DACSPI_CV_CHANNEL_1 && (buffer&1)==0 || channel==DACSPI_CV_CHANNEL_2 && (buffer&1)==1;
		
		if(isCard)
		{
			dacspi.channelSteps[buffer][channel][DACSPI_STEP_COUNT-2]=DACSPI_CARD_CV_LDAC_MASK;

			if(buffer&1)
			{
				stdSteps=0;

				lli[lliPos][0].SrcAddr=(uint32_t)&abcCommands[cardCV&7];
				lli[lliPos][0].DstAddr=(uint32_t)&LPC_GPIO0->FIOPIN2;
				lli[lliPos][0].NextLLI=(uint32_t)&markerLli[buffer];
				lli[lliPos][0].Control=
					GPDMA_DMACCxControl_TransferSize(1) |
					GPDMA_DMACCxControl_SWidth(0) |
					GPDMA_DMACCxControl_DWidth(0);

				markerLli[buffer].SrcAddr=(uint32_t)&markerSource[buffer];
				markerLli[buffer].DstAddr=(uint32_t)&marker;
				markerLli[buffer].NextLLI=(uint32_t)&lli[lliPos][1];
				markerLli[buffer].Control=
					GPDMA_DMACCxControl_TransferSize(1) |
					GPDMA_DMACCxControl_SWidth(0) |
					GPDMA_DMACCxControl_DWidth(0);

				lli[lliPos][1].SrcAddr=(uint32_t)&selectCommands[cardCV>>3][1];
				lli[lliPos][1].DstAddr=selectCommands[cardCV>>3][0];
				lli[lliPos][1].NextLLI=(uint32_t)&lli[(lliPos+1)%(DACSPI_BUFFER_COUNT*DACSPI_CHANNEL_COUNT)][0];
				lli[lliPos][1].Control=
					GPDMA_DMACCxControl_TransferSize(1) |
					GPDMA_DMACCxControl_SWidth(2) |
					GPDMA_DMACCxControl_DWidth(2);
			}
			else
			{
				lli[lliPos][0].SrcAddr=(uint32_t)&dacspi.cvCommands[cardCV];
				lli[lliPos][0].DstAddr=(uint32_t)&LPC_SSP0->DR;
				lli[lliPos][0].NextLLI=(uint32_t)&cardLli[buffer];
				lli[lliPos][0].Control=
					GPDMA_DMACCxControl_TransferSize(1) |
					GPDMA_DMACCxControl_SWidth(1) |
					GPDMA_DMACCxControl_DWidth(1);

				int idx=MIN(2,((uint32_t)cardCV-1)/8);

				cardLli[buffer].SrcAddr=(uint32_t)&deselectCommands[idx][1];
				cardLli[buffer].DstAddr=deselectCommands[idx][0];
				cardLli[buffer].NextLLI=(uint32_t)&lli[lliPos][1];
				cardLli[buffer].Control=
					GPDMA_DMACCxControl_TransferSize(1) |
					GPDMA_DMACCxControl_SWidth(2) |
					GPDMA_DMACCxControl_DWidth(2);
			}
		}
		else
		{
			dacspi.channelSteps[buffer][channel][DACSPI_STEP_COUNT-2]=DACSPI_VCA_CV_LDAC_MASK;
			stdSteps=0;

			lli[lliPos][0].SrcAddr=(uint32_t)&dacspi.cvCommands[24+vcaDAC*2];
			lli[lliPos][0].DstAddr=(uint32_t)&LPC_SSP0->DR;
			lli[lliPos][0].NextLLI=(uint32_t)&vcaLli[buffer];
			lli[lliPos][0].Control=
				GPDMA_DMACCxControl_TransferSize(2) |
				GPDMA_DMACCxControl_SWidth(1) |
				GPDMA_DMACCxControl_DWidth(1) |
				GPDMA_DMACCxControl_SI;

			vcaLli[buffer].SrcAddr=(uint32_t)&abcCommands[vcaDAC];
			vcaLli[buffer].DstAddr=(uint32_t)&LPC_GPIO0->FIOPIN2;
			vcaLli[buffer].NextLLI=(uint32_t)&lli[lliPos][1];
			vcaLli[buffer].Control=
				GPDMA_DMACCxControl_TransferSize(1) |
				GPDMA_DMACCxControl_SWidth(0) |
				GPDMA_DMACCxControl_DWidth(0);

			lli[lliPos][1].NextLLI=(uint32_t)&lli[(lliPos+1)%(DACSPI_BUFFER_COUNT*DACSPI_CHANNEL_COUNT)][0];
			lli[lliPos][1].SrcAddr=(uint32_t)&dacspi.channelSteps[buffer][channel][0];
			lli[lliPos][1].DstAddr=(uint32_t)&LPC_GPIO4->FIOPIN3;
			lli[lliPos][1].Control=
				GPDMA_DMACCxControl_TransferSize(DACSPI_STEP_COUNT) |
				GPDMA_DMACCxControl_SWidth(0) |
				GPDMA_DMACCxControl_DWidth(0) |
				GPDMA_DMACCxControl_SI;
		}
	}
	else
	{
		dacspi.channelSteps[buffer][channel][DACSPI_STEP_COUNT-2]=voice_ldac_mask[channel];

		lli[lliPos][0].SrcAddr=(uint32_t)&dacspi.voiceCommands[buffer][channel][0];
		lli[lliPos][0].DstAddr=(uint32_t)&LPC_SSP0->DR;
		lli[lliPos][0].NextLLI=(uint32_t)&lli[lliPos][1];
		lli[lliPos][0].Control=
			GPDMA_DMACCxControl_TransferSize(2) |
			GPDMA_DMACCxControl_SWidth(1) |
			GPDMA_DMACCxControl_DWidth(1) |
			GPDMA_DMACCxControl_SI;
	}

	if(stdSteps)
	{
		lli[lliPos][1].NextLLI=(uint32_t)&lli[(lliPos+1)%(DACSPI_BUFFER_COUNT*DACSPI_CHANNEL_COUNT)][0];
		lli[lliPos][1].SrcAddr=(uint32_t)&dacspi.channelSteps[buffer][channel][0];
		lli[lliPos][1].DstAddr=(uint32_t)&LPC_GPIO2->FIOPIN0;
		lli[lliPos][1].Control=
			GPDMA_DMACCxControl_TransferSize(DACSPI_STEP_COUNT) |
			GPDMA_DMACCxControl_SWidth(0) |
			GPDMA_DMACCxControl_DWidth(0) |
			GPDMA_DMACCxControl_SI;
	}
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
	
	if(channel<24)
		dacspi.cvCommands[channel]=value|DACSPI_CMD_SET_REF;
	else if(channel&1)
		dacspi.cvCommands[channel]=value|DACSPI_CMD_SET_B;
	else
		dacspi.cvCommands[channel]=value|DACSPI_CMD_SET_A;
}

void dacspi_init(void)
{
	int i,j;
	
	memset(&dacspi,0,sizeof(dacspi));

	// SSP

	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_SSP0,CLKPWR_PCLKSEL_CCLK_DIV_1);
	CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCSSP0,ENABLE);

	LPC_SSP0->CPSR=4; // SYNTH_MASTER_CLOCK / 4
	LPC_SSP0->CR0=0x0f; // 16Bit SPI(0,0)
	LPC_SSP0->CR1=2; // Enable
	LPC_SSP0->DMACR=3; // DMA

	// SSP pins

	PINSEL_SetPinFunc(1,20,3);
	PINSEL_SetPinFunc(1,21,3);
	PINSEL_SetPinFunc(1,24,3);
	
	LPC_GPIO1->FIODIR2|=0x20;
	LPC_GPIO1->FIOMASK2&=~0x20;
	
	// /LDAC pins
	
	for(i=0;i<8;++i)
	{
		PINSEL_SetPinFunc(2,i,0);
		GPIO_SetDir(2,1<<i,1);
	}

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