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

#ifndef analyze_tonedetect_h_
#define analyze_tonedetect_h_

#include "Arduino.h"
#include "AudioStream.h"

class AudioAnalyzeToneDetect : public AudioStream
{
public:
	AudioAnalyzeToneDetect(void)
	  : AudioStream(1, inputQueueArray), thresh(6554), enabled(false) { }
	void frequency(float freq, uint16_t cycles=10) {
		set_params((int32_t)(cos((double)freq
		  * (2.0 * 3.14159265358979323846 / AUDIO_SAMPLE_RATE_EXACT))
		  * (double)2147483647.999), cycles,
		  (float)AUDIO_SAMPLE_RATE_EXACT / freq * (float)cycles + 0.5f);
	}
	void set_params(int32_t coef, uint16_t cycles, uint16_t len);
	bool available(void) {
		__disable_irq();
		bool flag = new_output;
		if (flag) new_output = false;
		__enable_irq();
		return flag;
	}
	float read(void);
	void threshold(float level) {
		if (level < 0.01f) thresh = 655;
		else if (level > 0.99f) thresh = 64881;
		else thresh = level * 65536.0f + 0.5f;
	}
	operator bool();  // true if at or above threshold, false if below
	virtual void update(void);
private:
	int32_t coefficient;	// Goertzel algorithm coefficient
	int32_t s1, s2;		// Goertzel algorithm state
	int32_t out1, out2;	// Goertzel algorithm state output
	uint16_t length;	// number of samples to analyze
	uint16_t count;		// how many left to analyze
	uint16_t ncycles;	// number of waveform cycles to seek
	uint16_t thresh;	// threshold, 655 to 64881 (1% to 99%)
	bool enabled;
	volatile bool new_output;
	audio_block_t *inputQueueArray[1];
};

#endif
