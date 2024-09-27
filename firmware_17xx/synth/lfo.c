////////////////////////////////////////////////////////////////////////////////
// LFO (low frequency oscillator)
////////////////////////////////////////////////////////////////////////////////

#include "lfo.h"
#include "lfo_lookups.h"
#include "scan.h"
#include "dacspi.h"

static void updateIncrement(struct lfo_s * lfo)
{
	lfo->increment=lfo->speed*(1-lfo->halfPeriod*2);
}

static void updateSpeed(struct lfo_s * lfo)
{
	int32_t spd;
	
	spd=((1LL<<24)*scan_potFrom16bits(lfo->bpmCV))/(DACSPI_UPDATE_HZ*30);
	spd<<=lfo->speedShift;

	lfo->speed=spd;
}

static void handlePhaseOverflow(struct lfo_s * l)
{
	l->halfPeriod=1-l->halfPeriod;
	l->phase=l->halfPeriod?0x00ffffff:0;

	updateIncrement(l);

	switch(l->shape)
	{
	case lsPulse:
		l->rawOutput=l->halfPeriod*UINT16_MAX;
		break;
	case lsRand:
		l->rawOutput=random();
		break;
	default:
		;
	}
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

void lfo_setShape(struct lfo_s * lfo, lfoShape_t shape)
{
	lfo->shape=shape;
	
	lfo->noise=random();
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

void lfo_init(struct lfo_s * lfo)
{
	memset(lfo,0,sizeof(struct lfo_s));
}

inline void lfo_update(struct lfo_s * l)
{
	// if bit 24 or higher is set, it's an overflow -> a half period is done!
	
	if(l->phase>>24) 
		handlePhaseOverflow(l);
	
	// handle continuous shapes

	switch(l->shape)
	{
	case lsTri:
		l->rawOutput=l->phase>>8;
		break;
	case lsSine:
		l->rawOutput=computeShape(l->phase,sineShape,2);
		break;
	case lsNoise:
		l->noise=lfsr(l->noise,(l->bpmCV>>12)+1);
		l->rawOutput=l->noise;
		break;
	case lsSaw:
		l->rawOutput=l->phase>>9;
		if(l->halfPeriod)
			l->rawOutput=UINT16_MAX-l->rawOutput;
		break;
	case lsRevSaw:
		l->rawOutput=l->phase>>9;
		if(!l->halfPeriod)
			l->rawOutput=UINT16_MAX-l->rawOutput;
		break;
	default:
		;
	}
	
	// phase increment
	
	l->phase+=l->increment;

	// compute output
	
	l->output=scaleU16S16(l->levelCV,(int32_t)l->rawOutput+INT16_MIN);
}


