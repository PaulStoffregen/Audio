#pragma once
#include <Audio.h>

extern const AudioSynthWavetable::sample_data Viola_samples[8];
const uint8_t Viola_ranges[] = {58, 65, 68, 73, 79, 92, 96, 127, };

const AudioSynthWavetable::instrument_data Viola = {8, Viola_ranges, Viola_samples };


extern const uint32_t sample_0_Viola_ViolinBb2[768];

extern const uint32_t sample_1_Viola_ViolinD3[896];

extern const uint32_t sample_2_Viola_ViolinG3[768];

extern const uint32_t sample_3_Viola_ViolinC4[768];

extern const uint32_t sample_4_Viola_ViolinGb4[768];

extern const uint32_t sample_5_Viola_ViolinC5[640];

extern const uint32_t sample_6_Viola_ViolinEb5[768];

extern const uint32_t sample_7_Viola_ViolinEb6[512];
