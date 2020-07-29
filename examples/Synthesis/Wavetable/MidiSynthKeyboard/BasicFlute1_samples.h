#pragma once
#include <Audio.h>

extern const AudioSynthWavetable::sample_data BasicFlute1_samples[2];
const uint8_t BasicFlute1_ranges[] = {54, 127, };

const AudioSynthWavetable::instrument_data BasicFlute1 = {2, BasicFlute1_ranges, BasicFlute1_samples };


extern const uint32_t sample_0_BasicFlute1_BreathyFluteC2[28544];

extern const uint32_t sample_1_BasicFlute1_BreathyFluteA2[31616];
