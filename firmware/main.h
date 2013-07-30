#ifndef MAIN_H
#define MAIN_H

#define CPU_FREQ 60000000 // hz

#include <string.h>
#include <math.h>
#include <stdio.h>

#include "LPC214x.h"
#include "serial.h"
#include "rprintf.h"
#include "system.h"
#include "System/fat16.h"

extern struct fat16_fs_struct* fs;

#endif
