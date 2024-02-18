#ifndef UTILS_H
#define	UTILS_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <float.h>
#include <math.h>

#define FORCEINLINE inline __attribute__((always_inline))
#define NOINLINE __attribute__ ((noinline)) 
#define LOWERCODESIZE NOINLINE __attribute__((optimize ("Os")))

#undef MAX
#undef MIN
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

#define xstr(s) str(s)
#define str(s) #s

#define BP {rprintf(0,"[Breakpoint] in function %s, line %d, file %s\n",__FUNCTION__,__LINE__,__FILE__);for(;;);}
#define TR {rprintf(0,"[Trace] in function %s, line %d\n",__FUNCTION__,__LINE__);}

uint16_t satAddU16U16(uint16_t a, uint16_t b);
uint16_t satAddU16S32(uint16_t a, int32_t b);
uint16_t satAddU16S16(uint16_t a, int16_t b);

uint16_t scaleU16U16(uint16_t a, uint16_t b);
int16_t scaleU16S16(uint16_t a, int16_t b);

uint16_t lerp(uint16_t a,uint16_t b,uint8_t x);
uint16_t lerp16(uint16_t a,uint16_t b,uint32_t x);
uint16_t herp(int32_t alpha, int32_t cur, int32_t prev, int32_t prev2, int32_t prev3, int8_t frac_shift);
uint16_t computeShape(uint32_t phase, const uint16_t lookup[], int8_t interpolate);

uint32_t lfsr(uint32_t v, uint8_t taps);

uint16_t exponentialCourse(uint16_t v, float ratio, float range);
uint16_t linearCourse(uint16_t v, float ratio, float range); // reverse of exponentialCourse
		
int uint16Compare(const void * a,const void * b); // for qsort
int stringCompare(const void * a,const void * b); // for qsort

void buffer_dump(void * buf, int size);

#endif	/* UTILS_H */

