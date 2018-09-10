#pragma once
#include <Audio.h>

extern const AudioSynthWavetable::sample_data trumpet_samples[5];
const uint8_t trumpet_ranges[] = {64, 69, 74, 79, 127, };

const AudioSynthWavetable::instrument_data trumpet = {5, trumpet_ranges, trumpet_samples };


extern const uint32_t sample_0_trumpet_htrumpetd2[896];

extern const uint32_t sample_1_trumpet_htrumpetg2[896];

extern const uint32_t sample_2_trumpet_htrumpetc3[896];

extern const uint32_t sample_3_trumpet_htrumpetf3[768];

extern const uint32_t sample_4_trumpet_htrumpetax3[896];
