#pragma once
#include <Audio.h>

extern const AudioSynthWavetable::sample_data mutedgtr_samples[1];
const uint8_t mutedgtr_ranges[] = {127, };

const AudioSynthWavetable::instrument_data mutedgtr = {1, mutedgtr_ranges, mutedgtr_samples };


extern const uint32_t sample_0_mutedgtr_mgtr[512];
