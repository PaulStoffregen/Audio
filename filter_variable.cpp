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

#include "filter_variable.h"
#include "utility/dspinst.h"

// State Variable Filter (Chamberlin) with 2X oversampling
// http://www.musicdsp.org/showArchiveComment.php?ArchiveID=92

// The fast 32x32 with rshift32 discards 2 bits, which probably
// never matter.
//#define MULT(a, b) (int32_t)(((int64_t)(a) * (b)) >> 30)
#define MULT(a, b) (multiply_32x32_rshift32_rounded(a, b) << 2)

// It's very unlikely anyone could hear any difference, but if you
// care deeply about numerical precision in seldom-used cases,
// uncomment this to improve the control signal accuracy
//#define IMPROVE_HIGH_FREQUENCY_ACCURACY

// This increases the exponential approximation accuracy from
// about 0.341% error to only 0.012% error, which probably makes
// no audible difference.
//#define IMPROVE_EXPONENTIAL_ACCURACY

#if defined(KINETISK) || defined(__SAMD51__)

void AudioFilterStateVariable::update_fixed(const int16_t *in,
	int16_t *lp, int16_t *bp, int16_t *hp)
{
	const int16_t *end = in + AUDIO_BLOCK_SAMPLES;
	int32_t input, inputprev;
	int32_t lowpass, bandpass, highpass;
	int32_t lowpasstmp, bandpasstmp, highpasstmp;
	int32_t fmult, damp;

	fmult = setting_fmult;
	damp = setting_damp;
	inputprev = state_inputprev;
	lowpass = state_lowpass;
	bandpass = state_bandpass;
	do {
		input = (*in++) << 12;
		lowpass = lowpass + MULT(fmult, bandpass);
		highpass = ((input + inputprev)>>1) - lowpass - MULT(damp, bandpass);
		inputprev = input;
		bandpass = bandpass + MULT(fmult, highpass);
		lowpasstmp = lowpass;
		bandpasstmp = bandpass;
		highpasstmp = highpass;
		lowpass = lowpass + MULT(fmult, bandpass);
		highpass = input - lowpass - MULT(damp, bandpass);
		bandpass = bandpass + MULT(fmult, highpass);
		lowpasstmp = signed_saturate_rshift(lowpass+lowpasstmp, 16, 13);
		bandpasstmp = signed_saturate_rshift(bandpass+bandpasstmp, 16, 13);
		highpasstmp = signed_saturate_rshift(highpass+highpasstmp, 16, 13);
		*lp++ = lowpasstmp;
		*bp++ = bandpasstmp;
		*hp++ = highpasstmp;
	} while (in < end);
	state_inputprev = inputprev;
	state_lowpass = lowpass;
	state_bandpass = bandpass;
}


void AudioFilterStateVariable::update_variable(const int16_t *in,
	const int16_t *ctl, int16_t *lp, int16_t *bp, int16_t *hp)
{
	const int16_t *end = in + AUDIO_BLOCK_SAMPLES;
	int32_t input, inputprev, control;
	int32_t lowpass, bandpass, highpass;
	int32_t lowpasstmp, bandpasstmp, highpasstmp;
	int32_t fcenter, fmult, damp, octavemult;
	int32_t n;

	fcenter = setting_fcenter;
	octavemult = setting_octavemult;
	damp = setting_damp;
	inputprev = state_inputprev;
	lowpass = state_lowpass;
	bandpass = state_bandpass;
	do {
		// compute fmult using control input, fcenter and octavemult
		control = *ctl++;          // signal is always 15 fractional bits
		control *= octavemult;     // octavemult range: 0 to 28671 (12 frac bits)
		n = control & 0x7FFFFFF;   // 27 fractional control bits
		#ifdef IMPROVE_EXPONENTIAL_ACCURACY
		// exp2 polynomial suggested by Stefan Stenzel on "music-dsp"
		// mail list, Wed, 3 Sep 2014 10:08:55 +0200
		int32_t x = n << 3;
		n = multiply_accumulate_32x32_rshift32_rounded(536870912, x, 1494202713);
		int32_t sq = multiply_32x32_rshift32_rounded(x, x);
		n = multiply_accumulate_32x32_rshift32_rounded(n, sq, 1934101615);
		n = n + (multiply_32x32_rshift32_rounded(sq,
			multiply_32x32_rshift32_rounded(x, 1358044250)) << 1);
		n = n << 1;
		#else
		// exp2 algorithm by Laurent de Soras
		// http://www.musicdsp.org/showone.php?id=106
		n = (n + 134217728) << 3;
		n = multiply_32x32_rshift32_rounded(n, n);
		n = multiply_32x32_rshift32_rounded(n, 715827883) << 3;
		n = n + 715827882;
		#endif
		n = n >> (6 - (control >> 27)); // 4 integer control bits
		fmult = multiply_32x32_rshift32_rounded(fcenter, n);
		if (fmult > 5378279) fmult = 5378279;
		fmult = fmult << 8;
		// fmult is within 0.4% accuracy for all but the top 2 octaves
		// of the audio band.  This math improves accuracy above 5 kHz.
		// Without this, the filter still works fine for processing
		// high frequencies, but the filter's corner frequency response
		// can end up about 6% higher than requested.
		#ifdef IMPROVE_HIGH_FREQUENCY_ACCURACY
		// From "Fast Polynomial Approximations to Sine and Cosine"
		// Charles K Garrett, http://krisgarrett.net/
		fmult = (multiply_32x32_rshift32_rounded(fmult, 2145892402) +
			multiply_32x32_rshift32_rounded(
			multiply_32x32_rshift32_rounded(fmult, fmult),
			multiply_32x32_rshift32_rounded(fmult, -1383276101))) << 1;
		#endif
		// now do the state variable filter as normal, using fmult
		input = (*in++) << 12;
		lowpass = lowpass + MULT(fmult, bandpass);
		highpass = ((input + inputprev)>>1) - lowpass - MULT(damp, bandpass);
		inputprev = input;
		bandpass = bandpass + MULT(fmult, highpass);
		lowpasstmp = lowpass;
		bandpasstmp = bandpass;
		highpasstmp = highpass;
		lowpass = lowpass + MULT(fmult, bandpass);
		highpass = input - lowpass - MULT(damp, bandpass);
		bandpass = bandpass + MULT(fmult, highpass);
		lowpasstmp = signed_saturate_rshift(lowpass+lowpasstmp, 16, 13);
		bandpasstmp = signed_saturate_rshift(bandpass+bandpasstmp, 16, 13);
		highpasstmp = signed_saturate_rshift(highpass+highpasstmp, 16, 13);
		*lp++ = lowpasstmp;
		*bp++ = bandpasstmp;
		*hp++ = highpasstmp;
	} while (in < end);
	state_inputprev = inputprev;
	state_lowpass = lowpass;
	state_bandpass = bandpass;
}


void AudioFilterStateVariable::update(void)
{
	audio_block_t *input_block=NULL, *control_block=NULL;
	audio_block_t *lowpass_block=NULL, *bandpass_block=NULL, *highpass_block=NULL;

	input_block = receiveReadOnly(0);
	control_block = receiveReadOnly(1);
	if (!input_block) {
		if (control_block) release(control_block);
		return;
	}
	lowpass_block = allocate();
	if (!lowpass_block) {
		release(input_block);
		if (control_block) release(control_block);
		return;
	}
	bandpass_block = allocate();
	if (!bandpass_block) {
		release(input_block);
		release(lowpass_block);
		if (control_block) release(control_block);
		return;
	}
	highpass_block = allocate();
	if (!highpass_block) {
		release(input_block);
		release(lowpass_block);
		release(bandpass_block);
		if (control_block) release(control_block);
		return;
	}

	if (control_block) {
		update_variable(input_block->data,
			 control_block->data,
			 lowpass_block->data,
			 bandpass_block->data,
			 highpass_block->data);
		release(control_block);
	} else {
		update_fixed(input_block->data,
			 lowpass_block->data,
			 bandpass_block->data,
			 highpass_block->data);
	}
	release(input_block);
	transmit(lowpass_block, 0);
	release(lowpass_block);
	transmit(bandpass_block, 1);
	release(bandpass_block);
	transmit(highpass_block, 2);
	release(highpass_block);
	return;
}

#elif defined(KINETISL)

void AudioFilterStateVariable::update(void)
{
	audio_block_t *block;

	block = receiveReadOnly(0);
	if (block) release(block);
	block = receiveReadOnly(1);
	if (block) release(block);
}

#endif

