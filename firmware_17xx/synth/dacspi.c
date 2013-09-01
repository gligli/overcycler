///////////////////////////////////////////////////////////////////////////////
// 12bit voice DACs communication through SPI
///////////////////////////////////////////////////////////////////////////////

#include "dacspi.h"

#define DACSPI_DMACONFIG \
			GPDMA_DMACCxConfig_E | \
			GPDMA_DMACCxConfig_DestPeripheral(14) | \
			GPDMA_DMACCxConfig_TransferType(1) | \
			GPDMA_DMACCxConfig_ITC

static const uint8_t channel2mask[DACSPI_CHANNEL_COUNT]=
{
	0x02,0x01,0x04,0x08,0x010,0x80,0x40
};

__attribute__ ((section(".eth_ram"))) static struct
{
	uint32_t commands[DACSPI_CHANNEL_COUNT][2];
	uint8_t steps[DACSPI_CHANNEL_COUNT][DACSPI_STEP_COUNT];
	GPDMA_LLI_Type lli[DACSPI_CHANNEL_COUNT][2];
} dacspi;


__attribute__ ((used)) void DMA_IRQHandler(void)
{
	LPC_GPDMA->DMACIntTCClear=LPC_GPDMA->DMACIntTCStat;
	
	synth_updateDACs();
}

FORCEINLINE void dacspi_setCommand(uint8_t channel, int8_t ab, uint16_t command)
{
	dacspi.commands[channel][ab]=command;
}

void dacspi_init(void)
{
	int i;
	
	memset(&dacspi,0,sizeof(dacspi));

	// SSP

	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_SSP0,CLKPWR_PCLKSEL_CCLK_DIV_1);
	CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCSSP0,ENABLE);

	LPC_SSP0->CPSR=6; // 20Mhz
	LPC_SSP0->CR0=0x0f; // 16Bit SPI(0,0)
	LPC_SSP0->CR1=2; // Enable
	LPC_SSP0->DMACR=3; // DMA

	// SSP pins

	PINSEL_SetPinFunc(1,20,3);
	PINSEL_SetPinFunc(1,21,3);
	PINSEL_SetPinFunc(1,24,3);
	
	// /LDAC pins
	
	for(i=0;i<8;++i)
	{
		PINSEL_SetPinFunc(2,i,0);
		GPIO_SetDir(2,1<<i,1);
		GPIO_SetValue(2,1<<i);
	}

	//
	
	for(i=0;i<DACSPI_CHANNEL_COUNT;++i)
	{
		memset(&dacspi.steps[i][0],0xff,DACSPI_STEP_COUNT);
		dacspi.steps[i][DACSPI_STEP_COUNT-2]=~channel2mask[i];

		dacspi.lli[i][0].SrcAddr=(uint32_t)&dacspi.commands[i][0];
		dacspi.lli[i][0].DstAddr=(uint32_t)&LPC_SSP0->DR;
		dacspi.lli[i][0].NextLLI=(uint32_t)&dacspi.lli[i][1];
		dacspi.lli[i][0].Control=
			GPDMA_DMACCxControl_TransferSize(2) |
			GPDMA_DMACCxControl_SWidth(2) |
			GPDMA_DMACCxControl_DWidth(2) |
			GPDMA_DMACCxControl_SI;

		dacspi.lli[i][1].SrcAddr=(uint32_t)&dacspi.steps[i][0];
		dacspi.lli[i][1].DstAddr=(uint32_t)&LPC_GPIO2->FIOPIN0;
		dacspi.lli[i][1].NextLLI=(uint32_t)&dacspi.lli[(i+1)%DACSPI_CHANNEL_COUNT][0];
		dacspi.lli[i][1].Control=
			GPDMA_DMACCxControl_TransferSize(DACSPI_STEP_COUNT) |
			GPDMA_DMACCxControl_SWidth(0) |
			GPDMA_DMACCxControl_DWidth(0) |
			GPDMA_DMACCxControl_SI;
	}
	
	dacspi.lli[0][1].Control|=GPDMA_DMACCxControl_I;
	
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
}