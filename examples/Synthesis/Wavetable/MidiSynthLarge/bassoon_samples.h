#pragma once
#include <Audio.h>

extern const AudioSynthWavetable::sample_data bassoon_samples[2];
const uint8_t bassoon_ranges[] = {88, 127, };

const AudioSynthWavetable::instrument_data bassoon = {2, bassoon_ranges, bassoon_samples };


extern const uint32_t sample_0_bassoon_bassoonc2[640];

extern const uint32_t sample_1_bassoon_enghorndx3[896];
