#pragma once
#include <Audio.h>

extern const AudioSynthWavetable::sample_data flute_samples[1];
const uint8_t flute_ranges[] = {127, };

const AudioSynthWavetable::instrument_data flute = {1, flute_ranges, flute_samples };


extern const uint32_t sample_0_flute_flutec4[768];
