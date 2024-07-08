/* Author: Domen Puncer <domen@cba.si>.  License: WTFPL, see file LICENSE */
/* Ported to lpc17xx by GliGli */
#include <system/hd44780.h>
#include <system/rprintf.h>
#include <system/main.h>
#include <drivers/lpc177x_8x_gpio.h>

#define HALF_CYCLE_DELAY() DELAY_100NS();DELAY_100NS();DELAY_100NS();DELAY_100NS();DELAY_100NS();

static void lcd_put(struct hd44780_data *lcd, int rs, int data)
{
	if(rs)
		GPIO_SetValue(lcd->port, 1<<lcd->pins.rs);
	else
		GPIO_ClearValue(lcd->port, 1<<lcd->pins.rs);
	GPIO_ClearValue(lcd->port, 1<<lcd->pins.rw);
	HALF_CYCLE_DELAY();
	GPIO_SetValue(lcd->port, 1<<lcd->pins.e);
	if(data&1)
		GPIO_SetValue(lcd->port, 1<<lcd->pins.d4);
	else
		GPIO_ClearValue(lcd->port, 1<<lcd->pins.d4);
	if((data>>1)&1)
		GPIO_SetValue(lcd->port, 1<<lcd->pins.d5);
	else
		GPIO_ClearValue(lcd->port, 1<<lcd->pins.d5);
	if((data>>2)&1)
		GPIO_SetValue(lcd->port, 1<<lcd->pins.d6);
	else
		GPIO_ClearValue(lcd->port, 1<<lcd->pins.d6);
	if((data>>3)&1)
		GPIO_SetValue(lcd->port, 1<<lcd->pins.d7);
	else
		GPIO_ClearValue(lcd->port, 1<<lcd->pins.d7);
	HALF_CYCLE_DELAY();
	GPIO_ClearValue(lcd->port, 1<<lcd->pins.e);
}

static void lcd_cmd(struct hd44780_data *lcd, int cmd, int long_delay)
{
	lcd_put(lcd, 0, cmd>>4);
	lcd_put(lcd, 0, cmd&0xf);
	delay_us(long_delay?2000:38);
}

static void lcd_data(struct hd44780_data *lcd, int cmd)
{
	lcd_put(lcd, 1, cmd>>4);
	lcd_put(lcd, 1, cmd&0xf);
	delay_us(38);
}

static void lcd_init(struct hd44780_data *lcd)
{
	GPIO_ClearValue(lcd->port, 1<<lcd->pins.rs);
	GPIO_ClearValue(lcd->port, 1<<lcd->pins.rw);
	GPIO_ClearValue(lcd->port, 1<<lcd->pins.e);
	GPIO_ClearValue(lcd->port, 1<<lcd->pins.d4);
	GPIO_ClearValue(lcd->port, 1<<lcd->pins.d5);
	GPIO_ClearValue(lcd->port, 1<<lcd->pins.d6);
	GPIO_ClearValue(lcd->port, 1<<lcd->pins.d7);
	
	GPIO_SetDir(lcd->port, 1<<lcd->pins.rs, 1);
	GPIO_SetDir(lcd->port, 1<<lcd->pins.rw, 1);
	GPIO_SetDir(lcd->port, 1<<lcd->pins.e, 1);
	GPIO_SetDir(lcd->port, 1<<lcd->pins.d4, 1);
	GPIO_SetDir(lcd->port, 1<<lcd->pins.d5, 1);
	GPIO_SetDir(lcd->port, 1<<lcd->pins.d6, 1);
	GPIO_SetDir(lcd->port, 1<<lcd->pins.d7, 1);

	/* reset sequence */
	lcd_put(lcd, 0, 3);
	delay_us(5000);
	lcd_put(lcd, 0, 3);
	delay_us(1000);
	lcd_put(lcd, 0, 3);
	delay_us(1000);

	lcd_put(lcd, 0, 2);
	delay_us(38);

	/* ok, in 4-bit mode now */
	int tmp = 0;
	if (lcd->caps & HD44780_CAPS_2LINES)
		tmp |= 1<<3;
	if (lcd->caps & HD44780_CAPS_5X10)
		tmp |= 1<<2;
	lcd_cmd(lcd, CMD_FUNCTION_SET | tmp, 0);
	lcd_cmd(lcd, CMD_DISPLAY_ON_OFF, 0); /* display, cursor and blink off */
	lcd_cmd(lcd, CMD_CLEAR, 1);

	lcd_cmd(lcd, CMD_ENTRY_MODE | HD44780_ENTRY_INC, 0);
}


static void lcd_clear(struct hd44780_data *lcd)
{
	lcd_cmd(lcd, CMD_CLEAR, 1);
}

static void lcd_home(struct hd44780_data *lcd)
{
	lcd_cmd(lcd, CMD_HOME, 1);
}

static void lcd_entry_mode(struct hd44780_data *lcd, int mode)
{
	lcd_cmd(lcd, CMD_ENTRY_MODE | (mode&0x3), 0);
}

static void lcd_onoff(struct hd44780_data *lcd, int features)
{
	lcd_cmd(lcd, CMD_DISPLAY_ON_OFF | (features&0x7), 0);
}

static void lcd_shift(struct hd44780_data *lcd, int shift)
{
	lcd_cmd(lcd, CMD_SHIFT | (shift&0xc), 0);
}

static void lcd_set_position(struct hd44780_data *lcd, int pos)
{
	lcd_cmd(lcd, CMD_DDRAM_ADDR | (pos&0x7f), 0);
}

static void lcd_puts(struct hd44780_data *lcd, const char *str)
{
	while (*str)
		lcd_data(lcd, *str++);
}


const struct hd44780_driver hd44780_driver = {
	.init = lcd_init,
	.clear = lcd_clear,
	.home = lcd_home,
	.entry_mode = lcd_entry_mode,
	.onoff = lcd_onoff,
	.shift = lcd_shift,
	.set_position = lcd_set_position,
	.write = lcd_data,
	.puts = lcd_puts,

	.write_cmd = lcd_cmd,
};
