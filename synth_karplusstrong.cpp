/* Audio Library for Teensy 3.X
 * Copyright (c) 2016, Paul Stoffregen, paul@pjrc.com
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

#include "synth_karplusstrong.h"

static uint32_t pseudorand(uint32_t lo)
{
	uint32_t hi;

	hi = multiply_16bx16t(16807, lo); // 16807 * (lo >> 16)
	lo = 16807 * (lo & 0xFFFF);
	lo += (hi & 0x7FFF) << 16;
	lo += hi >> 15;
	lo = (lo & 0x7FFFFFFF) + (lo >> 31);
	return lo;
}


void AudioSynthKarplusStrong::update(void)
{
	audio_block_t *block;

	if (state == 0) return;

	if (state == 1) {
		uint32_t lo = seed;
		for (int i=0; i < bufferLen; i++) {
			lo = pseudorand(lo);
			buffer[i] = signed_multiply_32x16b(magnitude, lo);
		}
		seed = lo;
		state = 2;
	}

	block = allocate();
	if (!block) {
		state = 0;
		return;
	}

	int16_t prior;
	if (bufferIndex > 0) {
		prior = buffer[bufferIndex - 1];
	} else {
		prior = buffer[bufferLen - 1];
	}
	int16_t *data = block->data;
	for (int i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
		int16_t in = buffer[bufferIndex];
		//int16_t out = (in * 32604 + prior * 32604) >> 16;
		int16_t out = (in * 32686 + prior * 32686) >> 16;
		//int16_t out = (in * 32768 + prior * 32768) >> 16;
		*data++ = out;
		buffer[bufferIndex] = out;
		prior = in;
		if (++bufferIndex >= bufferLen) bufferIndex = 0;
	}

	transmit(block);
	release(block);
}


uint32_t AudioSynthKarplusStrong::seed = 1;

