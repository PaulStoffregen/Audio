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

#ifndef synth_pinknoise_h_
#define synth_pinknoise_h_
#include "Arduino.h"
#include "AudioStream.h"
#include "utility/dspinst.h"

class AudioSynthNoisePink : public AudioStream
{
public:
	AudioSynthNoisePink() : AudioStream(0, NULL) {
		plfsr  = 0x5EED41F5 + instance_cnt++;
		paccu  = 0;
		pncnt  = 0;
		pinc   = 0x0CCC;
		pdec   = 0x0CCC;
	}	
	void amplitude(float n) {
		if (n < 0.0) n = 0.0;
		else if (n > 1.0) n = 1.0;
		level = (int32_t)(n * 65536.0);
	}
	virtual void update(void);
private:
	static const uint8_t pnmask[256];
	static const int32_t pfira[64];
	static const int32_t pfirb[64];
	static int16_t instance_cnt;
	int32_t plfsr;		// linear feedback shift register
	int32_t pinc;		// increment for all noise sources (bits)
	int32_t pdec;		// decrement for all noise sources
	int32_t paccu;		// accumulator
	uint8_t pncnt;		// overflowing counter as index to pnmask[]
	int32_t level;		// 0=off, 65536=max
};

#endif
