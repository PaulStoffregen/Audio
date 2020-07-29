#pragma once
#include <Audio.h>

extern const AudioSynthWavetable::sample_data Pizzicato_samples[4];
const uint8_t Pizzicato_ranges[] = {68, 83, 93, 127, };

const AudioSynthWavetable::instrument_data Pizzicato = {4, Pizzicato_ranges, Pizzicato_samples };


extern const uint32_t sample_0_Pizzicato_PizzViolinE3[2944];

extern const uint32_t sample_1_Pizzicato_PizzViolinC4[2304];

extern const uint32_t sample_2_Pizzicato_PizzViolinE5[768];

extern const uint32_t sample_3_Pizzicato_PizzViolinE5[768];
