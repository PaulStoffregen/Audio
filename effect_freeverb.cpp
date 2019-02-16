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

// A fixed point implementation of Freeverb by Jezar at Dreampoint
//  http://blog.bjornroche.com/2012/06/freeverb-original-public-domain-code-by.html
//  https://music.columbia.edu/pipermail/music-dsp/2001-October/045433.html

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

// TODO: move this to one of the data files, use in output_adat.cpp, output_tdm.cpp, etc
static const audio_block_t zeroblock = {
0, 0, 0, {
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
#if AUDIO_BLOCK_SAMPLES > 16
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
#endif
#if AUDIO_BLOCK_SAMPLES > 32
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
#endif
#if AUDIO_BLOCK_SAMPLES > 48
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
#endif
#if AUDIO_BLOCK_SAMPLES > 64
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
#endif
#if AUDIO_BLOCK_SAMPLES > 80
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
#endif
#if AUDIO_BLOCK_SAMPLES > 96
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
#endif
#if AUDIO_BLOCK_SAMPLES > 112
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
#endif
} };

void AudioEffectFreeverb::update()
{
#if defined(__ARM_ARCH_7EM__)
	const audio_block_t *block;
	audio_block_t *outblock;
	int i;
	int16_t input, bufout, output;
	int32_t sum;

	outblock = allocate();
	if (!outblock) {
		audio_block_t *tmp = receiveReadOnly(0);
		if (tmp) release(tmp);
		return;
	}
	block = receiveReadOnly(0);
	if (!block) block = &zeroblock;

	for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
		// TODO: scale numerical range depending on roomsize & damping
		input = sat16(block->data[i] * 8738, 17); // for numerical headroom
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

		outblock->data[i] = sat16(output * 30, 0);
	}
	transmit(outblock);
	release(outblock);
	if (block != &zeroblock) release((audio_block_t *)block);

#elif defined(KINETISL)
	audio_block_t *block;
	block = receiveReadOnly(0);
	if (block) release(block);
#endif
}


AudioEffectFreeverbStereo::AudioEffectFreeverbStereo() : AudioStream(1, inputQueueArray)
{
	memset(comb1bufL, 0, sizeof(comb1bufL));
	memset(comb2bufL, 0, sizeof(comb2bufL));
	memset(comb3bufL, 0, sizeof(comb3bufL));
	memset(comb4bufL, 0, sizeof(comb4bufL));
	memset(comb5bufL, 0, sizeof(comb5bufL));
	memset(comb6bufL, 0, sizeof(comb6bufL));
	memset(comb7bufL, 0, sizeof(comb7bufL));
	memset(comb8bufL, 0, sizeof(comb8bufL));
	comb1indexL = 0;
	comb2indexL = 0;
	comb3indexL = 0;
	comb4indexL = 0;
	comb5indexL = 0;
	comb6indexL = 0;
	comb7indexL = 0;
	comb8indexL = 0;
	comb1filterL = 0;
	comb2filterL = 0;
	comb3filterL = 0;
	comb4filterL = 0;
	comb5filterL = 0;
	comb6filterL = 0;
	comb7filterL = 0;
	comb8filterL = 0;
	memset(comb1bufR, 0, sizeof(comb1bufR));
	memset(comb2bufR, 0, sizeof(comb2bufR));
	memset(comb3bufR, 0, sizeof(comb3bufR));
	memset(comb4bufR, 0, sizeof(comb4bufR));
	memset(comb5bufR, 0, sizeof(comb5bufR));
	memset(comb6bufR, 0, sizeof(comb6bufR));
	memset(comb7bufR, 0, sizeof(comb7bufR));
	memset(comb8bufR, 0, sizeof(comb8bufR));
	comb1indexR = 0;
	comb2indexR = 0;
	comb3indexR = 0;
	comb4indexR = 0;
	comb5indexR = 0;
	comb6indexR = 0;
	comb7indexR = 0;
	comb8indexR = 0;
	comb1filterR = 0;
	comb2filterR = 0;
	comb3filterR = 0;
	comb4filterR = 0;
	comb5filterR = 0;
	comb6filterR = 0;
	comb7filterR = 0;
	comb8filterR = 0;
	combdamp1 = 6553;
	combdamp2 = 26215;
	combfeeback = 27524;
	memset(allpass1bufL, 0, sizeof(allpass1bufL));
	memset(allpass2bufL, 0, sizeof(allpass2bufL));
	memset(allpass3bufL, 0, sizeof(allpass3bufL));
	memset(allpass4bufL, 0, sizeof(allpass4bufL));
	allpass1indexL = 0;
	allpass2indexL = 0;
	allpass3indexL = 0;
	allpass4indexL = 0;
	memset(allpass1bufR, 0, sizeof(allpass1bufR));
	memset(allpass2bufR, 0, sizeof(allpass2bufR));
	memset(allpass3bufR, 0, sizeof(allpass3bufR));
	memset(allpass4bufR, 0, sizeof(allpass4bufR));
	allpass1indexR = 0;
	allpass2indexR = 0;
	allpass3indexR = 0;
	allpass4indexR = 0;
}

void AudioEffectFreeverbStereo::update()
{
#if defined(__ARM_ARCH_7EM__)
	const audio_block_t *block;
	audio_block_t *outblockL;
	audio_block_t *outblockR;
	int i;
	int16_t input, bufout, outputL, outputR;
	int32_t sum;

	block = receiveReadOnly(0);
	outblockL = allocate();
	outblockR = allocate();
	if (!outblockL || !outblockR) {
		if (outblockL) release(outblockL);
		if (outblockR) release(outblockR);
		if (block) release((audio_block_t *)block);
		return;
	}
	if (!block) block = &zeroblock;

	for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
		// TODO: scale numerical range depending on roomsize & damping
		input = sat16(block->data[i] * 8738, 17); // for numerical headroom
		sum = 0;

		bufout = comb1bufL[comb1indexL];
		sum += bufout;
		comb1filterL = sat16(bufout * combdamp2 + comb1filterL * combdamp1, 15);
		comb1bufL[comb1indexL] = sat16(input + sat16(comb1filterL * combfeeback, 15), 0);
		if (++comb1indexL >= sizeof(comb1bufL)/sizeof(int16_t)) comb1indexL = 0;

		bufout = comb2bufL[comb2indexL];
		sum += bufout;
		comb2filterL = sat16(bufout * combdamp2 + comb2filterL * combdamp1, 15);
		comb2bufL[comb2indexL] = sat16(input + sat16(comb2filterL * combfeeback, 15), 0);
		if (++comb2indexL >= sizeof(comb2bufL)/sizeof(int16_t)) comb2indexL = 0;

		bufout = comb3bufL[comb3indexL];
		sum += bufout;
		comb3filterL = sat16(bufout * combdamp2 + comb3filterL * combdamp1, 15);
		comb3bufL[comb3indexL] = sat16(input + sat16(comb3filterL * combfeeback, 15), 0);
		if (++comb3indexL >= sizeof(comb3bufL)/sizeof(int16_t)) comb3indexL = 0;

		bufout = comb4bufL[comb4indexL];
		sum += bufout;
		comb4filterL = sat16(bufout * combdamp2 + comb4filterL * combdamp1, 15);
		comb4bufL[comb4indexL] = sat16(input + sat16(comb4filterL * combfeeback, 15), 0);
		if (++comb4indexL >= sizeof(comb4bufL)/sizeof(int16_t)) comb4indexL = 0;

		bufout = comb5bufL[comb5indexL];
		sum += bufout;
		comb5filterL = sat16(bufout * combdamp2 + comb5filterL * combdamp1, 15);
		comb5bufL[comb5indexL] = sat16(input + sat16(comb5filterL * combfeeback, 15), 0);
		if (++comb5indexL >= sizeof(comb5bufL)/sizeof(int16_t)) comb5indexL = 0;

		bufout = comb6bufL[comb6indexL];
		sum += bufout;
		comb6filterL = sat16(bufout * combdamp2 + comb6filterL * combdamp1, 15);
		comb6bufL[comb6indexL] = sat16(input + sat16(comb6filterL * combfeeback, 15), 0);
		if (++comb6indexL >= sizeof(comb6bufL)/sizeof(int16_t)) comb6indexL = 0;

		bufout = comb7bufL[comb7indexL];
		sum += bufout;
		comb7filterL = sat16(bufout * combdamp2 + comb7filterL * combdamp1, 15);
		comb7bufL[comb7indexL] = sat16(input + sat16(comb7filterL * combfeeback, 15), 0);
		if (++comb7indexL >= sizeof(comb7bufL)/sizeof(int16_t)) comb7indexL = 0;

		bufout = comb8bufL[comb8indexL];
		sum += bufout;
		comb8filterL = sat16(bufout * combdamp2 + comb8filterL * combdamp1, 15);
		comb8bufL[comb8indexL] = sat16(input + sat16(comb8filterL * combfeeback, 15), 0);
		if (++comb8indexL >= sizeof(comb8bufL)/sizeof(int16_t)) comb8indexL = 0;

		outputL = sat16(sum * 31457, 17);
		sum = 0;

		bufout = comb1bufR[comb1indexR];
		sum += bufout;
		comb1filterR = sat16(bufout * combdamp2 + comb1filterR * combdamp1, 15);
		comb1bufR[comb1indexR] = sat16(input + sat16(comb1filterR * combfeeback, 15), 0);
		if (++comb1indexR >= sizeof(comb1bufR)/sizeof(int16_t)) comb1indexR = 0;

		bufout = comb2bufR[comb2indexR];
		sum += bufout;
		comb2filterR = sat16(bufout * combdamp2 + comb2filterR * combdamp1, 15);
		comb2bufR[comb2indexR] = sat16(input + sat16(comb2filterR * combfeeback, 15), 0);
		if (++comb2indexR >= sizeof(comb2bufR)/sizeof(int16_t)) comb2indexR = 0;

		bufout = comb3bufR[comb3indexR];
		sum += bufout;
		comb3filterR = sat16(bufout * combdamp2 + comb3filterR * combdamp1, 15);
		comb3bufR[comb3indexR] = sat16(input + sat16(comb3filterR * combfeeback, 15), 0);
		if (++comb3indexR >= sizeof(comb3bufR)/sizeof(int16_t)) comb3indexR = 0;

		bufout = comb4bufR[comb4indexR];
		sum += bufout;
		comb4filterR = sat16(bufout * combdamp2 + comb4filterR * combdamp1, 15);
		comb4bufR[comb4indexR] = sat16(input + sat16(comb4filterR * combfeeback, 15), 0);
		if (++comb4indexR >= sizeof(comb4bufR)/sizeof(int16_t)) comb4indexR = 0;

		bufout = comb5bufR[comb5indexR];
		sum += bufout;
		comb5filterR = sat16(bufout * combdamp2 + comb5filterR * combdamp1, 15);
		comb5bufR[comb5indexR] = sat16(input + sat16(comb5filterR * combfeeback, 15), 0);
		if (++comb5indexR >= sizeof(comb5bufR)/sizeof(int16_t)) comb5indexR = 0;

		bufout = comb6bufR[comb6indexR];
		sum += bufout;
		comb6filterR = sat16(bufout * combdamp2 + comb6filterR * combdamp1, 15);
		comb6bufR[comb6indexR] = sat16(input + sat16(comb6filterR * combfeeback, 15), 0);
		if (++comb6indexR >= sizeof(comb6bufR)/sizeof(int16_t)) comb6indexR = 0;

		bufout = comb7bufR[comb7indexR];
		sum += bufout;
		comb7filterR = sat16(bufout * combdamp2 + comb7filterR * combdamp1, 15);
		comb7bufR[comb7indexR] = sat16(input + sat16(comb7filterR * combfeeback, 15), 0);
		if (++comb7indexR >= sizeof(comb7bufR)/sizeof(int16_t)) comb7indexR = 0;

		bufout = comb8bufR[comb8indexR];
		sum += bufout;
		comb8filterR = sat16(bufout * combdamp2 + comb8filterR * combdamp1, 15);
		comb8bufR[comb8indexR] = sat16(input + sat16(comb8filterR * combfeeback, 15), 0);
		if (++comb8indexR >= sizeof(comb8bufR)/sizeof(int16_t)) comb8indexR = 0;

		outputR = sat16(sum * 31457, 17);

		bufout = allpass1bufL[allpass1indexL];
		allpass1bufL[allpass1indexL] = outputL + (bufout >> 1);
		outputL = sat16(bufout - outputL, 1);
		if (++allpass1indexL >= sizeof(allpass1bufL)/sizeof(int16_t)) allpass1indexL = 0;

		bufout = allpass2bufL[allpass2indexL];
		allpass2bufL[allpass2indexL] = outputL + (bufout >> 1);
		outputL = sat16(bufout - outputL, 1);
		if (++allpass2indexL >= sizeof(allpass2bufL)/sizeof(int16_t)) allpass2indexL = 0;

		bufout = allpass3bufL[allpass3indexL];
		allpass3bufL[allpass3indexL] = outputL + (bufout >> 1);
		outputL = sat16(bufout - outputL, 1);
		if (++allpass3indexL >= sizeof(allpass3bufL)/sizeof(int16_t)) allpass3indexL = 0;

		bufout = allpass4bufL[allpass4indexL];
		allpass4bufL[allpass4indexL] = outputL + (bufout >> 1);
		outputL = sat16(bufout - outputL, 1);
		if (++allpass4indexL >= sizeof(allpass4bufL)/sizeof(int16_t)) allpass4indexL = 0;

		outblockL->data[i] = sat16(outputL * 30, 0);

		bufout = allpass1bufR[allpass1indexR];
		allpass1bufR[allpass1indexR] = outputR + (bufout >> 1);
		outputR = sat16(bufout - outputR, 1);
		if (++allpass1indexR >= sizeof(allpass1bufR)/sizeof(int16_t)) allpass1indexR = 0;

		bufout = allpass2bufR[allpass2indexR];
		allpass2bufR[allpass2indexR] = outputR + (bufout >> 1);
		outputR = sat16(bufout - outputR, 1);
		if (++allpass2indexR >= sizeof(allpass2bufR)/sizeof(int16_t)) allpass2indexR = 0;

		bufout = allpass3bufR[allpass3indexR];
		allpass3bufR[allpass3indexR] = outputR + (bufout >> 1);
		outputR = sat16(bufout - outputR, 1);
		if (++allpass3indexR >= sizeof(allpass3bufR)/sizeof(int16_t)) allpass3indexR = 0;

		bufout = allpass4bufR[allpass4indexR];
		allpass4bufR[allpass4indexR] = outputR + (bufout >> 1);
		outputR = sat16(bufout - outputR, 1);
		if (++allpass4indexR >= sizeof(allpass4bufR)/sizeof(int16_t)) allpass4indexR = 0;

		outblockR->data[i] = sat16(outputL * 30, 0);
	}
	transmit(outblockL, 0);
	transmit(outblockR, 1);
	release(outblockL);
	release(outblockR);
	if (block != &zeroblock) release((audio_block_t *)block);

#elif defined(KINETISL)
	audio_block_t *block;
	block = receiveReadOnly(0);
	if (block) release(block);
#endif
}



