///////////////////////////////////////////////////////////////////////////////
// LCD / keypad / potentiometers user interface
///////////////////////////////////////////////////////////////////////////////

#include "ui.h"

#define ADCROW_PORT 3
#define ADCROW_PIN 25

#define UI_POT_COUNT 10
#define UI_POT_SAMPLES 50

static const uint8_t potToCh[UI_POT_COUNT]=
{
	0,1,2,3,5,
	0,1,2,3,5,
};

static struct
{
	uint16_t pots[UI_POT_COUNT][UI_POT_SAMPLES];
	int8_t row;
	int8_t curSample;
} ui;

__attribute__ ((used)) void ADC_IRQHandler(void)
{
	int8_t pot,i;
	
	pot=ui.row?(UI_POT_COUNT/2):0;

	for(i=0;i<UI_POT_COUNT/2;++i)
	{
		uint32_t adc_value;
 
		adc_value = *(uint32_t *) ((&LPC_ADC->ADDR0) + potToCh[pot]);
		
		if(adc_value&ADC_DR_DONE_FLAG)
			ui.pots[pot][ui.curSample]=adc_value;
		++pot;
	}

	if(ui.row)
	{
		GPIO_ClearValue(ADCROW_PORT,1<<ADCROW_PIN);
	}
	else
	{
		GPIO_SetValue(ADCROW_PORT,1<<ADCROW_PIN);
		
		++ui.curSample;
		if(ui.curSample>=UI_POT_SAMPLES)
			ui.curSample=0;
	}
	
	ui.row=!ui.row;
	
}

void bubble(uint16_t a[], int n)
{
	int i, j;
	uint16_t t;
	for (i = n - 2; i >= 0; i--)
	{
		for (j = 0; j <= i; j++)
		{
			if (a[j] > a[j + 1])
			{
				t = a[j];
				a[j] = a[j + 1];
				a[j + 1] = t;
			}
		}
	}
}

uint16_t ui_getPotValue(int8_t pot)
{
	uint16_t tmp[UI_POT_SAMPLES];
	
	memcpy(&tmp[0],&ui.pots[pot][0],UI_POT_SAMPLES*sizeof(uint16_t));
	bubble(tmp,UI_POT_SAMPLES);
	
	return (tmp[UI_POT_SAMPLES/2]>>6)<<6; // get median
}

void ui_init(void)
{
	memset(&ui,0,sizeof(ui));
	
	// init screen serial
	
	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_UART1,CLKPWR_PCLKSEL_CCLK_DIV_1);	
	init_serial1(115200);
	
	PINSEL_SetPinFunc(0,15,1);
	PINSEL_SetPinFunc(0,16,1);
	
	rprintf_devopen(1,putc_serial1);
	
	// init screen
	
	rprintf(1,"\r\x1b[c"); // reset
	delay_ms(500); // init time
	rprintf(1,"\x1b[20c"); // screen size (20 cols)
	rprintf(1,"\x1b[4L"); // screen size (4 rows)
	rprintf(1,"\x1b[?25I"); // hide cursor
	rprintf(1,"\x1b[1x"); // disable scrolling
	rprintf(1,"\x1b[2J"); // clear screen

	// welcome message
	
	rprintf(1,"    GliGli's DIY    ");
	rprintf(1,"                    ");
	rprintf(1,"-={[ OverCycler ]}=-");
	rprintf(1,"                    ");
	
	// init row select
	
	PINSEL_SetPinFunc(ADCROW_PORT,ADCROW_PIN,0);
	GPIO_SetDir(ADCROW_PORT,1<<ADCROW_PIN,1);
	
	// init adc pins

	PINSEL_SetPinFunc(0,23,1);
	PINSEL_SetPinFunc(0,24,1);
	PINSEL_SetPinFunc(0,25,1);
	PINSEL_SetPinFunc(0,26,1);
	PINSEL_SetPinFunc(1,31,3);
	
	// init adc
	
	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_ADC,CLKPWR_PCLKSEL_CCLK_DIV_8);
	CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCAD,ENABLE);

	LPC_ADC->ADCR=0x2f | ADC_CR_CLKDIV(24);
	LPC_ADC->ADINTEN=0x01;
	
	NVIC_EnableIRQ(ADC_IRQn);

	// start
	
	LPC_ADC->ADCR|= ADC_CR_PDN | ADC_CR_BURST;
}

void ui_update(void)
{
	rprintf(1,"                    ");
	for(int i=0;i<UI_POT_COUNT;++i)
		rprintf(1,"% 4d",((ui_getPotValue(i))>>6)%1000);
	rprintf(1,"                    ");
//	delay_ms(40);
}

