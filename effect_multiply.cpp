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

#include "effect_multiply.h"

void AudioEffectMultiply::update(void)
{
#if defined(KINETISK) || defined(__SAMD51__)
	audio_block_t *blocka, *blockb;
	uint32_t *pa, *pb, *end;
	uint32_t a12, a34; //, a56, a78;
	uint32_t b12, b34; //, b56, b78;

	blocka = receiveWritable(0);
	blockb = receiveReadOnly(1);
	if (!blocka) {
		if (blockb) release(blockb);
		return;
	}
	if (!blockb) {
		release(blocka);
		return;
	}
	pa = (uint32_t *)(blocka->data);
	pb = (uint32_t *)(blockb->data);
	end = pa + AUDIO_BLOCK_SAMPLES/2;
	while (pa < end) {
		a12 = *pa;
		a34 = *(pa+1);
		//a56 = *(pa+2); // 8 samples/loop should work, but crashes.
		//a78 = *(pa+3); // why?!  maybe a compiler bug??
		b12 = *pb++;
		b34 = *pb++;
		//b56 = *pb++;
		//b78 = *pb++;
		a12 = pack_16b_16b(
			signed_saturate_rshift(multiply_16tx16t(a12, b12), 16, 15), 
			signed_saturate_rshift(multiply_16bx16b(a12, b12), 16, 15));
		a34 = pack_16b_16b(
			signed_saturate_rshift(multiply_16tx16t(a34, b34), 16, 15), 
			signed_saturate_rshift(multiply_16bx16b(a34, b34), 16, 15));
		//a56 = pack_16b_16b(
		//	signed_saturate_rshift(multiply_16tx16t(a56, b56), 16, 15), 
		//	signed_saturate_rshift(multiply_16bx16b(a56, b56), 16, 15));
		//a78 = pack_16b_16b(
		//	signed_saturate_rshift(multiply_16tx16t(a78, b78), 16, 15), 
		//	signed_saturate_rshift(multiply_16bx16b(a78, b78), 16, 15));
		*pa++ = a12;
		*pa++ = a34;
		//*pa++ = a56;
		//*pa++ = a78;
	}
	transmit(blocka);
	release(blocka);
	release(blockb);

#elif defined(KINETISL)
	audio_block_t *block;

	block = receiveReadOnly(0);
	if (block) release(block);
	block = receiveReadOnly(1);
	if (block) release(block);
#endif
}

