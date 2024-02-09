#ifndef WAVE_READER_H
#define WAVE_READER_H

#include "ff.h"

typedef enum {
    WR_NO_ERROR = 0,
    WR_OPEN_ERROR,
    WR_IO_ERROR,
    WR_BAD_CONTENT,
} wave_reader_error;

typedef struct wave_reader {
    int format;
    int num_channels;
    int sample_rate;
    int sample_bits;
    int num_samples;
    FIL fp;
} wave_reader;

wave_reader_error wave_reader_open(const char *filename, wave_reader *wr);
void wave_reader_close(wave_reader *wr);
int wave_reader_get_format(wave_reader *wr);
int wave_reader_get_num_channels(wave_reader *wr);
int wave_reader_get_sample_rate(wave_reader *wr);
int wave_reader_get_sample_bits(wave_reader *wr);
int wave_reader_get_num_samples(wave_reader *wr);
int wave_reader_get_samples(wave_reader *wr, int n, void *buf);

#endif//WAVE_READER_H

