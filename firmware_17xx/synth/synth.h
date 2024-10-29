#ifndef SYNTH_H
#define SYNTH_H

#include "main.h"
#include "midi.h"
#include "utils.h"

#define SYNTH_VOICE_COUNT 6
//#define SYNTH_MASTER_CLOCK SystemCoreClock
#define SYNTH_MASTER_CLOCK 120000000

#define SYNTH_WAVEDATA_PATH "/WAVEDATA"
#define SYNTH_PRESETS_PATH "/PRESETS"
#define SYNTH_SEQUENCES_PATH "/SEQUENCES"

#define SYNTH_DEFAULT_MAIN_WAVE_BANK "_basic"
#define SYNTH_DEFAULT_MAIN_WAVE_NAME "saw.wav"

#define SYNTH_DEFAULT_XOVR_WAVE_BANK "_basic"
#define SYNTH_DEFAULT_XOVR_WAVE_NAME "sin.wav"

#define TICKER_HZ 500

// Some constants for 16 bit ranges */
#define FULL_RANGE UINT16_MAX
#define HALF_RANGE (FULL_RANGE/2+1)
#define HALF_RANGE_L (65536UL*HALF_RANGE) // i.e. HALF_RANGE<<16, as uint32_t

#define SCANNER_BASE_NOTE 12
#define MIDDLE_C_NOTE 60

typedef enum {
	cvAVol=0,cvBVol=1,cvCutoff=2,cvResonance=3,cvAPitch=4,cvBPitch=5,cvWaveMod=6,cvAmp=7,cvNoiseVol=8,
			
	// /!\ this must stay last
	cvCount
} cv_t; 

typedef enum
{
	modNone=0,modPitch=1,modFilter=2,modVolume=3,modWaveMod=4,modLFO1=5,modLFO2=6
} modulationTarget_t;

typedef enum
{
	otNone=0,otA=1,otB=2,otBoth=3
} oscTarget_t;

typedef enum
{
	abxNone=-1,abxAMain=0,abxBMain,abxACrossover,abxBCrossover,
	
	// /!\ this must stay last
	abxCount
}abx_t;

void synth_tickTimerEvent(uint8_t phase);
void synth_updateCVsEvent(void);
void synth_updateOscsEvent(int32_t start, int32_t count);
void synth_uartMIDIEvent(uint8_t data);
void synth_usbMIDIEvent(uint8_t data);
void synth_assignerEvent(uint8_t note, int8_t gate, int8_t voice, uint16_t velocity, uint8_t flags);
void synth_wheelEvent(int16_t bend, uint16_t modulation, uint8_t mask);
void synth_pressureEvent(uint16_t pressure);
void synth_realtimeEvent(midiPort_t port, uint8_t midiEvent);
void synth_clockEvent(void);

void synth_init(void);
void synth_update(void);

// synth.c internal api
void synth_refreshFullState(int8_t refreshWaveforms);
int8_t synth_refreshBankNames(int8_t sort, int8_t force);
void synth_refreshCurWaveNames(abx_t abx, int8_t sort);
void synth_refreshWaveforms(abx_t abx);
int synth_getBankCount(void);
int synth_getCurWaveCount(void);
int8_t synth_getBankName(int bankIndex, char * res);
int8_t synth_getWaveName(int waveIndex, char * res);
int32_t synth_getVisualEnvelope(int8_t voice);
uint16_t * synth_getWaveformData(abx_t abx);
void synth_refreshCV(int8_t voice, cv_t cv, uint32_t v, int8_t noDblBuf);
void synth_updateAssignerPattern(void);
void synth_silenceSynth(void);

extern volatile uint32_t currentTick; // 500hz
extern const uint16_t extClockDividers[16];
extern const uint8_t abx2bsp[abxCount];
extern const uint8_t abx2wsp[abxCount];
extern const abx_t sp2abx[];
extern const char * notesNames[12];

#endif
