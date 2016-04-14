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

#ifndef analyze_peakdetect_h_
#define analyze_peakdetect_h_

#include "Arduino.h"
#include "AudioStream.h"

class AudioAnalyzePeak : public AudioStream
{
public:
	AudioAnalyzePeak(void) : AudioStream(1, inputQueueArray) {
		min_sample = 32767;
		max_sample = -32768;
	}
	bool available(void) {
		__disable_irq();
		bool flag = new_output;
		if (flag) new_output = false;
		__enable_irq();
		return flag;
	}
	float read(void) {
		__disable_irq();
		int min = min_sample;
		int max = max_sample;
		min_sample = 32767;
		max_sample = -32768;
		__enable_irq();
		min = abs(min);
		max = abs(max);
		if (min > max) max = min;
		return (float)max / 32767.0f;
	}
	float readPeakToPeak(void) {
		__disable_irq();
		int min = min_sample;
		int max = max_sample;
		min_sample = 32767;
		max_sample = -32768;
		__enable_irq();
		return (float)(max - min) / 32767.0f;
	}

	virtual void update(void);
private:
	audio_block_t *inputQueueArray[1];
	volatile bool new_output;
	int16_t min_sample;
	int16_t max_sample;
};

#endif
