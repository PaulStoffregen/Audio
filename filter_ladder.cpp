
//-----------------------------------------------------------
// Huovilainen New Moog (HNM) model as per CMJ jun 2006
// Implemented as Teensy Audio Library compatible object
// Richard van Hoesel, Feb. 9 2021
// v.1.01 now includes FC "CV" modulation input
// please retain this header if you use this code.
//-----------------------------------------------------------

// https://forum.pjrc.com/threads/60488?p=269609&viewfull=1#post269609

#include <Arduino.h>
#include "filter_ladder.h"
#include <math.h>
#include <stdint.h>
#define MOOG_PI 3.14159265358979323846264338327950288

float HNMoog::LPF(float s, int i)
{
	float ft = s * (1/1.3) + (0.3/1.3) * z0[i] - z1[i];
	ft = ft * alpha + z1[i];
	z1[i] = ft;
	z0[i] = s;
	return ft;
}

void HNMoog::SetResonance(float res)
{
	// maps resonance = 0->1 to K = 0 -> 4
	if (res > 1) {
		res = 1;
	} else if (res < 0) {
		res = 0;
	}
	K = 4.0 * res;
}

void HNMoog::SetCutoff(float c)
{
	Fbase = c;
	compute_coeffs(c);
}

void HNMoog::compute_coeffs(float c)
{
	if (c > 0.49 * AUDIO_SAMPLE_RATE_EXACT)
		c = 0.49 * AUDIO_SAMPLE_RATE_EXACT;
	else if (c < 1)
		c = 1;

	float wc = c * (2.0 * MOOG_PI / AUDIO_SAMPLE_RATE_EXACT);
	float wc2 = wc * wc;
	alpha = 0.9892f * wc - 0.4324f * wc2 + 0.1381f * wc * wc2 - 0.0202f * wc2 * wc2;
}

inline float fast_tanh(float x)
{
	float x2 = x * x;
	return x * (27.0 + x2) / (27.0 + 9.0 * x2);
}

void HNMoog::update(void)
{
	audio_block_t *blocka, *blockb;
	float ftot, FCmod;
	bool FCmodActive = true;

	blocka = receiveWritable(0);
	blockb = receiveReadOnly(1);
	if (!blocka) {
		if (blockb) release(blockb);
		return;
	}
	if (!blockb) FCmodActive = false;
	for (int i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
		float input = blocka->data[i] * (1.0/32768);
		ftot = Fbase;
		if (FCmodActive) {
			FCmod = blockb->data[i] * (1.0/32768);
			ftot += Fbase * FCmod;
			if (FCmod != 0) compute_coeffs(ftot);
		}
		float u = input - (z1[3] - 0.5 * input) * K;
		u = fast_tanh(u);
		float stage1 = LPF(u,0);
		float stage2 = LPF(stage1,1);
		float stage3 = LPF(stage2,2);
		float stage4 = LPF(stage3,3);
		blocka->data[i] = stage4 * 32767.0;
	}
	transmit(blocka);
	release(blocka);
	if (blockb) release(blockb);
}

