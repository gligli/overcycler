////////////////////////////////////////////////////////////////////////////////
// ADSR envelope, based on electricdruid's ENVGEN7_MOOG.ASM
////////////////////////////////////////////////////////////////////////////////

/*
;  This program provides a versatile envelope generator on a single chip.
;  It is designed as a modern version of the CEM3312 or SSM2056 ICs.
;  Analogue output is provided as a PWM output, which requires LP
;  filtering to be useable.
;
;  Hardware Notes:
;   PIC16F684 running at 20 MHz using external crystal
; Six analogue inputs:
;   RA1/AN1: 0-5V Attack Time CV
;   RA2/AN2: 0-5V Decay Time CV
;   RC0/AN4: 0-5V Sustain Level CV
;   RC1/AN5: 0-5V Release Time CV
;   RC2/AN6: 0-5V Time Adjust CV (Keyboard CV or velocity, for example)
;   RC3/AN7: 0-5V Output Level CV
; Two digital inputs:
;   RA3: Gate Input
;   RC4: Exp/Lin Input (High is Linear)
; One digital output
;   RA0: Gate LED output
;
;  This version started as (ENVGEN4LIN.ASM), a test version without any of
;  the complications of the exponential output. Instead it passes
;  the linear PHASE value directly to the PWM output.
;  This should allow me to test the rest of the code, before trying to
;  add the complex (for a PIC) interpolation and lookp maths required for the 
;  exponential curve output 
;
;  29th Aug 06 - got this basically working.
;  2nd Sept 06 - ENVGEN5.ASM - Added exponential output.
; Still have 278 bytes left!
*/ 

#include "adsr.h"
#include "adsr_lookups.h"

static uint32_t getPhaseInc(uint8_t v)
{
	uint32_t r=0;
	
	r|=(uint32_t)phaseLookupLo[v];
	r|=(uint32_t)phaseLookupMid[v]<<8;
	r|=(uint32_t)phaseLookupHi[v]<<16;
	
	return r;
}

static inline void updateStageVars(struct adsr_s * a, adsrStage_t s)
{
	switch(s)
	{
	case sAttack:
		a->stageAdd=scaleU16U16(a->stageLevel,a->levelCV);
		a->stageMul=scaleU16U16(UINT16_MAX-a->stageLevel,a->levelCV);
		a->stageIncrement=a->attackIncrement;
		break;
	case sDecay:
		a->stageAdd=scaleU16U16(a->sustainCV,a->levelCV);
		a->stageMul=scaleU16U16(UINT16_MAX-a->sustainCV,a->levelCV);
		a->stageIncrement=a->decayIncrement;
		break;
	case sSustain:
		a->stageAdd=0;
		a->stageMul=a->levelCV;
		a->stageIncrement=0;
		break;
	case sRelease:
		a->stageAdd=0;
		a->stageMul=scaleU16U16(a->stageLevel,a->levelCV);
		a->stageIncrement=a->releaseIncrement;
		break;
	default:
		a->stageAdd=0;
		a->stageMul=0;
		a->stageIncrement=0;
	}
}

static LOWERCODESIZE void updateIncrements(struct adsr_s * adsr)
{
	uint32_t aInc, dInc, rInc;
	
	aInc=getPhaseInc(adsr->attackCV>>8)>>adsr->speedShift;
	dInc=getPhaseInc(adsr->decayCV>>8)>>adsr->speedShift;
	rInc=getPhaseInc(adsr->releaseCV>>8)>>adsr->speedShift;
	
	adsr->attackIncrement=aInc<<4; // phase is 20 bits, from bit 4 to bit 23
	adsr->decayIncrement=dInc<<4;
	adsr->releaseIncrement=rInc<<4;
	
	// immediate update of env settings
	
	updateStageVars(adsr,adsr->stage);
}


static inline uint16_t computeOutput(uint32_t phase, const uint16_t lookup[], int8_t isExp)
{
	if(isExp)
		return computeShape(phase,lookup,2);
	else
		return phase>>8; // 20bit -> 16 bit
}

static NOINLINE void handlePhaseOverflow(struct adsr_s * a)
{
	a->phase=0;
	a->stageIncrement=0;

	++a->stage;

	switch(a->stage)
	{
	case sDecay:
		a->output=a->levelCV;
		updateStageVars(a,sDecay);
		return;
	case sSustain:
		if (a->loop)
		{
			a->stageLevel=a->sustainCV;
			a->stage=sAttack;
			updateStageVars(a,sAttack);
		}
		else
		{
			updateStageVars(a,sSustain);
		}
		return;			
	case sDone:
		a->stage=sWait;
		a->output=0;
		return;
	default:
		;
	}
}

void adsr_setCVs(struct adsr_s * adsr, uint16_t atk, uint16_t dec, uint16_t sus, uint16_t rls, uint16_t lvl, uint8_t mask)
{
	int8_t m=mask&0x80;
	
	if(mask&0x01 && adsr->attackCV!=atk)
	{
		m=1;
		adsr->attackCV=atk;
	}
	
	if(mask&0x02 && adsr->decayCV!=dec)
	{
		m=1;
		adsr->decayCV=dec;
	}
	
	if(mask&0x04 && adsr->sustainCV!=sus)
	{
		m=1;
		adsr->sustainCV=sus;
	}
	
	if(mask&0x08 && adsr->releaseCV!=rls)
	{
		m=1;
		adsr->releaseCV=rls;
	}
	
	if(mask&0x10 && adsr->levelCV!=lvl)
	{
		m=1;
		adsr->levelCV=lvl;
	}

	if(m)
		updateIncrements(adsr);
}

void adsr_setGate(struct adsr_s * a, int8_t gate)
{
	a->phase=0;
	a->stageLevel=((uint32_t)a->output<<16)/a->levelCV;

	if(gate)
	{
		a->stage=sAttack;
		updateStageVars(a,sAttack);
	}
	else
	{
		a->stage=sRelease;
		updateStageVars(a,sRelease);
	}

	a->gate=gate;
}

void adsr_reset(struct adsr_s * adsr)
{
	adsr->gate=0;
	adsr->output=0;
	adsr->phase=0;
	adsr->stageLevel=0;
	adsr->stage=sWait;
	updateStageVars(adsr,sWait);
}

inline void adsr_setShape(struct adsr_s * adsr, int8_t isExp, int8_t isLoop)
{
	adsr->expOutput=isExp;
	adsr->loop=isLoop;
	
	if (adsr->loop && adsr->stage==sSustain)
	{
		// go out of sustain to start looping immediately
		adsr->stage=sDecay;
		handlePhaseOverflow(adsr);
	}
}

void adsr_setSpeedShift(struct adsr_s * adsr, int8_t shift)
{
	adsr->speedShift=shift;
	
	updateIncrements(adsr);
}

inline adsrStage_t adsr_getStage(struct adsr_s * adsr)
{
	return adsr->stage;
}

inline uint16_t adsr_getOutput(struct adsr_s * adsr)
{
	return adsr->output;
}

void adsr_init(struct adsr_s * adsr)
{
	memset(adsr,0,sizeof(struct adsr_s));
}

inline void adsr_update(struct adsr_s * a)
{
	// if bit 24 or higher is set, it's an overflow -> a timed stage is done!
	
	if(a->phase>>24)
		handlePhaseOverflow(a);
	
	// compute output level
	
	uint16_t o=0;
	
	switch(a->stage)
	{
	case sAttack:
		o=computeOutput(a->phase,attackCurveLookup,a->expOutput);
		break;
	case sDecay:
	case sRelease:
		o=UINT16_MAX-computeOutput(a->phase,decayCurveLookup,a->expOutput);
		break;
	case sSustain:
		o=a->sustainCV;
		break;
	default:
		;
	}
	
	a->output=scaleU16U16(o,a->stageMul)+a->stageAdd;

	// phase increment
	
	a->phase+=a->stageIncrement;
}

