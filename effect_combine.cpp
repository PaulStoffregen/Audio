/*
 * Copyright (c) 2018 John-Michael Reed
 * bleeplabs.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

//  Combine analog signals with bitwise expressions like XOR.
//  Combining two simple oscillators results in interesting new waveforms, 
//  Combining white noise or dynamic incoming audio results in aggressive digital distortion.
 
#include <Arduino.h>
#include "effect_combine.h"

void AudioEffectDigitalCombine::update(void)
{
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
	end = pa + AUDIO_BLOCK_SAMPLES / 2;

	while (pa < end) {
		a12 = *pa;
		a34 = *(pa + 1);
		b12 = *pb++;
		b34 = *pb++;
		if (mode_sel == OR) {
			a12 = a12 | b12;
			a34 = a34 | b34;
		}
		if (mode_sel == XOR) {
			a12 = a12 ^ b12;
			a34 = a34 ^ b34;
		}
		if (mode_sel == AND) {
			a12 = a12 & b12;
			a34 = a34 & b34;
		}
		if (mode_sel == MODULO) {
			a12 = a12 % b12;
			a34 = a34 % b34;
		}
		*pa++ = a12;
		*pa++ = a34;
	}
	transmit(blocka);
	release(blocka);
	release(blockb);
}

