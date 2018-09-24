/* Audio Library for Teensy 3.X
 * Copyright (c) 2017, TeensyAudio PSU Team
 *
 * Development of this audio library was sponsored by PJRC.COM, LLC.
 * Please support PJRC's efforts to develop open source
 * software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include "Arduino.h"
#include "AudioStream.h"
#include <math.h>
#include <stdint.h>

#define WAVETABLE_CENTS_SHIFT(C) (pow(2.0, (C)/1200.0))
#define WAVETABLE_NOTE_TO_FREQUENCY(N) (440.0 * pow(2.0, ((N) - 69) / 12.0))
#define WAVETABLE_DECIBEL_SHIFT(dB) (pow(10.0, (dB)/20.0))

class AudioSynthWavetable : public AudioStream
{
public:
	struct sample_data {
		// SAMPLE VALUES
		const int16_t* sample;
		const bool LOOP;
		const int INDEX_BITS;
		const float PER_HERTZ_PHASE_INCREMENT;
		const uint32_t MAX_PHASE;
		const uint32_t LOOP_PHASE_END;
		const uint32_t LOOP_PHASE_LENGTH;
		const uint16_t INITIAL_ATTENUATION_SCALAR;

		// VOLUME ENVELOPE VALUES
		const uint32_t DELAY_COUNT;
		const uint32_t ATTACK_COUNT;
		const uint32_t HOLD_COUNT;
		const uint32_t DECAY_COUNT;
		const uint32_t RELEASE_COUNT;
		const int32_t SUSTAIN_MULT;

		// VIRBRATO VALUES
		const uint32_t VIBRATO_DELAY;
		const uint32_t VIBRATO_INCREMENT;
		const float VIBRATO_PITCH_COEFFICIENT_INITIAL;
		const float VIBRATO_PITCH_COEFFICIENT_SECOND;

		// MODULATION VALUES
		const uint32_t MODULATION_DELAY;
		const uint32_t MODULATION_INCREMENT;
		const float MODULATION_PITCH_COEFFICIENT_INITIAL;
		const float MODULATION_PITCH_COEFFICIENT_SECOND;
		const int32_t MODULATION_AMPLITUDE_INITIAL_GAIN;
		const int32_t MODULATION_AMPLITUDE_SECOND_GAIN;
	};
	static const int32_t UNITY_GAIN = INT32_MAX;
	static constexpr float SAMPLES_PER_MSEC = (AUDIO_SAMPLE_RATE_EXACT/1000.0);
	static const int32_t LFO_SMOOTHNESS = 3;
	static constexpr float LFO_PERIOD = (AUDIO_BLOCK_SAMPLES/(1 << (LFO_SMOOTHNESS-1)));
	static const int32_t ENVELOPE_PERIOD = 8;

	struct instrument_data {
		const uint8_t sample_count;
		const uint8_t* sample_note_ranges;
		const sample_data* samples;
	};
	enum { DEFAULT_AMPLITUDE = 90 };
	enum { TRIANGLE_INITIAL_PHASE = -0x40000000 };
	enum envelopeStateEnum { STATE_IDLE, STATE_DELAY, STATE_ATTACK, STATE_HOLD, STATE_DECAY, STATE_SUSTAIN, STATE_RELEASE };

public:
	/**
	 * Class constructor.
	 */
	AudioSynthWavetable(void) : AudioStream(0, NULL) {}

	/**
	 * @brief Set the instrument_data struct to be used as the playback instrument.
	 *
	 * A wavetable uses a set of samples to generate sound.
	 * This function is used to set the instrument samples.
	 * @param instrument a struct of type instrument_data, commonly prodced from a
	 * decoded SoundFont file using the SoundFont Decoder Script which accompanies this library.
	 */
	void setInstrument(const instrument_data& instrument) {
		cli();
		this->instrument = &instrument;
		current_sample = NULL;
		env_state = STATE_IDLE;
		state_change = true;
		sei();
	}

	/**
	 * @brief Changes the amplitude to 'v'
	 *
	 * A value of 0 will set the synth output to minimum amplitude
	 * (i.e., no output). A value of 1 will set the output to the
	 * maximum amplitude. Amplitude is set linearly with intermediate
	 * values.
	 * @param v a value between 0.0 and 1.0
	 */
	void amplitude(float v) {
		v = (v < 0.0) ? 0.0 : (v > 1.0) ? 1.0 : v;
		tone_amp = (uint16_t)(UINT16_MAX*v);
	}

	/**
	 * @brief Scale midi_amp to a value between 0.0 and 1.0
	 * using a logarithmic tranformation.
	 *
	 * @param midi_amp a value between 0 and 127
	 * @return a value between 0.0 to 1.0
	 */
	static float midi_volume_transform(int midi_amp) {
		// scale midi_amp which is 0 t0 127 to be between
		// 0 and 1 using a logarithmic transformation
		return powf(midi_amp / 127.0, 4);
	}

	/**
	 * @brief Convert a MIDI note value to
	 * its corresponding frequency.
	 *
	 * @param note a value between 0 and 127
	 * @return a frequency
	 */
	static float noteToFreq(int note) {
		float exp = note * (1.0 / 12.0) + 3.0313597;
		return powf(2.0, exp);
	}

	/**
	 * @brief Convert a frequency to the corressponding
	 * MIDI note value.
	 *
	 * @param freq the frequency value as a float to convert
	 * @return a MIDI note (between 0 - 127)
	 */
	static int freqToNote(float freq) {
		return 12*log2f(freq) - 35.8763164;
	}

	// Defined in AudioSynthWavetable.cpp
	void stop(void);
	// TODO: amplitude should be 0 to 1.0 scale
	void playFrequency(float freq, int amp = DEFAULT_AMPLITUDE);
	void playNote(int note, int amp = DEFAULT_AMPLITUDE);
	bool isPlaying(void) { return env_state != STATE_IDLE; }
	void setFrequency(float freq);
	virtual void update(void);

	envelopeStateEnum getEnvState(void) { return env_state; }

private:
	void setState(int note, int amp, float freq);
	volatile bool state_change = false;

	volatile const instrument_data* instrument = NULL;
	volatile const sample_data* current_sample = NULL;

	//sample output state
	volatile uint32_t tone_phase = 0;
	volatile uint32_t tone_incr = 0;
	volatile uint16_t tone_amp = 0;

	//volume environment state
	volatile envelopeStateEnum  env_state = STATE_IDLE;
	volatile int32_t env_count = 0;
	volatile int32_t env_mult = 0;
	volatile int32_t env_incr = 0;

	//vibrato LFO state
	volatile uint32_t vib_count = 0;
	volatile uint32_t vib_phase = 0;
	volatile int32_t vib_pitch_offset_init = 0;
	volatile int32_t vib_pitch_offset_scnd = 0;

	//modulation LFO state
	volatile uint32_t mod_count = 0;
	volatile uint32_t mod_phase = TRIANGLE_INITIAL_PHASE;
	volatile int32_t mod_pitch_offset_init = 0;
	volatile int32_t mod_pitch_offset_scnd = 0;
};

