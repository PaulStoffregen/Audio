#pragma once
#include <Audio.h>

extern const AudioSynthWavetable::sample_data piano_samples[4];
const uint8_t piano_ranges[] = {54, 74, 88, 127, };

const AudioSynthWavetable::instrument_data piano = {4, piano_ranges, piano_samples };


extern const uint32_t sample_0_piano_kpianob1[8704];

extern const uint32_t sample_1_piano_kpianocx4[10880];

extern const uint32_t sample_2_piano_kpianodx5[3200];

extern const uint32_t sample_3_piano_kpianof502[2304];
