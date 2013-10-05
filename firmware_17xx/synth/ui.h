#ifndef UI_H
#define	UI_H

#include "synth.h"

typedef enum
{
	kb0=0xd7,kb1=0xee,kb2=0xde,kb3=0xbe,kb4=0xed,kb5=0xdd,kb6=0xbd,kb7=0xeb,kb8=0xdb,kb9=0xbb,
	kbA=0x7e,kbB=0x7d,kbC=0x7b,kbD=0x77,
	kbAsterisk=0xe7,kbSharp=0xb7
} uiKeypadButton_t;

void ui_init(void);
void ui_update(void);
uint16_t ui_getPotValue(int8_t pot);
void ui_setPresetModified(int8_t modified);
int8_t ui_isPresetModified(void);

#endif	/* UI_H */

