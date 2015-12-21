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

#include "analyze_fft256.h"
#include "sqrt_integer.h"
#include "utility/dspinst.h"


// 140312 - PAH - slightly faster copy
static void copy_to_fft_buffer(void *destination, const void *source)
{
	const uint16_t *src = (const uint16_t *)source;
	uint32_t *dst = (uint32_t *)destination;

	for (int i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
		*dst++ = *src++;  // real sample plus a zero for imaginary
	}
}

static void apply_window_to_fft_buffer(void *buffer, const void *window)
{
	int16_t *buf = (int16_t *)buffer;
	const int16_t *win = (int16_t *)window;;

	for (int i=0; i < 256; i++) {
		int32_t val = *buf * *win++;
		//*buf = signed_saturate_rshift(val, 16, 15);
		*buf = val >> 15;
		buf += 2;
	}
}

void AudioAnalyzeFFT256::update(void)
{
	audio_block_t *block;

	block = receiveReadOnly();
	if (!block) return;
#if AUDIO_BLOCK_SAMPLES == 128
	if (!prevblock) {
		prevblock = block;
		return;
	}
	copy_to_fft_buffer(buffer, prevblock->data);
	copy_to_fft_buffer(buffer+256, block->data);
	//window = AudioWindowBlackmanNuttall256;
	//window = NULL;
	if (window) apply_window_to_fft_buffer(buffer, window);
	arm_cfft_radix4_q15(&fft_inst, buffer);
	// G. Heinzel's paper says we're supposed to average the magnitude
	// squared, then do the square root at the end.
	if (count == 0) {
		for (int i=0; i < 128; i++) {
			uint32_t tmp = *((uint32_t *)buffer + i);
			uint32_t magsq = multiply_16tx16t_add_16bx16b(tmp, tmp);
			sum[i] = magsq / naverage;
		}
	} else {
		for (int i=0; i < 128; i++) {
			uint32_t tmp = *((uint32_t *)buffer + i);
			uint32_t magsq = multiply_16tx16t_add_16bx16b(tmp, tmp);
			sum[i] += magsq / naverage;
		}
	}
	if (++count == naverage) {
		count = 0;
		for (int i=0; i < 128; i++) {
			output[i] = sqrt_uint32_approx(sum[i]);
		}
		outputflag = true;
	}
	release(prevblock);
	prevblock = block;
#elif AUDIO_BLOCK_SAMPLES == 64
	if (prevblocks[2] == NULL) {
		prevblocks[2] = prevblocks[1];
		prevblocks[1] = prevblocks[0];
		prevblocks[0] = block;
		return;
	}
	if (count == 0) {
		count = 1;
		copy_to_fft_buffer(buffer, prevblocks[2]->data);
		copy_to_fft_buffer(buffer+128, prevblocks[1]->data);
		copy_to_fft_buffer(buffer+256, prevblocks[1]->data);
		copy_to_fft_buffer(buffer+384, block->data);
		if (window) apply_window_to_fft_buffer(buffer, window);
		arm_cfft_radix4_q15(&fft_inst, buffer);
	} else {
		count = 2;
		const uint32_t *p = (uint32_t *)buffer;
		for (int i=0; i < 128; i++) {
			uint32_t tmp = *p++;
			int16_t v1 = tmp & 0xFFFF;
			int16_t v2 = tmp >> 16;
			output[i] = sqrt_uint32_approx(v1 * v1 + v2 * v2);
		}
	}
	release(prevblocks[2]);
	prevblocks[2] = prevblocks[1];
	prevblocks[1] = prevblocks[0];
	prevblocks[0] = block;
#endif
}


