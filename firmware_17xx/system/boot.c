#include <string.h>

#include "version.h"
#include "rprintf.h"
#include "serial.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x_dac.h"
#include "hd44780.h"
#include "diskio.h"
#include "ff.h"
#include "iap/sbl_iap.h"
#include "usb/usbapi.h"
#include "usb/usb_msc.h"

////////////////////////////////////////////////////////////////////////////////
// delay
////////////////////////////////////////////////////////////////////////////////

// assumes 120Mhz clock
#define DELAY_50NS() asm volatile ("nop\nnop\nnop\nnop\nnop\nnop")
#define DELAY_100NS() {DELAY_50NS();DELAY_50NS();}

void delay_us(uint32_t count)
{
	uint32_t i;

	for (i = 0; i < count; i++)
	{
		DELAY_100NS();
		DELAY_100NS();
		DELAY_100NS();
		DELAY_100NS();
		DELAY_100NS();
		DELAY_100NS();
		DELAY_100NS();
		DELAY_100NS();
		DELAY_100NS();
		DELAY_100NS();
	}
}

void delay_ms(uint32_t count)
{
	delay_us(1000*count);
}

////////////////////////////////////////////////////////////////////////////////
// LCD
////////////////////////////////////////////////////////////////////////////////

#define UI_DEFAULT_LCD_CONTRAST 7
#define UI_MAX_LCD_CONTRAST 10

struct hd44780_data lcd1, lcd2;

static void setLcdContrast(uint8_t contrast)
{
	DAC_UpdateValue(0,(UI_MAX_LCD_CONTRAST-MIN(contrast,UI_MAX_LCD_CONTRAST))*300/UI_MAX_LCD_CONTRAST);
}

static int sendChar(int lcd, int ch)
{
	if(lcd==2)
		hd44780_driver.write(&lcd2,ch);	
	else
		hd44780_driver.write(&lcd1,ch);	
	return -1;
}

static int putc_lcd2(int ch)
{
	return sendChar(2,ch);
}

static void sendString(int lcd, const char * s)
{
	while(*s)
		sendChar(lcd, *s++);
}

static void clear(int lcd)
{
	if(lcd==2)
		hd44780_driver.clear(&lcd2);	
	else
		hd44780_driver.clear(&lcd1);	
}

static void setPos(int lcd, int col, int row)
{
	if(lcd==2)
		hd44780_driver.set_position(&lcd2,col+row*HD44780_LINE_OFFSET);	
	else
		hd44780_driver.set_position(&lcd1,col+row*HD44780_LINE_OFFSET);	
}

void initLcd(void)
{
	// init screen
	
	lcd1.port = 2;
	lcd1.pins.d4 = 2;
	lcd1.pins.d5 = 3;
	lcd1.pins.d6 = 0;
	lcd1.pins.d7 = 1;
	lcd1.pins.rs = 6;
	lcd1.pins.rw = 4;
	lcd1.pins.e = 5;
	lcd1.caps = HD44780_CAPS_2LINES;

	lcd2.port = 2;
	lcd2.pins.d4 = 2;
	lcd2.pins.d5 = 3;
	lcd2.pins.d6 = 0;
	lcd2.pins.d7 = 1;
	lcd2.pins.rs = 6;
	lcd2.pins.rw = 4;
	lcd2.pins.e = 7;
	lcd2.caps = HD44780_CAPS_2LINES;
	
	hd44780_driver.init(&lcd1);
	hd44780_driver.onoff(&lcd1, HD44780_ONOFF_DISPLAY_ON);
	hd44780_driver.init(&lcd2);
	hd44780_driver.onoff(&lcd2, HD44780_ONOFF_DISPLAY_ON);

	DAC_Init(0);
	setLcdContrast(UI_DEFAULT_LCD_CONTRAST);
	
	rprintf_devopen(1,putc_lcd2); 
}

////////////////////////////////////////////////////////////////////////////////
// keypad
////////////////////////////////////////////////////////////////////////////////

#define DEBOUNCE_THRESHOLD 10

enum scanKeypadButton_e
{
	kb0=0,kb1,kb2,kb3,kb4,kb5,kb6,kb7,kb8,kb9,
	kbA,kbB,kbC,kbD,
	kbSharp,kbAsterisk,
	
	// /!\ this must stay last
	kbCount
};

static uint8_t keypadButtonCode[kbCount]=
{
	0x32,0x01,0x02,0x04,0x11,0x12,0x14,0x21,0x22,0x24,
	0x08,0x18,0x28,0x38,
	0x34,0x31
};

int8_t keypadState[kbCount];

static void readKeypad(void)
{
	int key;
	int row,col[4];
	int8_t newState;

	for(row=0;row<4;++row)
	{
		LPC_GPIO0->FIOSET2=0b11110000;
		LPC_GPIO0->FIOCLR2=0b00010000<<row;
		
		delay_us(10);
	
		col[row]=(~LPC_GPIO0->FIOPIN2)&0b1111;
	}
	
	for(key=0;key<kbCount;++key)
	{
		newState=(col[keypadButtonCode[key]>>4]&keypadButtonCode[key])?1:0;
	
		if(newState && keypadState[key])
		{
			keypadState[key]=MIN(INT8_MAX,keypadState[key]+1);
		}
		else
		{
			keypadState[key]=newState;
		}
	}
}


void initKeypad(void)
{
	memset(&keypadState,0,sizeof(keypadState));

	// init keypad
	
	PINSEL_ConfigPin(0,16,0); // C1
	PINSEL_ConfigPin(0,17,0); // C2
	PINSEL_ConfigPin(0,18,0); // C3
	PINSEL_ConfigPin(0,19,0); // C4
	PINSEL_ConfigPin(0,20,0); // R1
	PINSEL_ConfigPin(0,21,0); // R2
	PINSEL_ConfigPin(0,22,0); // R3
	PINSEL_ConfigPin(0,23,0); // R4
	PINSEL_SetPinMode(0,16,PINSEL_BASICMODE_PULLUP);
	PINSEL_SetPinMode(0,17,PINSEL_BASICMODE_PULLUP);
	PINSEL_SetPinMode(0,18,PINSEL_BASICMODE_PULLUP);
	PINSEL_SetPinMode(0,19,PINSEL_BASICMODE_PULLUP);

	GPIO_SetValue(0,0b1111ul<<20);
	GPIO_SetDir(0,0b1111ul<<20,1);
}

////////////////////////////////////////////////////////////////////////////////
// boot
////////////////////////////////////////////////////////////////////////////////

typedef void __attribute__((noreturn))(*exec_t)();

static void boot(void)
{
    uint32_t *start;

    __set_MSP(*(uint32_t *)BOOT_MAX_SIZE);
    start = (uint32_t *)(BOOT_MAX_SIZE + 4);

	// reset pipeline, sync bus and memory access
	asm volatile (
		"dmb\n"
		"dsb\n"
		"isb\n"
	);

    ((exec_t)(*start))();
}	

////////////////////////////////////////////////////////////////////////////////
// flash
////////////////////////////////////////////////////////////////////////////////

#define FLASH_FILE_PATH "/overcycler.bin"

void flash_synth(FIL *file)
{
	clear(2);
	setPos(2,0,0);
	sendString(2,"Flashing firmware: Start!");

	static uint8_t readbuf[512];
	unsigned int readsize = sizeof(readbuf);
	uint32_t address = BOOT_MAX_SIZE;

	while (readsize == sizeof(readbuf))
	{
		if (f_read(file, readbuf, sizeof(readbuf), &readsize) != FR_OK)
		{
			f_close(file);
			return;
		}

		setPos(2,0,1);
		rprintf(1, "address: 0x%x, readsize: %d", address, readsize);

		write_flash((void *)address, (char *)readbuf, sizeof(readbuf));
		address += readsize;
	}

	f_close(file);
	if (address > BOOT_MAX_SIZE)
	{
		clear(2);
		setPos(2,0,0);
		sendString(2,"Flashing firmware: Complete!");
		setPos(2,0,1);
		sendString(2,"Rebooting...");
		
		delay_ms(2000);
		boot();
		for(;;);
	}
}

int main(void)
{
	int res,mode=-1;
	
	__disable_irq();
	USBHwPreinit(); // needs to be done ASAP at boot

	delay_ms(500);

	init_serial0(DEBUG_UART_BAUD);
	rprintf_devopen(0,putc_serial0); 

	rprintf(0,"\nBootLoader %d Hz\n",SystemCoreClock);

	initKeypad();
	
	for(int t=0;t<DEBOUNCE_THRESHOLD*2;++t)
	{
		readKeypad();
		if(keypadState[kb0]>=DEBOUNCE_THRESHOLD)
		{
			mode=0;
			break;
		}
		delay_ms(10);
	}
		
	if(mode==0)
	{
		initLcd();

		clear(1);
		clear(2);
		sendString(1,synthBLName);
		setPos(1,0,1);
		sendString(1,synthVersion);
		sendString(2,"Press 1 to go into USB disk mode");
		setPos(2,0,1);
		sendString(2,"Press 2 to flash " FLASH_FILE_PATH);
	
		for(;;)
		{
			readKeypad();
			if(keypadState[kb1]>=DEBOUNCE_THRESHOLD && !keypadState[kb0]) // check kb0 again to ensure keyboard not faulty
			{
				mode=1;
				break;
			}
			if(keypadState[kb2]>=DEBOUNCE_THRESHOLD && !keypadState[kb0]) // check kb0 again to ensure keyboard not faulty
			{
				mode=2;
				break;
			}
			delay_ms(10);
		}
		
		clear(2);
		
		if(mode==1)
		{
			sendString(2,"USB Disk mode, restart to quit");

			if((res=disk_initialize(0)))
			{
				sendString(2,"Error: disk_initialize failed!");
				for(;;);
			}

			usb_msc_start();
			for(;;)
			{
				USBHwISR();
			}
		}
		
		if(mode==2)
		{
			static FATFS fatFS;
			static FIL f;

			setPos(2,0,0);
			sendString(2,"Flash mode, please wait...");
			setPos(2,0,1);

			if((res=disk_initialize(0)))
			{
				sendString(2,"Error: disk_initialize failed!");
				for(;;);
			}

			if((res=f_mount(0,&fatFS)))
			{
				sendString(2,"Error: f_mount failed!");
				for(;;);
			}

			if((res=f_open(&f,FLASH_FILE_PATH,FA_READ|FA_OPEN_EXISTING)))
			{
				sendString(2,"Error: f_open failed!");
				for(;;);
			}
			
			if((res=f_lseek(&f,BOOT_MAX_SIZE)))
			{
				sendString(2,"Error: f_lseek failed!");
				for(;;);
			}
						
			flash_synth(&f);

			sendString(2,"Error: flash_synth failed!");
			for(;;);
		}
	}
	
	boot();
	for(;;);
}