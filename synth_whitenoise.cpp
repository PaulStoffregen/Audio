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

#include "synth_whitenoise.h"

// Park-Miller-Carta Pseudo-Random Number Generator
// http://www.firstpr.com.au/dsp/rand31/

void AudioSynthNoiseWhite::update(void)
{
	audio_block_t *block;
	uint32_t *p, *end;
	int32_t n1, n2, gain;
	uint32_t lo, hi, val1, val2;

	gain = level;
	if (gain == 0) return;
	block = allocate();
	if (!block) return;
	p = (uint32_t *)(block->data);
	end = p + AUDIO_BLOCK_SAMPLES/2;
	lo = seed;
	do {
#if defined(KINETISK) || defined(__SAMD51__)
		hi = multiply_16bx16t(16807, lo); // 16807 * (lo >> 16)
		lo = 16807 * (lo & 0xFFFF);
		lo += (hi & 0x7FFF) << 16;
		lo += hi >> 15;
		lo = (lo & 0x7FFFFFFF) + (lo >> 31);
		n1 = signed_multiply_32x16b(gain, lo);
		hi = multiply_16bx16t(16807, lo); // 16807 * (lo >> 16)
		lo = 16807 * (lo & 0xFFFF);
		lo += (hi & 0x7FFF) << 16;
		lo += hi >> 15;
		lo = (lo & 0x7FFFFFFF) + (lo >> 31);
		n2 = signed_multiply_32x16b(gain, lo);
		val1 = pack_16b_16b(n2, n1);
		hi = multiply_16bx16t(16807, lo); // 16807 * (lo >> 16)
		lo = 16807 * (lo & 0xFFFF);
		lo += (hi & 0x7FFF) << 16;
		lo += hi >> 15;
		lo = (lo & 0x7FFFFFFF) + (lo >> 31);
		n1 = signed_multiply_32x16b(gain, lo);
		hi = multiply_16bx16t(16807, lo); // 16807 * (lo >> 16)
		lo = 16807 * (lo & 0xFFFF);
		lo += (hi & 0x7FFF) << 16;
		lo += hi >> 15;
		lo = (lo & 0x7FFFFFFF) + (lo >> 31);
		n2 = signed_multiply_32x16b(gain, lo);
		val2 = pack_16b_16b(n2, n1);
		*p++ = val1;
		*p++ = val2;
#elif defined(KINETISL)
		hi = 16807 * (lo >> 16);
		lo = 16807 * (lo & 0xFFFF);
		lo += (hi & 0x7FFF) << 16;
		lo += hi >> 15;
		lo = (lo & 0x7FFFFFFF) + (lo >> 31);
		n1 = signed_multiply_32x16b(gain, lo);
		hi = 16807 * (lo >> 16);
		lo = 16807 * (lo & 0xFFFF);
		lo += (hi & 0x7FFF) << 16;
		lo += hi >> 15;
		lo = (lo & 0x7FFFFFFF) + (lo >> 31);
		n2 = signed_multiply_32x16b(gain, lo);
		val1 = pack_16b_16b(n2, n1);
		*p++ = val1;
#endif
	} while (p < end);
	seed = lo;
	transmit(block);
	release(block);
}

uint16_t AudioSynthNoiseWhite::instance_count = 0;


