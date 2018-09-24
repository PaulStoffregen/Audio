#pragma once
#include <Audio.h>

extern const AudioSynthWavetable::sample_data steelstrgtr_samples[2];
const uint8_t steelstrgtr_ranges[] = {72, 127, };

const AudioSynthWavetable::instrument_data steelstrgtr = {2, steelstrgtr_ranges, steelstrgtr_samples };


extern const uint32_t sample_0_steelstrgtr_acgtrg2[2560];

extern const uint32_t sample_1_steelstrgtr_acgtrb3[3200];
