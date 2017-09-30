/* Audio Library for Teensy 3.X
 * Copyright (c) 2017, Paul Stoffregen, paul@pjrc.com
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

#ifndef synth_pwm_h_
#define synth_pwm_h_

#include "Arduino.h"
#include "AudioStream.h"
#include "arm_math.h"

class AudioSynthWaveformPWM : public AudioStream
{
public:
	AudioSynthWaveformPWM() : AudioStream(1, inputQueueArray), magnitude(0), elapsed(0) {}
	void frequency(float freq) {
		if (freq < 1.0) freq = 1.0;
		else if (freq > AUDIO_SAMPLE_RATE_EXACT/4) freq = AUDIO_SAMPLE_RATE_EXACT/4;
		//phase_increment = freq * (4294967296.0 / AUDIO_SAMPLE_RATE_EXACT);
		duration = (AUDIO_SAMPLE_RATE_EXACT * 65536.0 + freq) / (freq * 2.0);
	}
	void amplitude(float n) {
		if (n < 0) n = 0;
		else if (n > 1.0) n = 1.0;
		magnitude = n * 32767.0;
	}
	virtual void update(void);
private:
	uint32_t duration; // samples per half cycle (when 50% duty) * 65536
	audio_block_t *inputQueueArray[1];
	int32_t magnitude;
	uint32_t elapsed;
};


#endif
