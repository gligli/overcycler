////////////////////////////////////////////////////////////////////////////////
// LFO (low frequency oscillator)
////////////////////////////////////////////////////////////////////////////////

#include "lfo.h"
#include "lfo_lookups.h"
#include "scan.h"
#include "dacspi.h"

static inline void updateIncrement(struct lfo_s * lfo)
{
	lfo->increment=lfo->speed*(1-(lfo->halfPeriodCounter&1)*2);
}

static inline void updateSpeed(struct lfo_s * lfo)
{
	int32_t spd;
	
	spd=((1LL<<24)*scan_potFrom16bits(lfo->bpmCV))/(DACSPI_UPDATE_HZ*30);
	spd<<=lfo->speedShift;

	lfo->speed=spd;
}

static inline void handlePhaseOverflow(struct lfo_s * l)
{
	++l->halfPeriodCounter;
	l->phase=(l->halfPeriodCounter&1)?0x00ffffff:0;

	updateIncrement(l);
}

void lfo_setCVs(struct lfo_s * lfo, uint16_t bpm, uint16_t lvl)
{
	lfo->levelCV=lvl;

	if(bpm!=lfo->bpmCV)
	{
		lfo->bpmCV=bpm;
		updateSpeed(lfo);
		updateIncrement(lfo);
	}
}

void lfo_setShape(struct lfo_s * lfo, lfoShape_t shape, uint8_t halfPeriods)
{
	lfo->shape=shape;
	lfo->halfPeriodLimit=halfPeriods;
}

void lfo_setSpeedShift(struct lfo_s * lfo, int8_t shift)
{
	if(shift!=lfo->speedShift)
	{
		lfo->speedShift=shift;
		updateSpeed(lfo);
		updateIncrement(lfo);
	}
}

int16_t inline lfo_getOutput(struct lfo_s * lfo)
{
	return lfo->output;
}

const char * lfo_shapeName(lfoShape_t shape)
{
	switch(shape)
	{
	case lsPulse:
		return "pulse";
	case lsTri:
		return "tri";
	case lsRand:
		return "rand";
	case lsSine:
		return "sine";
	case lsNoise:
		return "noise";
	case lsSaw:
		return "saw";
	case lsRevSaw:
		return "rev-saw";
	}
	
	return "";
}

void lfo_reset(struct lfo_s * lfo)
{
	lfo->halfPeriodCounter=0;
	lfo->phase=0;
	
	updateIncrement(lfo);
}

void lfo_init(struct lfo_s * lfo)
{
	memset(lfo,0,sizeof(struct lfo_s));
	lfo->noise=random();
}

inline void lfo_update(struct lfo_s * l)
{
	int16_t rawOutput;
	
	// if bit 24 or higher is set, it's an overflow -> a half period is done!
	
	if(l->phase>>24) 
		handlePhaseOverflow(l);
	
	// handle shapes

	switch(l->shape)
	{
	case lsPulse:
		rawOutput=INT16_MAX;
		break;
	case lsTri:
		rawOutput=abs((int16_t)(l->phase>>8));
		break;
	case lsRand:
		if(!l->phase)
			l->noise=random();
		rawOutput=(l->noise&UINT16_MAX)+INT16_MIN;
		break;
	case lsSine:
		rawOutput=computeShape(l->phase,sineShape,2);
		break;
	case lsNoise:
		l->noise=lfsr(l->noise,(l->bpmCV>>12)+1);
		rawOutput=(l->noise&UINT16_MAX)+INT16_MIN;
		break;
	case lsSaw:
		rawOutput=l->phase>>9;
		break;
	case lsRevSaw:
		rawOutput=-(l->phase>>9);
		break;
	default:
		rawOutput=0;
	}

	if(l->halfPeriodCounter&1)
		rawOutput=-rawOutput;
	
	// compute output
	
	if(!l->bpmCV)
	{
		// constant output when BPM is zero
		
		l->output=l->levelCV>>1;
	}
	else if(!l->halfPeriodLimit || l->halfPeriodCounter<l->halfPeriodLimit)
	{
		// phase increment

		l->phase+=l->increment;

		// level

		l->output=scaleU16S16(l->levelCV,rawOutput);
	}
	else
	{
		// no oscillation -> no output
		
		l->output=0;
	}
}


