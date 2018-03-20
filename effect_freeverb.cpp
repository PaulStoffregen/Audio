/* Audio Library for Teensy 3.X
 * Copyright (c) 2018, Paul Stoffregen, paul@pjrc.com
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

#include <Arduino.h>
#include "effect_freeverb.h"
#include "utility/dspinst.h"

AudioEffectFreeverb::AudioEffectFreeverb() : AudioStream(1, inputQueueArray)
{
	memset(comb1buf, 0, sizeof(comb1buf));
	memset(comb2buf, 0, sizeof(comb2buf));
	memset(comb3buf, 0, sizeof(comb3buf));
	memset(comb4buf, 0, sizeof(comb4buf));
	memset(comb5buf, 0, sizeof(comb5buf));
	memset(comb6buf, 0, sizeof(comb6buf));
	memset(comb7buf, 0, sizeof(comb7buf));
	memset(comb8buf, 0, sizeof(comb8buf));
	comb1index = 0;
	comb2index = 0;
	comb3index = 0;
	comb4index = 0;
	comb5index = 0;
	comb6index = 0;
	comb7index = 0;
	comb8index = 0;
	comb1filter = 0;
	comb2filter = 0;
	comb3filter = 0;
	comb4filter = 0;
	comb5filter = 0;
	comb6filter = 0;
	comb7filter = 0;
	comb8filter = 0;
	combdamp1 = 6553;
	combdamp2 = 26215;
	combfeeback = 27524;
	memset(allpass1buf, 0, sizeof(allpass1buf));
	memset(allpass2buf, 0, sizeof(allpass2buf));
	memset(allpass3buf, 0, sizeof(allpass3buf));
	memset(allpass4buf, 0, sizeof(allpass4buf));
	allpass1index = 0;
	allpass2index = 0;
	allpass3index = 0;
	allpass4index = 0;
}

#if 1
#define sat16(n, rshift) signed_saturate_rshift((n), 16, (rshift))
#else
static int16_t sat16(int32_t n, int rshift)
{
	n = n >> rshift;
	if (n > 32767) {
		return 32767;
	}
	if (n < -32768) {
		return -32768;
	}
	return n;
}
#endif

void AudioEffectFreeverb::update()
{
#if defined(KINETISK)
	audio_block_t *block, *outblock;
	int i;
	int16_t input, bufout, output;
	int32_t sum;

	outblock = allocate();
	if (!outblock) {
		block = receiveReadOnly(0);
		if (block) release(block);
		return;
	}
	block = receiveReadOnly(0);
	if (!block) {
		release(outblock);
		return;
		// TODO: pointer to zero block
	}
	for (i=0; i < 128; i++) {
		input = sat16(block->data[i] * 21845, 17); // div by 6, for numerical headroom
		sum = 0;

		bufout = comb1buf[comb1index];
		sum += bufout;
		comb1filter = sat16(bufout * combdamp2 + comb1filter * combdamp1, 15);
		comb1buf[comb1index] = sat16(input + sat16(comb1filter * combfeeback, 15), 0);
		if (++comb1index >= sizeof(comb1buf)/sizeof(int16_t)) comb1index = 0;

		bufout = comb2buf[comb2index];
		sum += bufout;
		comb2filter = sat16(bufout * combdamp2 + comb2filter * combdamp1, 15);
		comb2buf[comb2index] = sat16(input + sat16(comb2filter * combfeeback, 15), 0);
		if (++comb2index >= sizeof(comb2buf)/sizeof(int16_t)) comb2index = 0;

		bufout = comb3buf[comb3index];
		sum += bufout;
		comb3filter = sat16(bufout * combdamp2 + comb3filter * combdamp1, 15);
		comb3buf[comb3index] = sat16(input + sat16(comb3filter * combfeeback, 15), 0);
		if (++comb3index >= sizeof(comb3buf)/sizeof(int16_t)) comb3index = 0;

		bufout = comb4buf[comb4index];
		sum += bufout;
		comb4filter = sat16(bufout * combdamp2 + comb4filter * combdamp1, 15);
		comb4buf[comb4index] = sat16(input + sat16(comb4filter * combfeeback, 15), 0);
		if (++comb4index >= sizeof(comb4buf)/sizeof(int16_t)) comb4index = 0;

		bufout = comb5buf[comb5index];
		sum += bufout;
		comb5filter = sat16(bufout * combdamp2 + comb5filter * combdamp1, 15);
		comb5buf[comb5index] = sat16(input + sat16(comb5filter * combfeeback, 15), 0);
		if (++comb5index >= sizeof(comb5buf)/sizeof(int16_t)) comb5index = 0;

		bufout = comb6buf[comb6index];
		sum += bufout;
		comb6filter = sat16(bufout * combdamp2 + comb6filter * combdamp1, 15);
		comb6buf[comb6index] = sat16(input + sat16(comb6filter * combfeeback, 15), 0);
		if (++comb6index >= sizeof(comb6buf)/sizeof(int16_t)) comb6index = 0;

		bufout = comb7buf[comb7index];
		sum += bufout;
		comb7filter = sat16(bufout * combdamp2 + comb7filter * combdamp1, 15);
		comb7buf[comb7index] = sat16(input + sat16(comb7filter * combfeeback, 15), 0);
		if (++comb7index >= sizeof(comb7buf)/sizeof(int16_t)) comb7index = 0;

		bufout = comb8buf[comb8index];
		sum += bufout;
		comb8filter = sat16(bufout * combdamp2 + comb8filter * combdamp1, 15);
		comb8buf[comb8index] = sat16(input + sat16(comb8filter * combfeeback, 15), 0);
		if (++comb8index >= sizeof(comb8buf)/sizeof(int16_t)) comb8index = 0;

		output = sat16(sum * 31457, 17);

		bufout = allpass1buf[allpass1index];
		allpass1buf[allpass1index] = output + (bufout >> 1);
		output = sat16(bufout - output, 1);
		if (++allpass1index >= sizeof(allpass1buf)/sizeof(int16_t)) allpass1index = 0;

		bufout = allpass2buf[allpass2index];
		allpass2buf[allpass2index] = output + (bufout >> 1);
		output = sat16(bufout - output, 1);
		if (++allpass2index >= sizeof(allpass2buf)/sizeof(int16_t)) allpass2index = 0;

		bufout = allpass3buf[allpass3index];
		allpass3buf[allpass3index] = output + (bufout >> 1);
		output = sat16(bufout - output, 1);
		if (++allpass3index >= sizeof(allpass3buf)/sizeof(int16_t)) allpass3index = 0;

		bufout = allpass4buf[allpass4index];
		allpass4buf[allpass4index] = output + (bufout >> 1);
		output = sat16(bufout - output, 1);
		if (++allpass4index >= sizeof(allpass4buf)/sizeof(int16_t)) allpass4index = 0;

		outblock->data[i] = sat16(output * 12, 0);
	}
	transmit(outblock);
	release(outblock);
	release(block);

#elif defined(KINETISL)
	audio_block_t *block;
	block = receiveReadOnly(0);
	if (block) release(block);
#endif
}

