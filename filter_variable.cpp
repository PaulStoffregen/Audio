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

//#define MULT(a, b) (int32_t)(((int64_t)(a) * (b)) >> 30)
#define MULT(a, b) (multiply_32x32_rshift32_rounded(a, b) << 2)

static bool first = true;

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
#if 0
		if (first) {
			Serial.print(input >> 12);
			Serial.print(", ");
			Serial.print(lowpasstmp);
			Serial.print(", ");
			Serial.print(bandpasstmp);
			Serial.print(", ");
			Serial.print(highpasstmp);
			Serial.println();
		}
#endif
	} while (in < end);

	state_inputprev = inputprev;
	state_lowpass = lowpass;
	state_bandpass = bandpass;
	first = false;
}

void AudioFilterStateVariable::update_variable(const int16_t *in,
	const int16_t *ctl, int16_t *lp, int16_t *bp, int16_t *hp)
{
	// TODO: implement control signal modulation of corner frequency
	update_fixed(in, lp, bp, hp);
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

