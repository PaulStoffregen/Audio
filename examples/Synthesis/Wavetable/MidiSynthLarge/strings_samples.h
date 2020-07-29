#pragma once
#include <Audio.h>

extern const AudioSynthWavetable::sample_data strings_samples[3];
const uint8_t strings_ranges[] = {59, 72, 127, };

const AudioSynthWavetable::instrument_data strings = {3, strings_ranges, strings_samples };


extern const uint32_t sample_0_strings_stringsg2[4736];

extern const uint32_t sample_1_strings_stringsf3[4352];

extern const uint32_t sample_2_strings_stringsdx4[5376];
