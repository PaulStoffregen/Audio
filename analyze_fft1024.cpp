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

#include "analyze_fft1024.h"
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

	for (int i=0; i < 1024; i++) {
		int32_t val = *buf * *win++;
		//*buf = signed_saturate_rshift(val, 16, 15);
		*buf = val >> 15;
		buf += 2;
	}

}

void AudioAnalyzeFFT1024::update(void)
{
	audio_block_t *block;

	block = receiveReadOnly();
	if (!block) return;

#if defined(KINETISK) || defined(__SAMD51__)
	switch (state) {
	case 0:
		blocklist[0] = block;
		state = 1;
		break;
	case 1:
		blocklist[1] = block;
		state = 2;
		break;
	case 2:
		blocklist[2] = block;
		state = 3;
		break;
	case 3:
		blocklist[3] = block;
		state = 4;
		break;
	case 4:
		blocklist[4] = block;
		state = 5;
		break;
	case 5:
		blocklist[5] = block;
		state = 6;
		break;
	case 6:
		blocklist[6] = block;
		state = 7;
		break;
	case 7:
		blocklist[7] = block;
		// TODO: perhaps distribute the work over multiple update() ??
		//       github pull requsts welcome......
		copy_to_fft_buffer(buffer+0x000, blocklist[0]->data);
		copy_to_fft_buffer(buffer+0x100, blocklist[1]->data);
		copy_to_fft_buffer(buffer+0x200, blocklist[2]->data);
		copy_to_fft_buffer(buffer+0x300, blocklist[3]->data);
		copy_to_fft_buffer(buffer+0x400, blocklist[4]->data);
		copy_to_fft_buffer(buffer+0x500, blocklist[5]->data);
		copy_to_fft_buffer(buffer+0x600, blocklist[6]->data);
		copy_to_fft_buffer(buffer+0x700, blocklist[7]->data);
		if (window) apply_window_to_fft_buffer(buffer, window);
		arm_cfft_radix4_q15(&fft_inst, buffer);
		// TODO: support averaging multiple copies
		for (int i=0; i < 512; i++) {
			uint32_t tmp = *((uint32_t *)buffer + i); // real & imag
			uint32_t magsq = multiply_16tx16t_add_16bx16b(tmp, tmp);
			output[i] = sqrt_uint32_approx(magsq);
		}
		outputflag = true;
		release(blocklist[0]);
		release(blocklist[1]);
		release(blocklist[2]);
		release(blocklist[3]);
		blocklist[0] = blocklist[4];
		blocklist[1] = blocklist[5];
		blocklist[2] = blocklist[6];
		blocklist[3] = blocklist[7];
		state = 4;
		break;
	}
#else
	release(block);
#endif
}


