#pragma once
#include <Audio.h>

extern const AudioSynthWavetable::sample_data overdrivegt_samples[3];
const uint8_t overdrivegt_ranges[] = {62, 66, 127, };

const AudioSynthWavetable::instrument_data overdrivegt = {3, overdrivegt_ranges, overdrivegt_samples };


extern const uint32_t sample_0_overdrivegt_distgtra2[1024];

extern const uint32_t sample_1_overdrivegt_distgtre3[768];

extern const uint32_t sample_2_overdrivegt_distgtra3[640];
