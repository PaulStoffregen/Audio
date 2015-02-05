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

#include "effect_midside_encode.h"




void AudioEffectMidSideEncode::update(void) {
	audio_block_t *blocka, *blockb;

	uint32_t *pa, *pb, *end;
	uint32_t a12, a34; //, a56, a78;
	uint32_t b12, b34; //, b56, b78;

	blocka = receiveWritable(0); // left
	blockb = receiveWritable(1); // right

	if (!blocka || !blockb) {
		if (blocka) release(blocka); // maybe an extra if statement here but if one of the blocks is NULL then it's trouble anyway
		if (blockb) release(blockb);
		return;
	}

	pa = (uint32_t *)(blocka->data);
	pb = (uint32_t *)(blockb->data);
	end = pa + AUDIO_BLOCK_SAMPLES/2;
	
	while (pa < end) { // assert(AUDIO_BLOCK_SAMPLES % 4 == 0); // because 32bit, and loading 2x 32bit
		// mid[i] =  (blocka[i] + blockb[i])/2; // put the /2 here to prevent overflows
		// side[i] = (blocka[i] - blockb[i])/2; // put the /2 here to prevent overflows
		// L[i] = (mid[i] + side[i])/2; // 
		// R[i] = (mid[i] - side[i])/2; //
		
		a12 = signed_halving_add_16_and_16(*pa, *pb); // mid12
		a34 = signed_halving_add_16_and_16(*(pa+1), *(pb+1)); //mid34

		b12 = signed_halving_subtract_16_and_16(*pa, *pb); // side12
		b34 = signed_halving_subtract_16_and_16(*(pa+1), *(pb+1)); // side34
		
		*pa++ = a12;
		*pa++ = a34;
		
		*pb++ = b12;
		*pb++ = b34;
	}
	
	transmit(blocka, 0); // (0) mid
	transmit(blockb, 1); // (1) side
	
	release(blocka);
	release(blockb);
}

