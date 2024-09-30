////////////////////////////////////////////////////////////////////////////////
// Usefull code not directly synth-related
////////////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "utils.h"

inline uint16_t satAddU16U16(uint16_t a, uint16_t b)
{
	uint16_t r;

	r =  (b > UINT16_MAX - a) ? UINT16_MAX : b + a;
	
	return r;
}

inline uint16_t satAddU16S32(uint16_t a, int32_t b)
{
	int32_t r;

	r=a;
	r+=b;
	r=__USAT(r,16);
	
	return r;
}

inline uint16_t satAddU16S16(uint16_t a, int16_t b)
{
	int32_t r;

	r=a;
	r+=b;
	r=__USAT(r,16);
	
	return r;
}

inline uint16_t lerp(uint16_t a,uint16_t b,uint8_t x)
{
	return a+(((uint32_t)x*(b-a))>>8);
}

inline int32_t lerp16(uint16_t a,uint16_t b,int32_t x)
{
	return a+((x*(b-a))>>16);
}

// max frac shift = 12
inline uint16_t herp(int32_t alpha, int32_t cur, int32_t prev, int32_t prev2, int32_t prev3, int8_t frac_shift)
{
	uint16_t r;
	int32_t v,p0,p1,p2,total;

	// do 4-point, 3rd-order Hermite/Catmull-Rom spline (x-form) interpolation

	v=prev2-prev;
	p0=((prev3-cur-v)>>1)-v;
	p1=cur+v*2-((prev3+prev)>>1);
	p2=(prev2-cur)>>1;

	total=((p0*alpha)>>frac_shift)+p1;
	total=((total*alpha)>>frac_shift)+p2;
	total=((total*alpha)>>frac_shift)+prev;

	r=__USAT(total,16);
	
	return r;
}

void resample(const uint16_t * src, uint16_t * dst, uint16_t src_samples, uint16_t dst_samples)
{
	if(src_samples==dst_samples)
	{
		memcpy(dst,src,src_samples*sizeof(uint16_t));
	}
	else
	{
		const int8_t frac_shift=14;
		int32_t counter=0, increment=(src_samples<<frac_shift)/dst_samples;
		int32_t c,p1,p2,p3;
		
		p1=src[(src_samples-((increment*1)>>frac_shift))%src_samples];
		p2=src[(src_samples-((increment*2)>>frac_shift))%src_samples];
		p3=src[(src_samples-((increment*3)>>frac_shift))%src_samples];
		
		for(uint16_t ds=0;ds<dst_samples;++ds)
		{
			c=src[counter>>frac_shift];
			
			*dst++=herp(counter&((1<<frac_shift)-1),c,p1,p2,p3,frac_shift);

			p3=p2;
			p2=p1;
			p1=c;
			
			counter+=increment;
		}
	}
}

// phase is 20 bits, from bit 4 to bit 23
inline uint16_t computeShape(uint32_t phase, const uint16_t lookup[], int8_t interpolate)
{
	if(interpolate==2)
	{
		uint8_t ci,p1i,p2i,p3i;
		int32_t x,c,p1,p2,p3;
		
		x=4095-((phase>>4)&4095);
		p1i=phase>>16;

		ci=p1i;
		if(p1i<UINT8_MAX)
			ci=p1i+1;

		p2i=p1i;
		if(p1i>0)
			p2i=p1i-1;

		p3i=p2i;
		if(p2i>0)
			p3i=p2i-1;

		c=lookup[ci];
		p1=lookup[p1i];
		p2=lookup[p2i];
		p3=lookup[p3i];

		return herp(x,c,p1,p2,p3,12);
	}
	else if(interpolate==1)
	{
		uint8_t ai,bi,x;
		uint16_t a,b;

		x=phase>>8;
		bi=ai=phase>>16;

		if(ai<UINT8_MAX)
			bi=ai+1;

		a=lookup[ai];
		b=lookup[bi];

		return lerp(a,b,x);
	}
	else
	{
		return lookup[phase>>16];
	}
}

inline uint16_t scaleU16U16(uint16_t a, uint16_t b)
{
	uint16_t r;
	
	r=((uint32_t)a*b)>>16;
	
	return r;
}

inline int16_t scaleU16S16(uint16_t a, int16_t b)
{
	uint16_t r;
	
	r=((int32_t)a*b)>>16;
	
	return r;
}

inline uint32_t lfsr(uint32_t v, uint8_t taps)
{
	uint8_t b24;
	
	while(taps--)
	{
		b24=v>>24;
		
		v<<=1;
		v|=((b24>>7)^(b24>>5)^(b24>>1)^b24)&1;
	}
	
	return v;
}

uint16_t exponentialCourse(uint16_t v, float ratio, float range)
{
	return expf(-(float)v/ratio)*range;
}

uint16_t linearCourse(uint16_t v, float ratio, float range)
{
	return -logf((float)v/range)*ratio;
}

int uint16Compare(const void * a,const void * b)
{
	return *(uint16_t*)a - *(uint16_t*)b;
}

int stringCompare(const void * a,const void * b)
{
	return strcasecmp((const char*)a,(const char*)b);
}

void buffer_dump(void * buf, int size){
	int i, osize = size;
	char* cbuf=(char*) buf;

	while(size>0){
		for(i=0;i<16;++i){
			rprintf(0, "%02x ",*cbuf);
			++cbuf;
			if ((uint32_t)cbuf - (uint32_t)buf >= osize)
				break;
		}
		rprintf(0, "\n");
		size-=16;
	}
}

