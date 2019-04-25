////////////////////////////////////////////////////////////////////////////////
// Potnetiometers / keypad scanner
////////////////////////////////////////////////////////////////////////////////

#include "scan.h"

#include "ui.h"
#include "LPC17xx.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_gpio.h"

#define ADC_QUANTUM 64 // 10 bit

#define POT_SAMPLES 5
#define POT_THRESHOLD (ADC_QUANTUM*6)
#define POT_TIMEOUT_THRESHOLD (ADC_QUANTUM*3)
#define POT_TIMEOUT (TICKER_1S)

#define DEBOUNCE_THRESHOLD 5

static const uint8_t keypadButtonCode[16]=
{
	0x01,0x02,0x04,0x11,0x12,0x14,0x21,0x22,0x24,0x32,
	0x08,0x18,0x28,0x38,
	0x34,0x31
};

static struct
{
	int16_t curPotSample;

	uint16_t potSamples[POT_COUNT][POT_SAMPLES];
	uint16_t potValue[POT_COUNT];
	uint32_t potLockTimeout[POT_COUNT];
	uint32_t potLockValue[POT_COUNT];

	int8_t keypadState[16];
} scan;

static void readPots(void)
{
	uint32_t new;
	int i,pot,nextPot;
	uint16_t tmp[POT_SAMPLES];
	
	scan.curPotSample=(scan.curPotSample+1)%POT_SAMPLES;
	
	GPIO_ClearValue(0,1<<24); // CS
	delay_us(8);

	for(pot=0;pot<POT_COUNT;++pot)
	{
		// read pot on TLV1543 ADC

		new=0;
		nextPot=(pot+1)%POT_COUNT;
		
		BLOCK_INT(1)
		{
			for(i=0;i<16;++i)
			{
				// read value back

				if(i<10)
					new|=(LPC_GPIO0->FIOPIN3&(1<<(25-24)))?(1<<(15-i)):0;

				// send next address

				if(i<4)
				{
					if(nextPot&(1<<(3-i)))
						LPC_GPIO0->FIOSET3=1<<(26-24); // ADDR 
					else
						LPC_GPIO0->FIOCLR3=1<<(26-24); // ADDR 
				}

				// wiggle clock

				DELAY_100NS();DELAY_100NS();DELAY_100NS();DELAY_100NS();
				DELAY_100NS();DELAY_100NS();DELAY_100NS();DELAY_100NS();
				LPC_GPIO0->FIOSET3=1<<(27-24); // CLK 
				DELAY_100NS();DELAY_100NS();DELAY_100NS();DELAY_100NS();
				DELAY_100NS();DELAY_100NS();DELAY_100NS();DELAY_100NS();
				LPC_GPIO0->FIOCLR3=1<<(27-24); // CLK 
				DELAY_100NS();DELAY_100NS();DELAY_100NS();DELAY_100NS();
			}
		}

		scan.potSamples[pot][scan.curPotSample]=new;
		
//		if(ui.activePage==upNone)
//			rprintf(0,"% 8d", new>>6);

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

	GPIO_SetValue(0,1<<24); // CS
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

	// init TLV1543 ADC
	
	PINSEL_SetI2C0Pins(PINSEL_I2C_Fast_Mode, DISABLE);
	
	PINSEL_SetPinFunc(0,24,0); // CS
	PINSEL_SetPinFunc(0,25,0); // OUT
	PINSEL_SetPinFunc(0,26,0); // ADDR
	PINSEL_SetPinFunc(0,27,0); // CLK
	PINSEL_SetPinFunc(0,28,0); // EOC
	
	GPIO_SetValue(0,0b01101ul<<24);
	GPIO_SetDir(0,0b01101ul<<24,1);

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
}

void scan_update(void)
{
	readKeypad();
	readPots();
}
