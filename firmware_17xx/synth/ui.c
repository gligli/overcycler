///////////////////////////////////////////////////////////////////////////////
// LCD / keypad / potentiometers user interface
///////////////////////////////////////////////////////////////////////////////

#include "ui.h"
#include "storage.h"

#define DISPLAY_ACK 6

#define ADC_QUANTUM (1<<(16-12)) // 12bit resolution

#define ADCROW_PORT 3
#define ADCROW_PIN 25

#define UI_POT_COUNT 10
#define UI_POT_SAMPLES 128

#define POT_CHANGE_DETECT_THRESHOLD (ADC_QUANTUM*24)
#define POT_TIMEOUT_THRESHOLD (ADC_QUANTUM*12)
#define POT_TIMEOUT 20

static const uint8_t potToCh[UI_POT_COUNT]=
{
	0,1,2,3,5,
	0,1,2,3,5,
};

static struct
{
	uint16_t pots[UI_POT_COUNT][UI_POT_SAMPLES];
	int16_t curSample;
	int8_t row;
	uint16_t value[UI_POT_COUNT];
	uint16_t lockTimeout[UI_POT_COUNT];
	
	int8_t presetModified;
} ui;

__attribute__ ((used)) void ADC_IRQHandler(void)
{
	int pot,i;
	
	pot=ui.row?(UI_POT_COUNT/2):0;

	for(i=0;i<UI_POT_COUNT/2;++i)
	{
		uint32_t adc_value;
 
		adc_value = *(uint32_t *) ((&LPC_ADC->ADDR0) + potToCh[pot]);
		
		if(adc_value&ADC_DR_DONE_FLAG)
			ui.pots[pot][ui.curSample]=(uint16_t)adc_value;
		
		++pot;
	}

	if(ui.row)
	{
		GPIO_ClearValue(ADCROW_PORT,1<<ADCROW_PIN);
	}
	else
	{
		GPIO_SetValue(ADCROW_PORT,1<<ADCROW_PIN);
		
		// scanned all the pots, move to next sample
		
		++ui.curSample;
		if(ui.curSample>=UI_POT_SAMPLES)
			ui.curSample=0;
	}
	
	ui.row=!ui.row;
	
}

int sendChar(int ch)
{
	while(UART_CheckBusy((LPC_UART_TypeDef*)LPC_UART1)==SET);
	UART_SendData((LPC_UART_TypeDef*)LPC_UART1,ch);
	return -1;
}

uint8_t receiveChar(void)
{
	return UART_ReceiveData((LPC_UART_TypeDef*)LPC_UART1);
}

static void sendCommand(const char * command, int8_t waitAck, char * response)
{
	uint8_t c;

	// empty receive fifo
	while(receiveChar());
		
	// send command
	sendChar(0x1b);
	while(*command)
		sendChar(*command++);

	if(waitAck)
	{
		do
		{
			c=receiveChar();
			if(response && c && c!=DISPLAY_ACK)
				*response++=c;
		}
		while(c!=DISPLAY_ACK);
	}

	if(response)
		*response=0;
}

static void readKeypad(void)
{
	int key;
	char response[10],*rb;

	// get all awaiting keys
	
	for(;;)
	{
		sendCommand("[k",1,response);

		key=0;
		rb=response;
		while(*rb)
		{
			if(*rb>='0' && *rb<='9')
				key=key*10+(*rb-'0');
			++rb;
		}
		
		if(!key) break;
		
		rprintf(0,"keypad %x %s\n",key,response);
	}
}

static void bubble(uint16_t a[], int n)
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

static void updatePotValue(int8_t pot)
{
	uint16_t tmp[UI_POT_SAMPLES];
	int i,cnt;
	uint32_t acc,new,delta;
	
	// sort values
	
	memcpy(&tmp[0],&ui.pots[pot][0],UI_POT_SAMPLES*sizeof(uint16_t));
	bubble(tmp,UI_POT_SAMPLES);
	
	// average of non-extreme values
	
	acc=0;
	cnt=0;

	for(i=0.4*UI_POT_SAMPLES;i<0.6*UI_POT_SAMPLES;++i)
	{
		acc+=tmp[i];
		++cnt;
	}

	new=acc/cnt;
	
	// ignore small changes
	
	delta=abs(new-ui.value[pot]);

	if(delta>POT_CHANGE_DETECT_THRESHOLD || ui.lockTimeout[pot])
	{
		if (ui.lockTimeout[pot])
			--ui.lockTimeout[pot];
		
		if(delta>POT_TIMEOUT_THRESHOLD)
			ui.lockTimeout[pot]=POT_TIMEOUT;
		
		ui.value[pot]=new;

//		rprintf(0,"t %d v %d d %d\n",ui.lockTimeout[pot],new,delta);
	}
}


uint16_t ui_getPotValue(int8_t pot)
{
	return ui.value[pot]&0xffc0;
}

void ui_setPresetModified(int8_t modified)
{
	ui.presetModified=modified;
}

int8_t ui_isPresetModified(void)
{
	return ui.presetModified;
}


void ui_init(void)
{
	memset(&ui,0,sizeof(ui));
	
	// init screen serial
	
	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_UART1,CLKPWR_PCLKSEL_CCLK_DIV_1);	
	PINSEL_SetPinFunc(0,15,1);
	PINSEL_SetPinFunc(0,16,1);

	UART_CFG_Type uart;
	UART_ConfigStructInit(&uart);
	uart.Baud_rate=115200;
	UART_Init((LPC_UART_TypeDef*)LPC_UART1,&uart);
	
	UART_FIFO_CFG_Type fifo;
	UART_FIFOConfigStructInit(&fifo);
	UART_FIFOConfig((LPC_UART_TypeDef*)LPC_UART1,&fifo);
	
	UART_TxCmd((LPC_UART_TypeDef*)LPC_UART1,ENABLE);

	rprintf_devopen(1,sendChar);
	
	// init screen (autobaud)
	sendChar(0x0d);
	delay_ms(500);

	// welcome message
	rprintf(1,"Gli's OverCycler");
	rprintf(1,"    " __DATE__);
	sendCommand("[1b",0,NULL); // default at 115200bps
	delay_ms(500);
	sendCommand("[?27S",0,NULL); // save it as start screen
	delay_ms(500);

	// configure screen
	sendCommand("[6k",0,NULL); // ack = DISPLAY_ACK
	sendCommand("[20c",1,NULL); // screen size (20 cols)
	sendCommand("[4L",1,NULL); // screen size (4 rows)
	sendCommand("[?25I",1,NULL); // hide cursor
	sendCommand("[1x",1,NULL); // disable scrolling
	sendCommand("[2J",1,NULL); // clear screen
	
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

	LPC_ADC->ADCR=0x2f | ADC_CR_CLKDIV(15);
	LPC_ADC->ADINTEN=0x01;
	
	NVIC_SetPriority(ADC_IRQn,31);
	NVIC_EnableIRQ(ADC_IRQn);

	// start
	
	LPC_ADC->ADCR|= ADC_CR_PDN | ADC_CR_BURST;
}

void ui_update(void)
{
	rprintf(1,"                    ");
	for(int i=0;i<UI_POT_COUNT;++i)
	{
		updatePotValue(i);
		rprintf(1,"% 4d",ui_getPotValue(i)>>6);
	}
	rprintf(1,"                    ");

	readKeypad();
}
