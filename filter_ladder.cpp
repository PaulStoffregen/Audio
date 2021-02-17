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
// v1.4 FC extended to 18.7kHz, max res to 1.8, 4x oversampling,
//      and a minor Q-tuning adjustment
// v.1.03 adds oversampling, extended resonance,
// and exposes parameters input_drive and passband_gain
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

//#define osTimes 1
//#define MAX_RESONANCE ((float)1.1)
//#define MAX_FREQUENCY ((float)(AUDIO_SAMPLE_RATE_EXACT * 0.249f))

//#define osTimes 2
//#define MAX_RESONANCE ((float)1.2)
//#define MAX_FREQUENCY ((float)(AUDIO_SAMPLE_RATE_EXACT * 0.38f))

#define osTimes 4
#define MAX_RESONANCE ((float)1.8)
#define MAX_FREQUENCY ((float)(AUDIO_SAMPLE_RATE_EXACT * 0.425f))
//#define lfq 0.25

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

void AudioFilterLadder::octaveControl(float octaves)
{
	if (octaves > 7.0f) {
		octaves = 7.0f;
	} else if (octaves < 0.0f) {
		octaves = 0.0f;
	}
	octaveScale = octaves / 32768.0f;
}

void AudioFilterLadder:: passband_gain(float passbandgain)
{
	pbg = passbandgain;
	if (pbg > 0.5f) pbg = 0.5f;
	if (pbg < 0.0f) pbg = 0.0f;
	input_drive(host_overdrive);
}

void AudioFilterLadder::input_drive(float odrv)
{
	host_overdrive = odrv;
	if (host_overdrive > 1.0f) {
		if (host_overdrive > 4.0f) host_overdrive = 4.0f;
		// max is 4 when pbg = 0, and 2.5 when pbg is 0.5
		overdrive = 1.0f + (host_overdrive - 1.0f) * (1.0f - pbg);
	} else {
		overdrive = host_overdrive;
		if (overdrive < 0.0f) overdrive = 0.0f;
	}
}

void AudioFilterLadder::compute_coeffs(float c)
{
	if (c > MAX_FREQUENCY) {
		c = MAX_FREQUENCY;
	} else if (c < 5.0f) {
		c = 5.0f;
	}
#ifdef lfq
	if (c < 500.0f) lfkmod = 1.0f + (500.0f - c) * (1.0f/500.0f) * lfq;
#endif
	float wc = c * (float)(2.0f * MOOG_PI / ((float)osTimes * AUDIO_SAMPLE_RATE_EXACT));
	float wc2 = wc * wc;
	alpha = 0.9892f * wc - 0.4324f * wc2 + 0.1381f * wc * wc2 - 0.0202f * wc2 * wc2;
	//Qadjust = 1.0029f + 0.0526f * wc - 0.0926 * wc2 + 0.0218* wc * wc2;
	//Qadjust = 1.006f + 0.0536f * wc - 0.095 * wc2 ;
	Qadjust = 1.006f + 0.0536f * wc - 0.095f * wc2 - 0.05f * wc2 * wc2;
}

bool AudioFilterLadder::resonating()
{
	for (int i=0; i < 4; i++) {
		if (fabsf(z0[i]) > 0.0001f) return true;
		if (fabsf(z1[i]) > 0.0001f) return true;
	}
	return false;
}

static inline float fast_exp2f(float x)
{
	float i;
	float f = modff(x, &i);
	f *= 0.693147f / 256.0f;
	f += 1.0f;
	f *= f;
	f *= f;
	f *= f;
	f *= f;
	f *= f;
	f *= f;
	f *= f;
	f *= f;
	f = ldexpf(f, i);
	return f;
}

static inline float fast_tanh(float x)
{
	float x2 = x * x;
	return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

void AudioFilterLadder::update(void)
{
	audio_block_t *blocka, *blockb, *blockc;
	float Ktot, Kmax;
	bool FCmodActive = true;
	bool QmodActive = true;

	blocka = receiveWritable(0);
	blockb = receiveReadOnly(1);
	blockc = receiveReadOnly(2);
	if (!blocka) {
		if (resonating()) {
			// When no data arrives but the filter is still
			// resonating, we must continue computing the filter
			// with zero input to sustain the resonance
			blocka = allocate();
		}
		if (!blocka) {
			if (blockb) release(blockb);
			if (blockc) release(blockc);
			return;
		}
		for (int i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			blocka->data[i] = 0;
		}
	}
	if (!blockb) {
		FCmodActive = false;
	}
	if (!blockc) {
		QmodActive = false;
	}
	for (int i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
		float input = blocka->data[i] * (1.0f/32768.0f) * overdrive;
		if (FCmodActive) {
			float FCmod = blockb->data[i] * octaveScale;
			float ftot = Fbase * fast_exp2f(FCmod);
			if (ftot > MAX_FREQUENCY) ftot = MAX_FREQUENCY;
			compute_coeffs(ftot);
		}
		if (QmodActive) {
			float Qmod = blockc->data[i] * (1.0f/32768.0f);
			Ktot = K + 4.0f * Qmod;
		} else {
			Ktot = K;
		}
		#ifdef lfq
		Kmax = MAX_RESONANCE * 4.0f * lfkmod;
		#else
		Kmax = MAX_RESONANCE * 4.0f;
		#endif
		if (Ktot > Kmax) {
			Ktot = Kmax;
		} else if (Ktot < 0.0f) {
			Ktot = 0.0f;
		}
		float total = 0.0f;
		for(int os = 0; os < osTimes; os++) {
			float u = input - (z1[3] - pbg * input) * Ktot * Qadjust;
			u = fast_tanh(u);
			float stage1 = LPF(u, 0);
			float stage2 = LPF(stage1, 1);
			float stage3 = LPF(stage2, 2);
			float stage4 = LPF(stage3, 3);
			total += stage4 * (1.0f / (float)osTimes);
		}
		blocka->data[i] = total * 32767.0f;
	}
	transmit(blocka);
	release(blocka);
	if (blockb) release(blockb);
	if (blockc) release(blockc);
}

