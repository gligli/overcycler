#ifndef rprintf_h_
#define rprintf_h_

extern void rprintf_devopen(int channel, int(*put)(int));
extern void rprintf(int channel, char const *format, ...);

#endif