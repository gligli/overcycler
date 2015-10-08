#ifndef UI_H
#define	UI_H

#include "synth.h"

void ui_init(void);
void ui_update(void);
void ui_setPresetModified(int8_t modified);
int8_t ui_isPresetModified(void);
int8_t ui_isTransposing(void);
int32_t ui_getTranspose(void);
void ui_setTranspose(int32_t transpose);

#endif	/* UI_H */

