#pragma once
#include <Audio.h>

extern const AudioSynthWavetable::sample_data harmonica_samples[1];
const uint8_t harmonica_ranges[] = {127, };

const AudioSynthWavetable::instrument_data harmonica = {1, harmonica_ranges, harmonica_samples };


extern const uint32_t sample_0_harmonica_harmonicaa3[512];
