/* Audio Library for Teensy 3.X
 * Copyright (c) 2015, Hedde Bosman
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

#include "effect_midside.h"

void AudioEffectMidSide::update(void)
{
	audio_block_t *blocka, *blockb;

	uint32_t *pa, *pb, *end;
	uint32_t a12, a34; //, a56, a78;
	uint32_t b12, b34; //, b56, b78;

	blocka = receiveWritable(0); // left (encoding) or  mid (decoding)
	blockb = receiveWritable(1); // right (encoding) or side (decoding)
	if (!blocka || !blockb) {
		if (blocka) release(blocka); // maybe an extra if statement here but if one
		if (blockb) release(blockb); // of the blocks is NULL then it's trouble anyway
		return;
	}
#if defined(KINETISK) || defined(__SAMD51__)
	pa = (uint32_t *)(blocka->data);
	pb = (uint32_t *)(blockb->data);
	end = pa + AUDIO_BLOCK_SAMPLES/2;

	if (encoding) {
		while (pa < end) {
			// mid[i] =  (blocka[i] + blockb[i])/2; // div2 to prevent overflows
			// side[i] = (blocka[i] - blockb[i])/2; // div2 to prevent overflows
			// L[i] = (mid[i] + side[i])/2;
			// R[i] = (mid[i] - side[i])/2;
			a12 = signed_halving_add_16_and_16(*pa, *pb); // mid12
			a34 = signed_halving_add_16_and_16(*(pa+1), *(pb+1)); //mid34
			b12 = signed_halving_subtract_16_and_16(*pa, *pb); // side12
			b34 = signed_halving_subtract_16_and_16(*(pa+1), *(pb+1)); // side34
			*pa++ = a12;
			*pa++ = a34;
			*pb++ = b12;
			*pb++ = b34;
		}
	} else {
		while (pa < end) {
			// L[i] = mid[i] + side[i]);
			// R[i] = mid[i] - side[i]);
			// Because the /2 has already been applied in the encoding,
			// we shouldn't have to add it here.
			// However... because of the posibility that the user has
			// modified mid or side such that
			// it could overflow, we have to:
			// a) preventively do a (x/2+y/2)*2 again, causing bit reduction
			// or b) perform saturation or something.
			// While (b) could produce artifacts if saturated, I'll go for
			// that option to preserve precision
			// QADD16 and QSUB16 perform saturating add/sub
			a12 = signed_add_16_and_16(*pa, *pb); // left12
			a34 = signed_add_16_and_16(*(pa+1), *(pb+1)); // left34
			b12 = signed_subtract_16_and_16(*pa, *pb); // right12
			b34 = signed_subtract_16_and_16(*(pa+1), *(pb+1)); // right34
			*pa++ = a12;
			*pa++ = a34;
			*pb++ = b12;
			*pb++ = b34;
		}
	}
	transmit(blocka, 0); // mid (encoding) or  left (decoding)
	transmit(blockb, 1); // side (encoding) or right (decoding)
#endif
	release(blocka);
	release(blockb);
}
