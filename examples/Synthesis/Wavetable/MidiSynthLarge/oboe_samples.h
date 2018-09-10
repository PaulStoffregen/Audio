#pragma once
#include <Audio.h>

extern const AudioSynthWavetable::sample_data oboe_samples[3];
const uint8_t oboe_ranges[] = {63, 68, 127, };

const AudioSynthWavetable::instrument_data oboe = {3, oboe_ranges, oboe_samples };


extern const uint32_t sample_0_oboe_oboecx3[512];

extern const uint32_t sample_1_oboe_oboefx3[640];

extern const uint32_t sample_2_oboe_oboeax3[512];
