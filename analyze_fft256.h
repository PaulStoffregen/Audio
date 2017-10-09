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

#ifndef analyze_fft256_h_
#define analyze_fft256_h_

#include "Arduino.h"
#include "AudioStream.h"
#include "arm_math.h"

// windows.c
extern "C" {
extern const int16_t AudioWindowHanning256[];
extern const int16_t AudioWindowBartlett256[];
extern const int16_t AudioWindowBlackman256[];
extern const int16_t AudioWindowFlattop256[];
extern const int16_t AudioWindowBlackmanHarris256[];
extern const int16_t AudioWindowNuttall256[];
extern const int16_t AudioWindowBlackmanNuttall256[];
extern const int16_t AudioWindowWelch256[];
extern const int16_t AudioWindowHamming256[];
extern const int16_t AudioWindowCosine256[];
extern const int16_t AudioWindowTukey256[];
}

class AudioAnalyzeFFT256 : public AudioStream
{
public:
	AudioAnalyzeFFT256() : AudioStream(1, inputQueueArray),
	  window(AudioWindowHanning256), count(0), outputflag(false) {
		arm_cfft_radix4_init_q15(&fft_inst, 256, 0, 1);
#if AUDIO_BLOCK_SAMPLES == 128
		prevblock = NULL;
		naverage = 8;
#elif AUDIO_BLOCK_SAMPLES == 64
		prevblocks[0] = NULL;
		prevblocks[1] = NULL;
		prevblocks[2] = NULL;
#endif
	}
	bool available() {
		if (outputflag == true) {
			outputflag = false;
			return true;
		}
		return false;
	}
	float read(unsigned int binNumber) {
		if (binNumber > 127) return 0.0;
		return (float)(output[binNumber]) * (1.0 / 16384.0);
	}
	float read(unsigned int binFirst, unsigned int binLast) {
		if (binFirst > binLast) {
			unsigned int tmp = binLast;
			binLast = binFirst;
			binFirst = tmp;
		}
		if (binFirst > 127) return 0.0;
		if (binLast > 127) binLast = 127;
		uint32_t sum = 0;
		do {
			sum += output[binFirst++];
		} while (binFirst <= binLast);
		return (float)sum * (1.0 / 16384.0);
	}
	void averageTogether(uint8_t n) {
#if AUDIO_BLOCK_SAMPLES == 128
		if (n == 0) n = 1;
		naverage = n;
#endif
	}
	void windowFunction(const int16_t *w) {
		window = w;
	}
	virtual void update(void);
	uint16_t output[128] __attribute__ ((aligned (4)));
private:
	const int16_t *window;
#if AUDIO_BLOCK_SAMPLES == 128
	audio_block_t *prevblock;
#elif AUDIO_BLOCK_SAMPLES == 64
	audio_block_t *prevblocks[3];
#endif
	int16_t buffer[512] __attribute__ ((aligned (4)));
#if AUDIO_BLOCK_SAMPLES == 128
	uint32_t sum[128];
	uint8_t naverage;
#endif
	uint8_t count;
	volatile bool outputflag;
	audio_block_t *inputQueueArray[1];
	arm_cfft_radix4_instance_q15 fft_inst;
};

#endif
