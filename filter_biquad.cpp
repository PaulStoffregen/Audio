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

#include "filter_biquad.h"
#include "utility/dspinst.h"

#if defined(KINETISK) || defined(__SAMD51__)

void AudioFilterBiquad::update(void)
{
	audio_block_t *block;
	int32_t b0, b1, b2, a1, a2, sum;
	uint32_t in2, out2, bprev, aprev, flag;
	uint32_t *data, *end;
	int32_t *state;
	block = receiveWritable();
	if (!block) return;
	end = (uint32_t *)(block->data) + AUDIO_BLOCK_SAMPLES/2;
	state = (int32_t *)definition;
	do {
		b0 = *state++;
		b1 = *state++;
		b2 = *state++;
		a1 = *state++;
		a2 = *state++;
		bprev = *state++;
		aprev = *state++;
		sum = *state & 0x3FFF;
		data = end - AUDIO_BLOCK_SAMPLES/2;
		do {
			in2 = *data;
			sum = signed_multiply_accumulate_32x16b(sum, b0, in2);
			sum = signed_multiply_accumulate_32x16t(sum, b1, bprev);
			sum = signed_multiply_accumulate_32x16b(sum, b2, bprev);
			sum = signed_multiply_accumulate_32x16t(sum, a1, aprev);
			sum = signed_multiply_accumulate_32x16b(sum, a2, aprev);
			out2 = signed_saturate_rshift(sum, 16, 14);
			sum &= 0x3FFF;
			sum = signed_multiply_accumulate_32x16t(sum, b0, in2);
			sum = signed_multiply_accumulate_32x16b(sum, b1, in2);
			sum = signed_multiply_accumulate_32x16t(sum, b2, bprev);
			sum = signed_multiply_accumulate_32x16b(sum, a1, out2);
			sum = signed_multiply_accumulate_32x16t(sum, a2, aprev);
			bprev = in2;
			aprev = pack_16b_16b(
				signed_saturate_rshift(sum, 16, 14), out2);
			sum &= 0x3FFF;
			bprev = in2;
			*data++ = aprev;
		} while (data < end);
		flag = *state & 0x80000000;
		*state++ = sum | flag;
		*(state-2) = aprev;
		*(state-3) = bprev;
	} while (flag);
	transmit(block);
	release(block);
}

void AudioFilterBiquad::setCoefficients(uint32_t stage, const int *coefficients)
{
	if (stage >= 4) return;
	int32_t *dest = definition + (stage << 3);
	__disable_irq();
	if (stage > 0) *(dest - 1) |= 0x80000000;
	*dest++ = *coefficients++;
	*dest++ = *coefficients++;
	*dest++ = *coefficients++;
	*dest++ = *coefficients++ * -1;
	*dest++ = *coefficients++ * -1;
	//*dest++ = 0;
	//*dest++ = 0;  // clearing filter state causes loud pop
	dest += 2;
	*dest   &= 0x80000000;
	__enable_irq();
}

#elif defined(KINETISL)

void AudioFilterBiquad::update(void)
{
        audio_block_t *block;

	block = receiveReadOnly();
	if (block) release(block);
}

#endif
