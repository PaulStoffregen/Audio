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

#ifndef effect_fade_h_
#define effect_fade_h_

#include "Arduino.h"
#include "AudioStream.h"

class AudioEffectFade : public AudioStream
{
public:
	AudioEffectFade(void)
	  : AudioStream(1, inputQueueArray), position(0xFFFFFFFF) {}
	void fadeIn(uint32_t milliseconds) {
		uint32_t samples = (uint32_t)(milliseconds * 441u + 5u) / 10u;
		//Serial.printf("fadeIn, %u samples\n", samples);
		fadeBegin(0xFFFFFFFFu / samples, 1);
	}
	void fadeOut(uint32_t milliseconds) {
		uint32_t samples = (uint32_t)(milliseconds * 441u + 5u) / 10u;
		//Serial.printf("fadeOut, %u samples\n", samples);
		fadeBegin(0xFFFFFFFFu / samples, 0);
	}
	virtual void update(void);
private:
	void fadeBegin(uint32_t newrate, uint8_t dir);
	uint32_t position; // 0 = off, 0xFFFFFFFF = on
	uint32_t rate;
	uint8_t direction; // 0 = fading out, 1 = fading in
	audio_block_t *inputQueueArray[1];
};

#endif
