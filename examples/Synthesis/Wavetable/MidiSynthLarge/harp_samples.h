#pragma once
#include <Audio.h>

extern const AudioSynthWavetable::sample_data harp_samples[1];
const uint8_t harp_ranges[] = {127, };

const AudioSynthWavetable::instrument_data harp = {1, harp_ranges, harp_samples };


extern const uint32_t sample_0_harp_pluckharp[1792];
