#pragma once
#include <Audio.h>

extern const AudioSynthWavetable::sample_data tuba_samples[2];
const uint8_t tuba_ranges[] = {86, 127, };

const AudioSynthWavetable::instrument_data tuba = {2, tuba_ranges, tuba_samples };


extern const uint32_t sample_0_tuba_tubaax1[1024];

extern const uint32_t sample_1_tuba_tromg4[896];
