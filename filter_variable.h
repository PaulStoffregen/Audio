/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
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

#ifndef filter_variable_h_
#define filter_variable_h_

#include <Arduino.h>     // github.com/PaulStoffregen/cores/blob/master/teensy4/Arduino.h
#include <AudioStream.h> // github.com/PaulStoffregen/cores/blob/master/teensy4/AudioStream.h

class AudioFilterStateVariable: public AudioStream
{
public:
	AudioFilterStateVariable() : AudioStream(2, inputQueueArray) {
		frequency(1000);
		octaveControl(1.0); // default values
		resonance(0.707);
		state_inputprev = 0;
		state_lowpass = 0;
		state_bandpass = 0;
	}
	void frequency(float freq) {
		if (freq < 20.0f) freq = 20.0f;
		else if (freq > AUDIO_SAMPLE_RATE_EXACT/2.5f) freq = AUDIO_SAMPLE_RATE_EXACT/2.5f;
		setting_fcenter = (freq * (3.141592654f/(AUDIO_SAMPLE_RATE_EXACT*2.0f)))
			* 2147483647.0f;
		// TODO: should we use an approximation when freq is not a const,
		// so the sinf() function isn't linked?
		setting_fmult = sinf(freq * (3.141592654f/(AUDIO_SAMPLE_RATE_EXACT*2.0f)))
			* 2147483647.0f;
	}
	void resonance(float q) {
		if (q < 0.7f) q = 0.7f;
		else if (q > 5.0f) q = 5.0f;
		// TODO: allow lower Q when frequency is lower
		setting_damp = (1.0f / q) * 1073741824.0f;
	}
	void octaveControl(float n) {
		// filter's corner frequency is Fcenter * 2^(control * N)
		// where "control" ranges from -1.0 to +1.0
		// and "N" allows the frequency to change from 0 to 7 octaves
		if (n < 0.0f) n = 0.0f;
		else if (n > 6.9999f) n = 6.9999f;
		setting_octavemult = n * 4096.0f;
	}
	virtual void update(void);
private:
	void update_fixed(const int16_t *in,
		int16_t *lp, int16_t *bp, int16_t *hp);
	void update_variable(const int16_t *in, const int16_t *ctl,
		int16_t *lp, int16_t *bp, int16_t *hp);
	int32_t setting_fcenter;
	int32_t setting_fmult;
	int32_t setting_octavemult;
	int32_t setting_damp;
	int32_t state_inputprev;
	int32_t state_lowpass;
	int32_t state_bandpass;
	audio_block_t *inputQueueArray[2];
};

#endif
