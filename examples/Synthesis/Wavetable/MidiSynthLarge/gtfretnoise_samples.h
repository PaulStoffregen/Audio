#pragma once
#include <Audio.h>

extern const AudioSynthWavetable::sample_data gtfretnoise_samples[1];
const uint8_t gtfretnoise_ranges[] = {127, };

const AudioSynthWavetable::instrument_data gtfretnoise = {1, gtfretnoise_ranges, gtfretnoise_samples };


extern const uint32_t sample_0_gtfretnoise_guitarfret[1792];
