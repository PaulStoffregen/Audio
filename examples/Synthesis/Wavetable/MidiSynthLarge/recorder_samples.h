#pragma once
#include <Audio.h>

extern const AudioSynthWavetable::sample_data recorder_samples[1];
const uint8_t recorder_ranges[] = {127, };

const AudioSynthWavetable::instrument_data recorder = {1, recorder_ranges, recorder_samples };


extern const uint32_t sample_0_recorder_recorderax2[768];
