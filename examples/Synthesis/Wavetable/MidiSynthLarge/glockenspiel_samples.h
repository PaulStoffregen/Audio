#pragma once
#include <Audio.h>

extern const AudioSynthWavetable::sample_data glockenspiel_samples[2];
const uint8_t glockenspiel_ranges[] = {59, 127, };

const AudioSynthWavetable::instrument_data glockenspiel = {2, glockenspiel_ranges, glockenspiel_samples };


extern const uint32_t sample_0_glockenspiel_sinetick[128];

extern const uint32_t sample_1_glockenspiel_sinetick[128];
