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

void AudioFilterBiquad::update(void)
{
	audio_block_t *block;
	int32_t a0, a1, a2, b1, b2, sum;
	uint32_t in2, out2, aprev, bprev, flag;
	uint32_t *data, *end;
	int32_t *state;
	block = receiveWritable();
	if (!block) return;
	end = (uint32_t *)(block->data) + AUDIO_BLOCK_SAMPLES/2;
	state = (int32_t *)definition;
	do {
		a0 = *state++;
		a1 = *state++;
		a2 = *state++;
		b1 = *state++;
		b2 = *state++;
		aprev = *state++;
		bprev = *state++;
		sum = *state & 0x3FFF;
		data = end - AUDIO_BLOCK_SAMPLES/2;
		do {
			in2 = *data;
			sum = signed_multiply_accumulate_32x16b(sum, a0, in2);
			sum = signed_multiply_accumulate_32x16t(sum, a1, aprev);
			sum = signed_multiply_accumulate_32x16b(sum, a2, aprev);
			sum = signed_multiply_accumulate_32x16t(sum, b1, bprev);
			sum = signed_multiply_accumulate_32x16b(sum, b2, bprev);
			out2 = (uint32_t)sum >> 14;
			sum &= 0x3FFF;
			sum = signed_multiply_accumulate_32x16t(sum, a0, in2);
			sum = signed_multiply_accumulate_32x16b(sum, a1, in2);
			sum = signed_multiply_accumulate_32x16t(sum, a2, aprev);
			sum = signed_multiply_accumulate_32x16b(sum, b1, out2);
			sum = signed_multiply_accumulate_32x16t(sum, b2, bprev);
			aprev = in2;
			bprev = pack_16x16(sum >> 14, out2);
			sum &= 0x3FFF;
			aprev = in2;
			*data++ = bprev;
		} while (data < end);
		flag = *state & 0x80000000;
		*state++ = sum | flag;
		*(state-2) = bprev;
		*(state-3) = aprev;
	} while (flag);
	transmit(block);
	release(block);
}

void AudioFilterBiquad::updateCoefs(uint8_t set,int *source, bool doReset)
{
	int32_t *dest=(int32_t *)definition;
	while(set)
	{
		dest+=7;
		if(!(*dest)&0x80000000) return;
		dest++;
		set--;
	}
	__disable_irq();
	for(uint8_t index=0;index<5;index++)
	{
		*dest++=*source++;
	}
	if(doReset)
	{
		*dest++=0;
		*dest++=0;
		*dest++=0;
	}
	__enable_irq();
}

