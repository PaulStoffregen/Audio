#pragma once
#include <Audio.h>

extern const AudioSynthWavetable::sample_data vibraphone_samples[2];
const uint8_t vibraphone_ranges[] = {94, 127, };

const AudioSynthWavetable::instrument_data vibraphone = {2, vibraphone_ranges, vibraphone_samples };


extern const uint32_t sample_0_vibraphone_vibese2[512];

extern const uint32_t sample_1_vibraphone_vibese2[512];
