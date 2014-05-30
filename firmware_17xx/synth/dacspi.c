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

#define DACSPI_DMACONFIG \
			GPDMA_DMACCxConfig_E | \
			GPDMA_DMACCxConfig_DestPeripheral(14) | \
			GPDMA_DMACCxConfig_TransferType(1) | \
			GPDMA_DMACCxConfig_ITC

#define DACSPI_BUFFER_COUNT 64

#define DACSPI_CV_CHANNEL 6
#define DACSPI_CV_LDAC_MASK 0xbf

GPDMA_LLI_Type additionalCvLli[DACSPI_BUFFER_COUNT];
GPDMA_LLI_Type markerLli[DACSPI_BUFFER_COUNT*DACSPI_CHANNEL_COUNT] __attribute__((section(".usb_ram")));

static volatile uint8_t marker;
uint8_t markerSource[DACSPI_BUFFER_COUNT];

uint32_t deselectCommands[32][2] =
{
	{(uint32_t)&LPC_GPIO4->FIOSET,1<<CVMUX_PIN_VCA},

	{(uint32_t)&LPC_GPIO0->FIOSET,1<<CVMUX_PIN_CARD0},
	{(uint32_t)&LPC_GPIO0->FIOSET,1<<CVMUX_PIN_CARD0},
	{(uint32_t)&LPC_GPIO0->FIOSET,1<<CVMUX_PIN_CARD0},
	{(uint32_t)&LPC_GPIO0->FIOSET,1<<CVMUX_PIN_CARD0},
	{(uint32_t)&LPC_GPIO0->FIOSET,1<<CVMUX_PIN_CARD0},
	{(uint32_t)&LPC_GPIO0->FIOSET,1<<CVMUX_PIN_CARD0},
	{(uint32_t)&LPC_GPIO0->FIOSET,1<<CVMUX_PIN_CARD0},
	{(uint32_t)&LPC_GPIO0->FIOSET,1<<CVMUX_PIN_CARD0},

	{(uint32_t)&LPC_GPIO0->FIOSET,1<<CVMUX_PIN_CARD1},
	{(uint32_t)&LPC_GPIO0->FIOSET,1<<CVMUX_PIN_CARD1},
	{(uint32_t)&LPC_GPIO0->FIOSET,1<<CVMUX_PIN_CARD1},
	{(uint32_t)&LPC_GPIO0->FIOSET,1<<CVMUX_PIN_CARD1},
	{(uint32_t)&LPC_GPIO0->FIOSET,1<<CVMUX_PIN_CARD1},
	{(uint32_t)&LPC_GPIO0->FIOSET,1<<CVMUX_PIN_CARD1},
	{(uint32_t)&LPC_GPIO0->FIOSET,1<<CVMUX_PIN_CARD1},
	{(uint32_t)&LPC_GPIO0->FIOSET,1<<CVMUX_PIN_CARD1},

	{(uint32_t)&LPC_GPIO4->FIOSET,1<<CVMUX_PIN_CARD2},
	{(uint32_t)&LPC_GPIO4->FIOSET,1<<CVMUX_PIN_CARD2},
	{(uint32_t)&LPC_GPIO4->FIOSET,1<<CVMUX_PIN_CARD2},
	{(uint32_t)&LPC_GPIO4->FIOSET,1<<CVMUX_PIN_CARD2},
	{(uint32_t)&LPC_GPIO4->FIOSET,1<<CVMUX_PIN_CARD2},
	{(uint32_t)&LPC_GPIO4->FIOSET,1<<CVMUX_PIN_CARD2},
	{(uint32_t)&LPC_GPIO4->FIOSET,1<<CVMUX_PIN_CARD2},
	{(uint32_t)&LPC_GPIO4->FIOSET,1<<CVMUX_PIN_CARD2},

	{(uint32_t)&LPC_GPIO4->FIOSET,1<<CVMUX_PIN_VCA},
	{(uint32_t)&LPC_GPIO4->FIOSET,1<<CVMUX_PIN_VCA},
	{(uint32_t)&LPC_GPIO4->FIOSET,1<<CVMUX_PIN_VCA},
	{(uint32_t)&LPC_GPIO4->FIOSET,1<<CVMUX_PIN_VCA},
	{(uint32_t)&LPC_GPIO4->FIOSET,1<<CVMUX_PIN_VCA},
	{(uint32_t)&LPC_GPIO4->FIOSET,1<<CVMUX_PIN_VCA},
	{(uint32_t)&LPC_GPIO4->FIOSET,1<<CVMUX_PIN_VCA},
};

uint32_t selectCommands[4][2] =
{
	{(uint32_t)&LPC_GPIO0->FIOCLR,1<<CVMUX_PIN_CARD0},
	{(uint32_t)&LPC_GPIO0->FIOCLR,1<<CVMUX_PIN_CARD1},
	{(uint32_t)&LPC_GPIO4->FIOCLR,1<<CVMUX_PIN_CARD2},
	{(uint32_t)&LPC_GPIO4->FIOCLR,1<<CVMUX_PIN_VCA},
};

uint32_t abcResetCommands[2] =
{
	(uint32_t)&LPC_GPIO0->FIOCLR,7<<CVMUX_PIN_A
};

uint32_t abcCommands[8][2] =
{
	{(uint32_t)&LPC_GPIO0->FIOSET,0<<CVMUX_PIN_A},
	{(uint32_t)&LPC_GPIO0->FIOSET,1<<CVMUX_PIN_A},
	{(uint32_t)&LPC_GPIO0->FIOSET,2<<CVMUX_PIN_A},
	{(uint32_t)&LPC_GPIO0->FIOSET,3<<CVMUX_PIN_A},
	{(uint32_t)&LPC_GPIO0->FIOSET,4<<CVMUX_PIN_A},
	{(uint32_t)&LPC_GPIO0->FIOSET,5<<CVMUX_PIN_A},
	{(uint32_t)&LPC_GPIO0->FIOSET,6<<CVMUX_PIN_A},
	{(uint32_t)&LPC_GPIO0->FIOSET,7<<CVMUX_PIN_A},
};

static struct
{
	GPDMA_LLI_Type lli[DACSPI_BUFFER_COUNT*DACSPI_CHANNEL_COUNT][2];
	uint16_t voiceCommands[DACSPI_BUFFER_COUNT][SYNTH_VOICE_COUNT][2];
	uint16_t cvCommands[32];
	uint8_t channelSteps[DACSPI_CHANNEL_COUNT][DACSPI_STEP_COUNT];
} dacspi __attribute__((section(".eth_ram"))) ;


__attribute__ ((used)) void DMA_IRQHandler(void)
{
	int8_t secondHalfPlaying=marker>=DACSPI_BUFFER_COUNT/2;
	
	LPC_GPDMA->DMACIntTCClear=LPC_GPDMA->DMACIntTCStat;

	// when second half is playing, update first and vice-versa
	synth_updateDACsEvent(secondHalfPlaying?0:DACSPI_BUFFER_COUNT/2,DACSPI_BUFFER_COUNT/2);
}

FORCEINLINE void dacspi_setVoiceCommand(int32_t buffer, int voice, int ab, uint16_t command)
{
	dacspi.voiceCommands[buffer][voice][ab]=command;
}

FORCEINLINE void dacspi_setCVCommand(uint16_t command, int channel)
{
	dacspi.cvCommands[channel]=command;
}

void dacspi_init(void)
{
	int i,j,lliPos,cv;
	
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

	//

	lliPos=0;
	
	for(j=0;j<DACSPI_BUFFER_COUNT;++j)
	{
		markerSource[j]=j;
		cv=j>>1;

		for(i=0;i<DACSPI_CHANNEL_COUNT;++i)
		{
			memset(&dacspi.channelSteps[i][0],0xff,DACSPI_STEP_COUNT);

			if(i==DACSPI_CV_CHANNEL)
			{
				dacspi.channelSteps[i][DACSPI_STEP_COUNT-2]=DACSPI_CV_LDAC_MASK;

				if(j&1)
				{
					dacspi.lli[lliPos][0].SrcAddr=(uint32_t)&abcResetCommands[1];
					dacspi.lli[lliPos][0].DstAddr=abcResetCommands[0];
					dacspi.lli[lliPos][0].NextLLI=(uint32_t)&additionalCvLli[j];
					dacspi.lli[lliPos][0].Control=
						GPDMA_DMACCxControl_TransferSize(1) |
						GPDMA_DMACCxControl_SWidth(2) |
						GPDMA_DMACCxControl_DWidth(2);

					additionalCvLli[j].SrcAddr=(uint32_t)&abcCommands[cv&7][1];
					additionalCvLli[j].DstAddr=abcCommands[cv&7][0];
					additionalCvLli[j].NextLLI=(uint32_t)&dacspi.lli[lliPos][1];
					additionalCvLli[j].Control=
						GPDMA_DMACCxControl_TransferSize(1) |
						GPDMA_DMACCxControl_SWidth(2) |
						GPDMA_DMACCxControl_DWidth(2);
					
					dacspi.lli[lliPos][1].SrcAddr=(uint32_t)&selectCommands[cv>>3][1];
					dacspi.lli[lliPos][1].DstAddr=selectCommands[cv>>3][0];
					dacspi.lli[lliPos][1].NextLLI=(uint32_t)&dacspi.lli[(lliPos+1)%(DACSPI_BUFFER_COUNT*DACSPI_CHANNEL_COUNT)][0];
					dacspi.lli[lliPos][1].Control=
						GPDMA_DMACCxControl_TransferSize(1) |
						GPDMA_DMACCxControl_SWidth(2) |
						GPDMA_DMACCxControl_DWidth(2);
				}
				else
				{
					dacspi.lli[lliPos][0].SrcAddr=(uint32_t)&dacspi.cvCommands[cv];
					dacspi.lli[lliPos][0].DstAddr=(uint32_t)&LPC_SSP0->DR;
					dacspi.lli[lliPos][0].NextLLI=(uint32_t)&additionalCvLli[j];
					dacspi.lli[lliPos][0].Control=
						GPDMA_DMACCxControl_TransferSize(1) |
						GPDMA_DMACCxControl_SWidth(1) |
						GPDMA_DMACCxControl_DWidth(1);

					additionalCvLli[j].SrcAddr=(uint32_t)&deselectCommands[cv][1];
					additionalCvLli[j].DstAddr=deselectCommands[cv][0];
					additionalCvLli[j].NextLLI=(uint32_t)&dacspi.lli[lliPos][1];
					additionalCvLli[j].Control=
						GPDMA_DMACCxControl_TransferSize(1) |
						GPDMA_DMACCxControl_SWidth(2) |
						GPDMA_DMACCxControl_DWidth(2);
				}
	 		}
			else
			{
				dacspi.channelSteps[i][DACSPI_STEP_COUNT-2]=voice_ldac_mask[synth_voiceLayout[0][i]];

				dacspi.lli[lliPos][0].SrcAddr=(uint32_t)&dacspi.voiceCommands[j][i][0];
				dacspi.lli[lliPos][0].DstAddr=(uint32_t)&LPC_SSP0->DR;
				dacspi.lli[lliPos][0].NextLLI=(uint32_t)&markerLli[lliPos];
				dacspi.lli[lliPos][0].Control=
					GPDMA_DMACCxControl_TransferSize(2) |
					GPDMA_DMACCxControl_SWidth(1) |
					GPDMA_DMACCxControl_DWidth(1) |
					GPDMA_DMACCxControl_SI;
				
				markerLli[lliPos].SrcAddr=(uint32_t)&markerSource[j];
				markerLli[lliPos].DstAddr=(uint32_t)&marker;
				markerLli[lliPos].NextLLI=(uint32_t)&dacspi.lli[lliPos][1];
				markerLli[lliPos].Control=
					GPDMA_DMACCxControl_TransferSize(1) |
					GPDMA_DMACCxControl_SWidth(0) |
					GPDMA_DMACCxControl_DWidth(0);
			}
			
			if(!(i==DACSPI_CV_CHANNEL && (j&1)))
			{
				dacspi.lli[lliPos][1].NextLLI=(uint32_t)&dacspi.lli[(lliPos+1)%(DACSPI_BUFFER_COUNT*DACSPI_CHANNEL_COUNT)][0];
				dacspi.lli[lliPos][1].SrcAddr=(uint32_t)&dacspi.channelSteps[i][0];
				dacspi.lli[lliPos][1].DstAddr=(uint32_t)&LPC_GPIO2->FIOPIN0;
				dacspi.lli[lliPos][1].Control=
					GPDMA_DMACCxControl_TransferSize(DACSPI_STEP_COUNT) |
					GPDMA_DMACCxControl_SWidth(0) |
					GPDMA_DMACCxControl_DWidth(0) |
					GPDMA_DMACCxControl_SI;
			}
			
			++lliPos;
		}
	}
	
	// interrupt triggers
	
	dacspi.lli[DACSPI_BUFFER_COUNT*DACSPI_CHANNEL_COUNT*1/16][0].Control|=GPDMA_DMACCxControl_I;
	dacspi.lli[DACSPI_BUFFER_COUNT*DACSPI_CHANNEL_COUNT*9/16][0].Control|=GPDMA_DMACCxControl_I;
	
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
	
	LPC_GPDMACH0->DMACCSrcAddr=dacspi.lli[0][0].SrcAddr;
	LPC_GPDMACH0->DMACCDestAddr=dacspi.lli[0][0].DstAddr;
	LPC_GPDMACH0->DMACCLLI=dacspi.lli[0][0].NextLLI;
	LPC_GPDMACH0->DMACCControl=dacspi.lli[0][0].Control;

	LPC_GPDMACH0->DMACCConfig=DACSPI_DMACONFIG;
	
	rprintf(0,"sampling at %d Hz\n",SYNTH_MASTER_CLOCK/DACSPI_TICK_RATE);
}