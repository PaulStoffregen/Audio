/* Audio Library for Teensy, Ladder Filter
 * Copyright (c) 2021, Richard van Hoesel
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

//-----------------------------------------------------------
// Huovilainen New Moog (HNM) model as per CMJ jun 2006
// Implemented as Teensy Audio Library compatible object
// Richard van Hoesel, Feb. 9 2021
// v.1.02 now includes both cutoff and resonance "CV" modulation inputs
// please retain this header if you use this code.
//-----------------------------------------------------------

// https://forum.pjrc.com/threads/60488?p=269755&viewfull=1#post269755
// https://forum.pjrc.com/threads/60488?p=269609&viewfull=1#post269609

#include <Arduino.h>
#include "filter_ladder.h"
#include <math.h>
#include <stdint.h>
#define MOOG_PI ((float)3.14159265358979323846264338327950288)

#define MAX_RESONANCE ((float)1.1)

float AudioFilterLadder::LPF(float s, int i)
{
	float ft = s * (1.0f/1.3f) + (0.3f/1.3f) * z0[i] - z1[i];
	ft = ft * alpha + z1[i];
	z1[i] = ft;
	z0[i] = s;
	return ft;
}

void AudioFilterLadder::resonance(float res)
{
	// maps resonance = 0->1 to K = 0 -> 4
	if (res > MAX_RESONANCE) {
		res = MAX_RESONANCE;
	} else if (res < 0.0f) {
		res = 0.0f;
	}
	K = 4.0f * res;
}

void AudioFilterLadder::frequency(float c)
{
	Fbase = c;
	compute_coeffs(c);
}

void AudioFilterLadder::compute_coeffs(float c)
{
	if (c > 0.49f * AUDIO_SAMPLE_RATE_EXACT) {
		c = 0.49f * AUDIO_SAMPLE_RATE_EXACT;
	} else if (c < 1.0f) {
		c = 1.0f;
	}
	float wc = c * (float)(2.0f * MOOG_PI / AUDIO_SAMPLE_RATE_EXACT);
	float wc2 = wc * wc;
	alpha = 0.9892f * wc - 0.4324f * wc2 + 0.1381f * wc * wc2 - 0.0202f * wc2 * wc2;
}

static inline float fast_tanh(float x)
{
	float x2 = x * x;
	return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

void AudioFilterLadder::update(void)
{
	audio_block_t *blocka, *blockb, *blockc;
	float Ktot;
	bool FCmodActive = true;
	bool QmodActive = true;

	blocka = receiveWritable(0);
	blockb = receiveReadOnly(1);
	blockc = receiveReadOnly(2);
	if (!blocka) {
		blocka = allocate();
		if (!blocka) {
			if (blockb) release(blockb);
			if (blockc) release(blockc);
			return;
		}
		// When no data arrives, we must treat it as if zeros had
		// arrived.  Because of resonance, we need to keep computing
		// output.  Perhaps we could examine the filter state here
		// and just return without any work when it's below some
		// threshold we know produces no more sound/resonance?
		for (int i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			blocka->data[i] = 0;
		}
	}
	if (!blockb) {
		FCmodActive = false;
	}
	if (!blockc) {
		QmodActive = false;
		Ktot = K;
	}
	for (int i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
		float input = blocka->data[i] * (1.0f/32768.0f);
		if (FCmodActive) {
			float FCmod = blockb->data[i] * (1.0f/32768.0f);
			// TODO: should this be "volts per octave"?
			float ftot = Fbase + Fbase * FCmod;
			if (FCmod != 0) compute_coeffs(ftot);
		}
		if (QmodActive) {
			float Qmod = blockc->data[i] * (1.0f/32768.0f);
			Ktot = K + (MAX_RESONANCE * 1.1f) * Qmod;
			if (Ktot < 0.0f) Ktot = 0.0f;
		}
		float u = input - (z1[3] - 0.5f * input) * Ktot;
		u = fast_tanh(u);
		float stage1 = LPF(u, 0);
		float stage2 = LPF(stage1, 1);
		float stage3 = LPF(stage2, 2);
		float stage4 = LPF(stage3, 3);
		blocka->data[i] = stage4 * 32767.0f;
	}
	transmit(blocka);
	release(blocka);
	if (blockb) release(blockb);
	if (blockc) release(blockc);
}

