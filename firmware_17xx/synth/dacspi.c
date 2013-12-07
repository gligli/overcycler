///////////////////////////////////////////////////////////////////////////////
// 12bit voice DACs communication through SPI
///////////////////////////////////////////////////////////////////////////////

#include "dacspi.h"

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

#define DACSPI_MARKER_COUNT (DACSPI_STEP_COUNT+2)
#define DACSPI_MARKER_PRE 0xC0
#define DACSPI_MARKER_SSP 0x40
#define DACSPI_MARKER_LDAC 0x80

static volatile uint8_t marker;

__attribute__ ((section(".eth_ram"))) static struct
{
	GPDMA_LLI_Type lli[DACSPI_BUFFER_COUNT*DACSPI_CHANNEL_COUNT][2];
	uint16_t voiceCommands[DACSPI_BUFFER_COUNT][SYNTH_VOICE_COUNT][2];
	uint8_t voiceSteps[SYNTH_VOICE_COUNT][DACSPI_STEP_COUNT];
	uint8_t markers[DACSPI_BUFFER_COUNT][DACSPI_MARKER_COUNT];
} dacspi;


__attribute__ ((used)) void DMA_IRQHandler(void)
{
	int8_t secondHalfPlaying,curBuffer=marker&0x3f;
	
	LPC_GPDMA->DMACIntTCClear=LPC_GPDMA->DMACIntTCStat;

	// when second half is playing, update first and vice-versa
	secondHalfPlaying=curBuffer>=(DACSPI_BUFFER_COUNT/2);
	synth_updateDACsEvent(secondHalfPlaying?0:DACSPI_BUFFER_COUNT/2,DACSPI_BUFFER_COUNT/2);
}

FORCEINLINE void dacspi_setVoiceCommand(int32_t buffer, int voice, int ab, uint16_t command)
{
	dacspi.voiceCommands[buffer][voice][ab]=command;
}

FORCEINLINE void dacspi_sendCVCommand(uint16_t command)
{
	// CV DAC update
	
	BLOCK_INT
	{
		while((marker&0xc0)!=DACSPI_MARKER_SSP); // wait for CV DAC slot
		
		LPC_GPIO2->FIOPIN0=0xff; // initial LDAC pulse state
		LPC_SSP0->DR=command; // send SPI DAC command

		DELAY_100NS();
		DELAY_100NS();
		DELAY_100NS();
		DELAY_100NS();
		DELAY_100NS();
		DELAY_100NS();
		DELAY_100NS();
		DELAY_100NS();
		
		LPC_GPIO2->FIOPIN0=DACSPI_CV_LDAC_MASK; // LDAC pulse

		DELAY_100NS();

		LPC_GPIO2->FIOPIN0=0xff; //  LDAC pulse end
	}

	// wait DAC setting time (4.5us)
	
	delay_us(6);
}

void dacspi_init(void)
{
	int i,j,lliPos;
	
	memset(&dacspi,0,sizeof(dacspi));

	// SSP

	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_SSP0,CLKPWR_PCLKSEL_CCLK_DIV_1);
	CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCSSP0,ENABLE);

	LPC_SSP0->CPSR=4; // 30Mhz
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
		for(i=0;i<DACSPI_CHANNEL_COUNT;++i)
		{
			if(i==DACSPI_CV_CHANNEL)
			{
				memset(&dacspi.markers[j][0],DACSPI_MARKER_SSP|j,DACSPI_MARKER_COUNT);
				/*dacspi.markers[j][0]=DACSPI_MARKER_PRE|j;
				dacspi.markers[j][1]=DACSPI_MARKER_PRE|j;*/
				dacspi.markers[j][DACSPI_MARKER_COUNT-1]=DACSPI_MARKER_LDAC|j;

				dacspi.lli[lliPos][0].SrcAddr=(uint32_t)&dacspi.markers[j][0];
				dacspi.lli[lliPos][0].DstAddr=(uint32_t)&marker;
				dacspi.lli[lliPos][0].NextLLI=(uint32_t)&dacspi.lli[(lliPos+1)%(DACSPI_BUFFER_COUNT*DACSPI_CHANNEL_COUNT)][0];
				dacspi.lli[lliPos][0].Control=
					GPDMA_DMACCxControl_TransferSize(DACSPI_STEP_COUNT+2) |
					GPDMA_DMACCxControl_SWidth(0) |
					GPDMA_DMACCxControl_DWidth(0) |
					GPDMA_DMACCxControl_SI;
			}
			else
			{
				memset(&dacspi.voiceSteps[i][0],0xff,DACSPI_STEP_COUNT);
				dacspi.voiceSteps[i][DACSPI_STEP_COUNT-1]=voice_ldac_mask[synth_voiceLayout[0][i]];

				dacspi.lli[lliPos][0].SrcAddr=(uint32_t)&dacspi.voiceCommands[j][i][0];
				dacspi.lli[lliPos][0].DstAddr=(uint32_t)&LPC_SSP0->DR;
				dacspi.lli[lliPos][0].NextLLI=(uint32_t)&dacspi.lli[lliPos][1];
				dacspi.lli[lliPos][0].Control=
					GPDMA_DMACCxControl_TransferSize(2) |
					GPDMA_DMACCxControl_SWidth(1) |
					GPDMA_DMACCxControl_DWidth(1) |
					GPDMA_DMACCxControl_SI;
			}

			dacspi.lli[lliPos][1].NextLLI=(uint32_t)&dacspi.lli[(lliPos+1)%(DACSPI_BUFFER_COUNT*DACSPI_CHANNEL_COUNT)][0];
			dacspi.lli[lliPos][1].SrcAddr=(uint32_t)&dacspi.voiceSteps[i][0];
			dacspi.lli[lliPos][1].DstAddr=(uint32_t)&LPC_GPIO2->FIOPIN0;
			dacspi.lli[lliPos][1].Control=
				GPDMA_DMACCxControl_TransferSize(DACSPI_STEP_COUNT) |
				GPDMA_DMACCxControl_SWidth(0) |
				GPDMA_DMACCxControl_DWidth(0) |
				GPDMA_DMACCxControl_SI;
			
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