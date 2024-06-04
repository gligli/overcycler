#ifndef _HD44780_H_
#define _HD44780_H_

struct hd44780_data {
	int port;
	struct {
		int rs, rw, e;
		int d4, d5, d6, d7;
	} pins;
	int caps;
};

#define HD44780_CAPS_2LINES     (1<<0)
#define HD44780_CAPS_5X10       (1<<1)

#define HD44780_ENTRY_INC       (1<<1)
#define HD44780_ENTRY_SHIFT     (1<<0)

#define HD44780_ONOFF_DISPLAY_ON      (1<<2)
#define HD44780_ONOFF_CURSOR_ON       (1<<1)
#define HD44780_ONOFF_BLINKING_ON     (1<<0)

#define HD44780_SHIFT_DISPLAY   (1<<3)
#define HD44780_SHIFT_CURSOR    (0<<3)
#define HD44780_SHIFT_RIGHT     (1<<2)
#define HD44780_SHIFT_LEFT      (0<<2)

#define HD44780_LINE_OFFSET	0x40

/* local defines */
enum hd44780_commands {
	CMD_CLEAR =             0x01,
	CMD_HOME =              0x02,
	CMD_ENTRY_MODE =        0x04, /* args: I/D, S */
	CMD_DISPLAY_ON_OFF =    0x08, /* args: D, C, B */
	CMD_SHIFT =             0x10, /* args: S/C, R/L, -, - */
	CMD_FUNCTION_SET =      0x20, /* args: DL, N, F, -, -; only valid at reset */
	CMD_CGRAM_ADDR =        0x40,
	CMD_DDRAM_ADDR =        0x80,
};

struct hd44780_driver {
	void (*init)(struct hd44780_data *lcd);
	void (*clear)(struct hd44780_data *lcd);
	void (*home)(struct hd44780_data *lcd);
	void (*entry_mode)(struct hd44780_data *lcd, int mode);
	void (*onoff)(struct hd44780_data *lcd, int features);
	void (*shift)(struct hd44780_data *lcd, int shift);
	void (*set_position)(struct hd44780_data *lcd, int pos);
	void (*write)(struct hd44780_data *lcd, int data);
	void (*puts)(struct hd44780_data *lcd, const char *str);

	/* low level, not to be used */
	void (*write_cmd)(struct hd44780_data *lcd, int cmd, int long_delay);
};

extern const struct hd44780_driver hd44780_driver;

#endif
