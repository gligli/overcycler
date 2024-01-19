#ifndef SERIAL_H_
#define SERIAL_H_

extern void init_serial0 ( unsigned long baudrate );
extern int putchar_serial0 (int ch);
void putstring_serial0 (const char *string);
extern int putc_serial0 (int ch);

#endif
