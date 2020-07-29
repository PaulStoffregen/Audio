#pragma once
#include <Audio.h>

extern const AudioSynthWavetable::sample_data distortiongt_samples[4];
const uint8_t distortiongt_ranges[] = {62, 66, 72, 127, };

const AudioSynthWavetable::instrument_data distortiongt = {4, distortiongt_ranges, distortiongt_samples };


extern const uint32_t sample_0_distortiongt_distgtra2[1024];

extern const uint32_t sample_1_distortiongt_distgtre3[768];

extern const uint32_t sample_2_distortiongt_distgtra3[640];

extern const uint32_t sample_3_distortiongt_distgtrd4[896];
