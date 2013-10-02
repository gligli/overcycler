#ifndef SERIAL_H_
#define SERIAL_H_

extern void init_serial0 ( unsigned long baudrate );
extern void init_serial1 ( unsigned long baudrate );
extern int putchar_serial0 (int ch);
void putstring_serial0 (const char *string);
extern int putc_serial0 (int ch);
extern int putc_serial1 (int ch);
extern int getkey_serial0 (void);
extern int getkey_serial1 (void);
int getc_serial0 (void);
int getc_serial1 (void);

#endif
