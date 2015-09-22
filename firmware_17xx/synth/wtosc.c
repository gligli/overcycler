///////////////////////////////////////////////////////////////////////////////
// Wavetable oscillator
///////////////////////////////////////////////////////////////////////////////

#include "wtosc.h"
#include "dacspi.h"
#include "osc_curves.h"

#define VIRTUAL_CLOCK 3600000000UL
#define VIRTUAL_DAC_TICK_RATE (DACSPI_TICK_RATE*(VIRTUAL_CLOCK/SYNTH_MASTER_CLOCK))

#define MAX_SAMPLERATE(oversampling) (VIRTUAL_CLOCK/((VIRTUAL_DAC_TICK_RATE*(oversampling))/16)) /* oversampling in 1/16th */



static FORCEINLINE uint32_t cvToFrequency(uint32_t cv) // returns the frequency shifted by 8
{
	uint32_t v;
	
	v=cv%(12*WTOSC_CV_SEMITONE); // offset in the octave
	v=(v*20)<<8; // phase for computeShape
	v=(uint32_t)computeShape(v,oscOctaveCurve,1)+32768; // octave frequency in the 12th octave
	v=(v<<8)>>(12-(cv/(12*WTOSC_CV_SEMITONE))); // full frequency shifted by 8
	
	return v;
}

void wtosc_init(struct wtosc_s * o, int8_t voice, int8_t ab)
{
	memset(o,0,sizeof(struct wtosc_s));

	o->voice=voice;
	o->ab=ab;
	
	wtosc_setSampleData(o,NULL,-1);
	wtosc_setParameters(o,69*WTOSC_CV_SEMITONE,0,UINT16_MAX/2);
}

void wtosc_setSampleData(struct wtosc_s * o, uint16_t * data, uint16_t sampleCount)
{
	o->data=NULL;
	o->sampleCount=0;
	o->halfSampleCount=0;

	if(sampleCount>WTOSC_MAX_SAMPLES)
		return;

	o->data=data;
	o->sampleCount=sampleCount;
	o->halfSampleCount=sampleCount>>1;
}

void wtosc_setParameters(struct wtosc_s * o, uint16_t cv, uint16_t aliasing, uint16_t width)
{
	uint32_t sampleRate[2], underSample[2], maxSampleRate, frequency;
	
	aliasing>>=7;
	
	width=MAX(UINT16_MAX/16,width);
	width=MIN((15*UINT16_MAX)/16,width);
	width>>=6;

	cv=MIN(WTOSC_HIGHEST_NOTE*WTOSC_CV_SEMITONE,cv);
	
	if(cv==o->cv && aliasing==o->aliasing && width==o->width)
		return;	
	
	maxSampleRate=MAX_SAMPLERATE(16+(cv>>12)); // no oversampling for bass, more and more as we go higher
	frequency=cvToFrequency(cv)<<10;

	sampleRate[0]=frequency/(1023-width);
	sampleRate[1]=frequency/width;
	
	sampleRate[0]=sampleRate[0]*o->halfSampleCount;
	sampleRate[1]=sampleRate[1]*o->halfSampleCount;

	underSample[0]=1+(sampleRate[0]>>8)/maxSampleRate;
	underSample[1]=1+(sampleRate[1]>>8)/maxSampleRate;

	while(o->halfSampleCount%underSample[0]) ++underSample[0];
	while(o->halfSampleCount%underSample[1]) ++underSample[1];
	
	o->increment[0]=underSample[0]+aliasing;
	o->increment[1]=underSample[1]+aliasing;
	
	o->period[0]=(VIRTUAL_CLOCK/((sampleRate[0]/o->increment[0])>>8));	
	o->period[1]=(VIRTUAL_CLOCK/((sampleRate[1]/o->increment[1])>>8));	

	o->cv=cv;
	o->aliasing=aliasing;
	o->width=width;
		
//	rprintf(0,"inc %d %d cv %x rate % 6d % 6d\n",o->increment[0],o->increment[1],o->cv,sampleRate[0]/(o->increment[0]<<8),sampleRate[1]/(o->increment[1]<<8));
}

FORCEINLINE void wtosc_update(struct wtosc_s * o, int32_t startBuffer, int32_t endBuffer)
{
	uint16_t *data;
	uint16_t r;
	int32_t buf,counter,curPeriod,phase,curIncrement,sampleCount,halfCount,period[2],increment[2];
	uint32_t prevSample,curSample,aliasing;
#ifdef WTOSC_CUBIC_INTERP
	uint32_t prevSample2,prevSample3;
	int alpha2,alpha3,p0,p1,p2,p3,total;
#endif
	int alpha,voice,ab;
	
	data=o->data;
	
	if(!data)
		return;
	
	period[0]=o->period[0];
	period[1]=o->period[1];
	increment[0]=o->increment[0];
	increment[1]=o->increment[1];
	
	counter=o->counter;
	phase=o->phase;
	sampleCount=o->sampleCount;
	halfCount=o->halfSampleCount;
	curPeriod=period[phase>=halfCount?1:0];

#ifdef WTOSC_CUBIC_INTERP
	prevSample2=o->prevSample2;
	prevSample3=o->prevSample3;
#endif
	prevSample=o->prevSample;
	curSample=o->curSample;
	aliasing=o->aliasing;	
	voice=o->voice;
	ab=o->ab;
	
	for(buf=startBuffer;buf<=endBuffer;++buf)
	{
		counter-=VIRTUAL_DAC_TICK_RATE;
		if(counter<0)
		{
			if (phase>=halfCount)
			{
				curPeriod=period[1];
				curIncrement=increment[1];
			}
			else
			{
				curPeriod=period[0];
				curIncrement=increment[0];
			}
			
			counter+=curPeriod;

			phase-=curIncrement;

			if(phase<0)
				phase+=sampleCount;


#ifdef WTOSC_CUBIC_INTERP
			prevSample3=prevSample2;
			prevSample2=prevSample;
#endif
			prevSample=curSample;
			curSample=data[phase];
		}

		if(aliasing)
		{
			// we want aliasing, don't interpolate !

			r=curSample;
		}
		else
		{
			// prepare interpolation

			alpha=(counter<<12)/curPeriod;

#ifdef WTOSC_CUBIC_INTERP
			// do cubic interpolation

			alpha2=(alpha*alpha)>>12;
			alpha3=(alpha2*alpha)>>12;
			
			p0=prevSample3-prevSample2-curSample+prevSample;
			p1=curSample-prevSample-p0;
			p2=prevSample2-curSample;
			p3=prevSample;
			
			total=((p0*alpha3+p1*alpha2+p2*alpha)>>12)+p3;
			r=MAX(0,MIN(UINT16_MAX,total));
#else
			// do linear interpolation

			r=(alpha*prevSample+(4095-alpha)*curSample)>>12;
#endif
		}

		dacspi_setVoiceValue(buf,voice,ab,r);
	}
	
	o->counter=counter;
	o->phase=phase;
#ifdef WTOSC_CUBIC_INTERP
	o->prevSample2=prevSample2;
	o->prevSample3=prevSample3;
#endif
	o->prevSample=prevSample;
	o->curSample=curSample;
}
