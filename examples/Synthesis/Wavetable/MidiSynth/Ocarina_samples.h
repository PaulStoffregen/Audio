#pragma once
#include <Audio.h>

extern const sample_data Ocarina_samples[3];
const uint8_t Ocarina_ranges[] = {78, 101, 127, };

const instrument_data Ocarina = {3, Ocarina_ranges, Ocarina_samples };


extern const uint32_t sample_0_Ocarina_OcarinaF4[1536];

extern const uint32_t sample_1_Ocarina_OcarinaF4[1536];

extern const uint32_t sample_2_Ocarina_OcarinaF6[256];
