#pragma once
#include <Audio.h>

extern const AudioSynthWavetable::sample_data clarinet_samples[2];
const uint8_t clarinet_ranges[] = {69, 127, };

const AudioSynthWavetable::instrument_data clarinet = {2, clarinet_ranges, clarinet_samples };


extern const uint32_t sample_0_clarinet_clarinetd2[384];

extern const uint32_t sample_1_clarinet_clarinetb2[384];
