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
#include "synth_waveform.h"
#include "arm_math.h"
#include "utility/dspinst.h"


// uncomment for more accurate but more computationally expensive frequency modulation
//#define IMPROVE_EXPONENTIAL_ACCURACY


void AudioSynthWaveform::update(void)
{
	audio_block_t *block;
	int16_t *bp, *end;
	int32_t val1, val2;
	int16_t magnitude15;
	uint32_t i, ph, index, index2, scale;
	const uint32_t inc = phase_increment;

	ph = phase_accumulator + phase_offset;
	if (magnitude == 0) {
		phase_accumulator += inc * AUDIO_BLOCK_SAMPLES;
		return;
	}
	block = allocate();
	if (!block) {
		phase_accumulator += inc * AUDIO_BLOCK_SAMPLES;
		return;
	}
	bp = block->data;

	switch(tone_type) {
	case WAVEFORM_SINE:
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			index = ph >> 24;
			val1 = AudioWaveformSine[index];
			val2 = AudioWaveformSine[index+1];
			scale = (ph >> 8) & 0xFFFF;
			val2 *= scale;
			val1 *= 0x10000 - scale;
			*bp++ = multiply_32x32_rshift32(val1 + val2, magnitude);
			ph += inc;
		}
		break;

	case WAVEFORM_ARBITRARY:
		if (!arbdata) {
			release(block);
			phase_accumulator += inc * AUDIO_BLOCK_SAMPLES;
			return;
		}
		// len = 256
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			index = ph >> 24;
			index2 = index + 1;
			if (index2 >= 256) index2 = 0;
			val1 = *(arbdata + index);
			val2 = *(arbdata + index2);
			scale = (ph >> 8) & 0xFFFF;
			val2 *= scale;
			val1 *= 0x10000 - scale;
			*bp++ = multiply_32x32_rshift32(val1 + val2, magnitude);
			ph += inc;
		}
		break;

	case WAVEFORM_SQUARE:
		magnitude15 = signed_saturate_rshift(magnitude, 16, 1);
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			if (ph & 0x80000000) {
				*bp++ = -magnitude15;
			} else {
				*bp++ = magnitude15;
			}
			ph += inc;
		}
		break;

	case WAVEFORM_SAWTOOTH:
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			*bp++ = signed_multiply_32x16t(magnitude, ph);
			ph += inc;
		}
		break;

	case WAVEFORM_SAWTOOTH_REVERSE:
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			*bp++ = signed_multiply_32x16t(0xFFFFFFFFu - magnitude, ph);
			ph += inc;
		}
		break;

	case WAVEFORM_TRIANGLE:
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			uint32_t phtop = ph >> 30;
			if (phtop == 1 || phtop == 2) {
				*bp++ = ((0xFFFF - (ph >> 15)) * magnitude) >> 16;
			} else {
				*bp++ = (((int32_t)ph >> 15) * magnitude) >> 16;
			}
			ph += inc;
		}
		break;

	case WAVEFORM_TRIANGLE_VARIABLE:
		do {
		uint32_t rise = 0xFFFFFFFF / (pulse_width >> 16);
		uint32_t fall = 0xFFFFFFFF / (0xFFFF - (pulse_width >> 16));
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			if (ph < pulse_width/2) {
				uint32_t n = (ph >> 16) * rise;
				*bp++ = ((n >> 16) * magnitude) >> 16;
			} else if (ph < 0xFFFFFFFF - pulse_width/2) {
				uint32_t n = 0x7FFFFFFF - (((ph - pulse_width/2) >> 16) * fall);
				*bp++ = (((int32_t)n >> 16) * magnitude) >> 16;
			} else {
				uint32_t n = ((ph + pulse_width/2) >> 16) * rise + 0x80000000;
				*bp++ = (((int32_t)n >> 16) * magnitude) >> 16;
			}
			ph += inc;
		}
		} while (0);
		break;

	case WAVEFORM_PULSE:
		magnitude15 = signed_saturate_rshift(magnitude, 16, 1);
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			if (ph < pulse_width) {
				*bp++ = magnitude15;
			} else {
				*bp++ = -magnitude15;
			}
			ph += inc;
		}
		break;

	case WAVEFORM_SAMPLE_HOLD:
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			*bp++ = sample;
			uint32_t newph = ph + inc;
			if (newph < ph) {
				sample = random(magnitude) - (magnitude >> 1);
			}
			ph = newph;
		}
		break;
	}
	phase_accumulator = ph - phase_offset;

	if (tone_offset) {
		bp = block->data;
		end = bp + AUDIO_BLOCK_SAMPLES;
		do {
			val1 = *bp;
			*bp++ = signed_saturate_rshift(val1 + tone_offset, 16, 0);
		} while (bp < end);
	}
	transmit(block, 0);
	release(block);
}

//--------------------------------------------------------------------------------

void AudioSynthWaveformModulated::update(void)
{
	audio_block_t *block, *moddata, *shapedata;
	int16_t *bp, *end;
	int32_t val1, val2;
	int16_t magnitude15;
	uint32_t i, ph, index, index2, scale, priorphase;
	const uint32_t inc = phase_increment;

	moddata = receiveReadOnly(0);
	shapedata = receiveReadOnly(1);

	// Pre-compute the phase angle for every output sample of this update
	ph = phase_accumulator;
	priorphase = phasedata[AUDIO_BLOCK_SAMPLES-1];
	if (moddata && modulation_type == 0) {
		// Frequency Modulation
		bp = moddata->data;
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			int32_t n = (*bp++) * modulation_factor; // n is # of octaves to mod
			int32_t ipart = n >> 27; // 4 integer bits
			n &= 0x7FFFFFF;          // 27 fractional bits
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
			// https://www.musicdsp.org/en/latest/Other/106-fast-exp2-approximation.html
			n = (n + 134217728) << 3;
			n = multiply_32x32_rshift32_rounded(n, n);
			n = multiply_32x32_rshift32_rounded(n, 715827883) << 3;
			n = n + 715827882;
			#endif
			uint32_t scale = n >> (14 - ipart);
			uint64_t phstep = (uint64_t)inc * scale;
			uint32_t phstep_msw = phstep >> 32;
			if (phstep_msw < 0x7FFE) {
				ph += phstep >> 16;
			} else {
				ph += 0x7FFE0000;
			}
			phasedata[i] = ph;
		}
		release(moddata);
	} else if (moddata) {
		// Phase Modulation
		bp = moddata->data;
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			// more than +/- 180 deg shift by 32 bit overflow of "n"
			uint32_t n = (uint16_t)(*bp++) * modulation_factor;
			phasedata[i] = ph + n;
			ph += inc;
		}
		release(moddata);
	} else {
		// No Modulation Input
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			phasedata[i] = ph;
			ph += inc;
		}
	}
	phase_accumulator = ph;

	// If the amplitude is zero, no output, but phase still increments properly
	if (magnitude == 0) {
		if (shapedata) release(shapedata);
		return;
	}
	block = allocate();
	if (!block) {
		if (shapedata) release(shapedata);
		return;
	}
	bp = block->data;

	// Now generate the output samples using the pre-computed phase angles
	switch(tone_type) {
	case WAVEFORM_SINE:
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			ph = phasedata[i];
			index = ph >> 24;
			val1 = AudioWaveformSine[index];
			val2 = AudioWaveformSine[index+1];
			scale = (ph >> 8) & 0xFFFF;
			val2 *= scale;
			val1 *= 0x10000 - scale;
			*bp++ = multiply_32x32_rshift32(val1 + val2, magnitude);
		}
		break;

	case WAVEFORM_ARBITRARY:
		if (!arbdata) {
			release(block);
			if (shapedata) release(shapedata);
			return;
		}
		// len = 256
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			ph = phasedata[i];
			index = ph >> 24;
			index2 = index + 1;
			if (index2 >= 256) index2 = 0;
			val1 = *(arbdata + index);
			val2 = *(arbdata + index2);
			scale = (ph >> 8) & 0xFFFF;
			val2 *= scale;
			val1 *= 0x10000 - scale;
			*bp++ = multiply_32x32_rshift32(val1 + val2, magnitude);
		}
		break;

	case WAVEFORM_PULSE:
		if (shapedata) {
			magnitude15 = signed_saturate_rshift(magnitude, 16, 1);
			for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
				uint32_t width = ((shapedata->data[i] + 0x8000) & 0xFFFF) << 16;
				if (phasedata[i] < width) {
					*bp++ = magnitude15;
				} else {
					*bp++ = -magnitude15;
				}
			}
			break;
		} // else fall through to orginary square without shape modulation

	case WAVEFORM_SQUARE:
		magnitude15 = signed_saturate_rshift(magnitude, 16, 1);
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			if (phasedata[i] & 0x80000000) {
				*bp++ = -magnitude15;
			} else {
				*bp++ = magnitude15;
			}
		}
		break;

	case WAVEFORM_SAWTOOTH:
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			*bp++ = signed_multiply_32x16t(magnitude, phasedata[i]);
		}
		break;

	case WAVEFORM_SAWTOOTH_REVERSE:
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			*bp++ = signed_multiply_32x16t(0xFFFFFFFFu - magnitude, phasedata[i]);
		}
		break;

	case WAVEFORM_TRIANGLE_VARIABLE:
		if (shapedata) {
			for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
				uint32_t width = (shapedata->data[i] + 0x8000) & 0xFFFF;
				uint32_t rise = 0xFFFFFFFF / width;
				uint32_t fall = 0xFFFFFFFF / (0xFFFF - width);
				uint32_t halfwidth = width << 15;
				uint32_t n;
				ph = phasedata[i];
				if (ph < halfwidth) {
					n = (ph >> 16) * rise;
					*bp++ = ((n >> 16) * magnitude) >> 16;
				} else if (ph < 0xFFFFFFFF - halfwidth) {
					n = 0x7FFFFFFF - (((ph - halfwidth) >> 16) * fall);
					*bp++ = (((int32_t)n >> 16) * magnitude) >> 16;
				} else {
					n = ((ph + halfwidth) >> 16) * rise + 0x80000000;
					*bp++ = (((int32_t)n >> 16) * magnitude) >> 16;
				}
				ph += inc;
			}
			break;
		} // else fall through to orginary triangle without shape modulation

	case WAVEFORM_TRIANGLE:
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			ph = phasedata[i];
			uint32_t phtop = ph >> 30;
			if (phtop == 1 || phtop == 2) {
				*bp++ = ((0xFFFF - (ph >> 15)) * magnitude) >> 16;
			} else {
				*bp++ = (((int32_t)ph >> 15) * magnitude) >> 16;
			}
		}
		break;
	case WAVEFORM_SAMPLE_HOLD:
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			ph = phasedata[i];
			if (ph < priorphase) { // does not work for phase modulation
				sample = random(magnitude) - (magnitude >> 1);
			}
			priorphase = ph;
			*bp++ = sample;
		}
		break;
	}

	if (tone_offset) {
		bp = block->data;
		end = bp + AUDIO_BLOCK_SAMPLES;
		do {
			val1 = *bp;
			*bp++ = signed_saturate_rshift(val1 + tone_offset, 16, 0);
		} while (bp < end);
	}
	if (shapedata) release(shapedata);
	transmit(block, 0);
	release(block);
}


