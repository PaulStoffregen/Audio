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

#ifndef analyze_rms_h_
#define analyze_rms_h_

#include "AudioStream.h"

class AudioAnalyzeRMS : public AudioStream
{
private:
	audio_block_t *inputQueueArray[1];
	volatile bool new_output;
	int16_t lastRMS;

public:
	AudioAnalyzeRMS(void) : AudioStream(1, inputQueueArray) {
		lastRMS = 0;
	}

	bool available(void) {
		__disable_irq();
		bool flag = new_output; // we don't reset new_output here, because if you don't read it, 
		//it'll still be available on the next call of available() 
		// (different from AnalyzePeak behavior, which resets it in available())
		__enable_irq();
		return flag;
	}

	float read(void) {
		__disable_irq();
		int rms = lastRMS;
		new_output = false; // we can always set the new_output to false, even if it was false already
		__enable_irq();
		
		return rms / 32767.0;
	}

	virtual void update(void);
};

#endif

