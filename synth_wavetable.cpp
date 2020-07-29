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

#include <Arduino.h>
#include "synth_wavetable.h"
#include <dspinst.h>
#include <SerialFlash.h>

//#define TIME_TEST_ON
//#define ENVELOPE_DEBUG

// Performance testing macro generally unrelated to the wavetable object, but was used to
// fine tune the performance of specific blocks of code in update(); usage is to specify a
// display interval in ms, then place the code block to be tracked *IN* the macro parens as
// the second argument
#ifdef TIME_TEST_ON
#define TIME_TEST(INTERVAL, CODE_BLOCK_TO_TEST) \
static float MICROS_AVG = 0.0; \
static int TEST_CUR_CNT = 0; \
static int TEST_LST_CNT = 0; \
static int NEXT_DISPLAY = 0; \
static int TEST_TIME_ACC = 0; \
int micros_start = micros(); \
CODE_BLOCK_TO_TEST \
int micros_end = micros(); \
TEST_TIME_ACC += micros_end - micros_start; \
++TEST_CUR_CNT; \
if (NEXT_DISPLAY < micros_end) { \
	MICROS_AVG += (TEST_TIME_ACC - TEST_CUR_CNT * MICROS_AVG) / (TEST_LST_CNT + TEST_CUR_CNT); \
	NEXT_DISPLAY = micros_end + INTERVAL*1000; \
	TEST_LST_CNT += TEST_CUR_CNT; \
	TEST_TIME_ACC = TEST_CUR_CNT = 0; \
	Serial.printf("avg: %f, n: %i\n", MICROS_AVG, TEST_LST_CNT); \
}
#else
#define TIME_TEST(INTERVAL, CODE_BLOCK_TO_TEST) do { } while(0); \
CODE_BLOCK_TO_TEST
#endif

// Debug code to track state variables for the volume envelope
#ifdef ENVELOPE_DEBUG
#define PRINT_ENV(NAME) Serial.printf("%14s-- env_mult:%06.4f%% of UNITY_GAIN env_incr:%06.4f%% of UNITY_GAIN env_count:%i\n", #NAME, float(env_mult)/float(UNITY_GAIN), float(env_incr)/float(UNITY_GAIN), env_count);
#else
#define PRINT_ENV(NAME) do { } while(0);
#endif


/**
 * @brief Stop playing waveform.
 *
 * Waveform does not immediately stop,
 * but fades out based on release time.
 *
 */
void AudioSynthWavetable::stop(void) {
	cli();
	if (env_state != STATE_IDLE) {
		env_state = STATE_RELEASE;
		env_count = current_sample->RELEASE_COUNT;
		if (env_count == 0) env_count = 1;
		env_incr = -(env_mult) / (env_count * ENVELOPE_PERIOD);
	}
	PRINT_ENV(STATE_RELEASE);
	sei();
}

/**
 * @brief Play waveform at defined frequency, amplitude.
 *
 * @param freq Frequency of note to playback, value between 1.0 and half of AUDIO_SAMPLE_RATE_EXACT
 * @param amp Amplitude scaling of note, value between 0-127, with 127 being base volume
 */
void AudioSynthWavetable::playFrequency(float freq, int amp) {
	setState(freqToNote(freq), amp, freq);
}

/**
 * @brief Play sample at specified note, amplitude.
 *
 * @param note Midi note number to playback, value between 0-127
 * @param amp Amplitude scaling of playback, value between 0-127, with 127 being base volume
 */
void AudioSynthWavetable::playNote(int note, int amp) {
	setState(note, amp, noteToFreq(note));
}

/**
 * @brief Initializes object state variables, sets freq/amp, and chooses appropriate sample
 *
 * @param note Midi note number to play, value between 0-127
 * @param amp the amplitude level at which playback should occur
 * @param freq exact frequency of the note to be played played
 */
void AudioSynthWavetable::setState(int note, int amp, float freq) {
	cli();
	int i;
	env_state = STATE_IDLE;
	// note ranges calculated by sound font decoder
	for (i = 0; note > instrument->sample_note_ranges[i]; i++);
	current_sample = &instrument->samples[i];
	if (current_sample == NULL) {
		sei();
		return;
	}
	setFrequency(freq);
	vib_count = mod_count = tone_phase = env_incr = env_mult = 0;
	vib_phase = mod_phase = TRIANGLE_INITIAL_PHASE;
	env_count = current_sample->DELAY_COUNT;
	// linear scalar for amp with UINT16_MAX being no attenuation
	tone_amp = amp * (UINT16_MAX / 127);
	// scale relative to initial attenuation defined by soundfont file
	tone_amp = current_sample->INITIAL_ATTENUATION_SCALAR * tone_amp >> 16;
	env_state = STATE_DELAY;
	PRINT_ENV(STATE_DELAY);
	state_change = true;
	sei();
}

/**
 * @brief Set various integer offsets to values that will produce intended frequencies
 * @details the main integer offset, tone_incr, is used to step through the current sample's 16-bit PCM audio sample.
 * Specifically, the tone_incr is the rate at which the interpolation code in update() steps through uint32_t space.
 * The remaining offset variables represent a minimum and maximum offset allowed for tone_incr, which allows for low-frequency
 * variation in playback frequency (aka vibrato). Further details on implementation in update() and in sample_data.h.
 *
 * @param freq frequency of the generated output (between 0 and the board-specific sample rate)
 */
void AudioSynthWavetable::setFrequency(float freq) {
	float tone_incr_temp = freq * current_sample->PER_HERTZ_PHASE_INCREMENT;
	tone_incr = tone_incr_temp;
	vib_pitch_offset_init = tone_incr_temp * current_sample->VIBRATO_PITCH_COEFFICIENT_INITIAL;
	vib_pitch_offset_scnd = tone_incr_temp * current_sample->VIBRATO_PITCH_COEFFICIENT_SECOND;
	mod_pitch_offset_init = tone_incr_temp * current_sample->MODULATION_PITCH_COEFFICIENT_INITIAL;
	mod_pitch_offset_scnd = tone_incr_temp * current_sample->MODULATION_PITCH_COEFFICIENT_SECOND;
}

/**
 * @brief Called by the AudioStream library to fill the audio output buffer.
 * The major parts are the interpoalation stage, and the volume envelope stage.
 * Further details on implementation included inline.
 *
 */
void AudioSynthWavetable::update(void) {
#if defined(KINETISK) || defined(__IMXRT1062__)
	// exit if nothing to do
	if (env_state == STATE_IDLE || (current_sample->LOOP == false && tone_phase >= current_sample->MAX_PHASE)) {
		env_state = STATE_IDLE;
		return;
	}
	// else locally copy object state and continue
	this->state_change = false;

	const sample_data* s = (const sample_data*)current_sample;
	uint32_t tone_phase = this->tone_phase;
	uint32_t tone_incr = this->tone_incr;
	uint16_t tone_amp = this->tone_amp;

	envelopeStateEnum  env_state = this->env_state;
	int32_t env_count = this->env_count;
	int32_t env_mult = this->env_mult;
	int32_t env_incr = this->env_incr;

	uint32_t vib_count = this->vib_count;
	uint32_t vib_phase = this->vib_phase;
	int32_t vib_pitch_offset_init = this->vib_pitch_offset_init;
	int32_t vib_pitch_offset_scnd = this->vib_pitch_offset_scnd;

	uint32_t mod_count = this->mod_count;
	int32_t mod_phase = this->mod_phase;
	int32_t mod_pitch_offset_init = this->mod_pitch_offset_init;
	int32_t mod_pitch_offset_scnd = this->mod_pitch_offset_scnd;

	audio_block_t* block;
	block = allocate();
	if (block == NULL) return;

	uint32_t* p, *end;
	uint32_t index, phase_scale;
	int32_t s1, s2;
	uint32_t tmp1, tmp2;

	// filling audio_block two samples at a time
	p = (uint32_t*)block->data;
	end = p + AUDIO_BLOCK_SAMPLES / 2;

	// Main loop to handle interpolation, vibrato (vibrato LFO and modulation LFO), and tremolo (modulation LFO only)
	// Virbrato and modulation offsets/multipliers are updated depending on the LFO_SMOOTHNESS, with max smoothness (7) being one
	// update per loop interation, and minimum smoothness (1) being once per loop. Hence there is a configurable trade-off
	// between performance and the smoothness of LFO changes to pitch/amplitude as well as the vibrato/modulation delay granularity

	// also note that the vibrato/tremolo for the two LFO are defined in the SoundFont spec to be a cents (vibrato) or centibel (tremolo)
	// diviation oscillating with a triangle wave at a given frequency; the following implementation gets the critical points of those
	// oscillations correct, but linearly interpolates the *frequency* and *amplitude* range between those points, which technically results
	// in a "bowing" of the triangle wave curve relative to what it should be (although this typically isn't audible)
	while (p < end) {
		// TODO: more elegant support of non-looping samples
		if (s->LOOP == false && tone_phase >= s->MAX_PHASE) break;

		// variable to accumulate LFO pitch offsets; stays 0 if still in vibrato/modulation delay
		int32_t tone_incr_offset = 0; 
		if (vib_count++ > s->VIBRATO_DELAY) {
			vib_phase += s->VIBRATO_INCREMENT;
			// convert uint32_t phase value to int32_t triangle wave value
			// TRIANGLE_INITIAL_PHASE (0xC0000000) and 0x40000000 -> 0, 0 -> INT32_MAX/2, 0x80000000 -> INT32_MIN/2
			int32_t vib_scale = vib_phase & 0x80000000 ? 0x40000000 + vib_phase : 0x3FFFFFFF - vib_phase;
			// select a vibrato pitch offset based on sign of scale; note that the values "init" and "scnd" values
			// produced by the decoder script will either both be negative, or both be positive; this allows the 
			// scalar to either start with either a downward (negative offset) or upward (positive) pitch oscillation
			int32_t vib_pitch_offset = vib_scale >= 0 ? vib_pitch_offset_init : vib_pitch_offset_scnd;
			// scale the offset and accumulate into offset
			// note the offset value is already preshifted by << 2 to account for this func shifting >> 32
			tone_incr_offset = multiply_accumulate_32x32_rshift32_rounded(tone_incr_offset, vib_scale, vib_pitch_offset);
		}

		// variable to hold an adjusted amplitude attenuation value; stays at tone_amp if modulation in delay
		int32_t mod_amp = tone_amp;
		if (mod_count++ > s->MODULATION_DELAY) {
			// pitch LFO component is same as above, but we'll also use the scale value for tremolo below
			mod_phase += s->MODULATION_INCREMENT;
			int32_t mod_scale = mod_phase & 0x80000000 ? 0x40000000 + mod_phase : 0x3FFFFFFF - mod_phase;

			int32_t mod_pitch_offset = mod_scale >= 0 ? mod_pitch_offset_init : mod_pitch_offset_scnd;
			tone_incr_offset = multiply_accumulate_32x32_rshift32_rounded(tone_incr_offset, mod_scale, mod_pitch_offset);

			// similar to pitch, sign of init and scnd are either both + or - to allow correct triangle direction
			int32_t mod_amp_offset = (mod_scale >= 0 ? s->MODULATION_AMPLITUDE_INITIAL_GAIN : s->MODULATION_AMPLITUDE_SECOND_GAIN);
			// here we scale the amp offset which, similar to the pitch offset, is already pre-shifted by << 2
			mod_scale = multiply_32x32_rshift32(mod_scale, mod_amp_offset);
			// the resulting scalar is then used to scale mod_map (possibly resulting in a negative) and add that back into mod_amp
			mod_amp = signed_multiply_accumulate_32x16b(mod_amp, mod_scale, mod_amp);
		}

		// producing 2 output values per iteration; repeat more depending on the LFO_SMOOTHNESS
		// this segment linearly interpolates, calculates how far we step through the sample data, and scales amplitude
		for (int i = LFO_PERIOD / 2; i; --i, ++p) {
			// INDEX_BITS representing the higher order bits we use to index into the sample data
			index = tone_phase >> (32 - s->INDEX_BITS);
			// recast as uint32_t to load in packed variable; initially int16_t* since we may need to read accross a word boundry
			// note we are assuming a little-endian cpu (i.e. the first sample is loaded into the lower half-word)
			tmp1 = *((uint32_t*)(s->sample + index));
			// phase_scale here being the next 16-bits after the first INDEX_BITS, representing the distince between the samples to interpolate at
			// 0x0000 gives us all of the first sample point, 0xFFFF all of the second, anything inbetween a sliding mix
			phase_scale = (tone_phase << s->INDEX_BITS) >> 16;
			// scaling of second sample point
			s1 = signed_multiply_32x16t(phase_scale, tmp1);
			// then add in scaling of first point
			s1 = signed_multiply_accumulate_32x16b(s1, 0xFFFF - phase_scale, tmp1);
			// apply amplitude scaling
			s1 = signed_multiply_32x16b(mod_amp, s1);

			// iterate tone_phase, giving us our desired frequency playback, and apply the offset, giving us our pitch LFOs
			tone_phase += tone_incr + tone_incr_offset;
			// break if no loop and we've gone past the end of the sample
			if (s->LOOP == false && tone_phase >= s->MAX_PHASE) break;
			// move phase back if a looped sample has overstepped its loop
			tone_phase = s->LOOP && tone_phase >= s->LOOP_PHASE_END ? tone_phase - s->LOOP_PHASE_LENGTH : tone_phase;

			//repeat as above
			index = tone_phase >> (32 - s->INDEX_BITS);
			tmp1 = *((uint32_t*)(s->sample + index));
			phase_scale = (tone_phase << s->INDEX_BITS) >> 16;
			s2 = signed_multiply_32x16t(phase_scale, tmp1);
			s2 = signed_multiply_accumulate_32x16b(s2, 0xFFFF - phase_scale, tmp1);
			s2 = signed_multiply_32x16b(mod_amp, s2);

			tone_phase += tone_incr + tone_incr_offset;
			if (s->LOOP == false && tone_phase >= s->MAX_PHASE) break;
			tone_phase = s->LOOP && tone_phase >= s->LOOP_PHASE_END ? tone_phase - s->LOOP_PHASE_LENGTH : tone_phase;

			// pack the two output samples into the audio_block
			*p = pack_16b_16b(s2, s1);
		}
	}
	// fill with 0s if non-looping sample that ended prematurely
	if (p < end) {
		env_state = STATE_IDLE;
		env_count = 0;
		while (p < end) *p++ = 0;
	}

	// filling audio_block two samples at a time
	p = (uint32_t *)block->data;
	end = p + AUDIO_BLOCK_SAMPLES / 2;

	// the following code handles the volume envelope with the following state transitions controlled here:
	// STATE_DELAY -> STATE_ATTACK -> STATE_HOLD -> STATE_DECAY -> STATE_SUSTAIN or STATE_IDLE
	// STATE_RELEASE -> STATE_IDLE
	// When STATE_SUSTAIN is reached, it is held indefinitely.
	// Outside of this code, playNote() and playFrequency() will initially set STATE_DELAY, and stop()
	// is responsible for setting STATE_RELEASE which can occur during any state, except STATE_IDLE

	// State defintions:
	// idle - not playing (generally should never arrive here)
	// delay - full attenuation
	// attack - linear ramp from full attenuation to no attenuation
	// hold - no attenuation
	// decay - linear ramp down to a given level of attenuation (SUSTAIN_MULT)
	// sustain - constant attenuation at a given level (SUSTAIN_MULT)
	// release - linear ramp down from current attenuation level to full attenuation
	
	// Definitions of the states generally follow the SoundFont spec, with a major exception being that all
	// volume scaling is linear realtive to amplitude; this is correct with respect to the attack, but not
	// the correct implementation relative to the decay and release which should be scaling linearly relative
	// to centibels. Practically this means the decay and release happen too slowing intially, and too quick
	// near the end

	// other points of note are that one env_count corresponds to 1 second * ENVELOPE_PERIOD / AUDIO_SAMPLE_RATE_EXACT;
	// the ENVELOPE_PERIOD is the number of samples processed per iteration of the following loop
	while (p < end) {
		// note env_count == 0 is used as a trigger for state transition
		if (env_count <= 0) switch (env_state) {
		case STATE_DELAY:
			env_state = STATE_ATTACK;
			env_count = s->ATTACK_COUNT;
			env_incr = UNITY_GAIN / (env_count * ENVELOPE_PERIOD);
			PRINT_ENV(STATE_ATTACK);
			continue;
		case STATE_ATTACK:
			env_mult = UNITY_GAIN;
			env_state = STATE_HOLD;
			env_count = s->HOLD_COUNT;
			env_incr = 0;
			PRINT_ENV(STATE_HOLD);
			continue;
		case STATE_HOLD:
			env_state = STATE_DECAY;
			env_count = s->DECAY_COUNT;
			env_incr = (-s->SUSTAIN_MULT) / (env_count * ENVELOPE_PERIOD);
			PRINT_ENV(STATE_DECAY);
			continue;
		case STATE_DECAY:
			env_mult = UNITY_GAIN - s->SUSTAIN_MULT;
			// UINT16_MAX is a value approximately corresponding to the -100 dBFS defined in the SoundFont spec as full attenuation
			// hence this comparison either sends the state to indefinite STATE_SUSTAIN, or immediately into STATE_RELEASE -> STATE_IDLE
			env_state = env_mult < UNITY_GAIN / UINT16_MAX ? STATE_RELEASE : STATE_SUSTAIN;
			env_incr = 0;
			continue;
		case STATE_SUSTAIN:
			env_count = INT32_MAX;
			PRINT_ENV(STATE_SUSTAIN);
			continue;
		case STATE_RELEASE:
			env_state = STATE_IDLE;
			for (; p < end; ++p) *p = 0;
			PRINT_ENV(STATE_IDLE);
			continue;
		default:
			p = end;
			PRINT_ENV(DEFAULT);
			continue;
		}

		env_mult += env_incr;
		// env_mult is INT32_MAX at max (i.e. 31-bits), so shift << 1 so result is aligned with high halfword of tmp1/tmp2
		tmp1 = signed_multiply_32x16b(env_mult, p[0]) << 1;
		env_mult += env_incr;
		tmp2 = signed_multiply_32x16t(env_mult, p[0]) << 1;
		// pack from high halfword of tmp1, tmp2
		p[0] = pack_16t_16t(tmp2, tmp1);
		env_mult += env_incr;
		tmp1 = signed_multiply_32x16b(env_mult, p[1]) << 1;
		env_mult += env_incr;
		tmp2 = signed_multiply_32x16t(env_mult, p[1]) << 1;
		p[1] = pack_16t_16t(tmp2, tmp1);
		env_mult += env_incr;
		tmp1 = signed_multiply_32x16b(env_mult, p[2]) << 1;
		env_mult += env_incr;
		tmp2 = signed_multiply_32x16t(env_mult, p[2]) << 1;
		p[2] = pack_16t_16t(tmp2, tmp1);
		env_mult += env_incr;
		tmp1 = signed_multiply_32x16b(env_mult, p[3]) << 1;
		env_mult += env_incr;
		tmp2 = signed_multiply_32x16t(env_mult, p[3]) << 1;
		p[3] = pack_16t_16t(tmp2, tmp1);

		p += ENVELOPE_PERIOD / 2;
		env_count--;
	}

	// copy state back, unless there was a state change
	if (this->state_change == false) {
		this->tone_phase = tone_phase;
		this->env_state = env_state;
		this->env_count = env_count;
		this->env_mult = env_mult;
		this->env_incr = env_incr;
		if (this->env_state != STATE_IDLE) {
			this->vib_count = vib_count;
			this->vib_phase = vib_phase;
			this->mod_count = mod_count;
			this->mod_phase = mod_phase;
		}
		else {
			this->vib_count = this->mod_count = 0;
			this->vib_phase = this->mod_phase = TRIANGLE_INITIAL_PHASE;
		}
	}

	transmit(block);
	release(block);
#endif
}
