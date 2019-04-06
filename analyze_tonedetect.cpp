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

#include "analyze_tonedetect.h"
#include "utility/dspinst.h"

#if defined(KINETISK) || defined(__SAMD51__)

static inline int32_t multiply_32x32_rshift30(int32_t a, int32_t b) __attribute__((always_inline));
static inline int32_t multiply_32x32_rshift30(int32_t a, int32_t b)
{
	return ((int64_t)a * (int64_t)b) >> 30;
}

//#define TONE_DETECT_FAST

void AudioAnalyzeToneDetect::update(void)
{
	audio_block_t *block;
	int32_t q0, q1, q2, coef;
	const int16_t *p, *end;
	uint16_t n;

	block = receiveReadOnly();
	if (!block) return;
	if (!enabled) {
		release(block);
		return;
	}
	p = block->data;
	end = p + AUDIO_BLOCK_SAMPLES;
	n = count;
	coef = coefficient;
	q1 = s1;
	q2 = s2;
	do {
		// the Goertzel algorithm is kinda magical ;-)
#ifdef TONE_DETECT_FAST
		q0 = (*p++) + (multiply_32x32_rshift32_rounded(coef, q1) << 2) - q2;
#else
		q0 = (*p++) + multiply_32x32_rshift30(coef, q1) - q2;
		// TODO: is this only 1 cycle slower?  if so, always use it
#endif
		q2 = q1;
		q1 = q0;
		if (--n == 0) {
			out1 = q1;
			out2 = q2;
			q1 = 0;  // TODO: does clearing these help or hinder?
			q2 = 0;
			new_output = true;
			n = length;
		}
	} while (p < end);
	count = n;
	s1 = q1;
	s2 = q2;
	release(block);
}

void AudioAnalyzeToneDetect::set_params(int32_t coef, uint16_t cycles, uint16_t len)
{
	__disable_irq();
	coefficient = coef;
	ncycles = cycles;
	length = len;
	count = len;
	s1 = 0;
	s2 = 0;
	enabled = true;
	__enable_irq();
	//Serial.printf("Tone: coef=%d, ncycles=%d, length=%d\n", coefficient, ncycles, length);
}

float AudioAnalyzeToneDetect::read(void)
{
	int32_t coef, q1, q2, power;
	uint16_t len;

	__disable_irq();
	coef = coefficient;
	q1 = out1;
	q2 = out2;
	len = length;
	__enable_irq();
#ifdef TONE_DETECT_FAST
	power = multiply_32x32_rshift32_rounded(q2, q2);
	power = multiply_accumulate_32x32_rshift32_rounded(power, q1, q1);
	power = multiply_subtract_32x32_rshift32_rounded(power,
		multiply_32x32_rshift30(q1, q2), coef);
	power <<= 4;
#else
	int64_t power64;
	power64 = (int64_t)q2 * (int64_t)q2;
	power64 += (int64_t)q1 * (int64_t)q1;
	power64 -= (((int64_t)q1 * (int64_t)q2) >> 30) * (int64_t)coef;
	power = power64 >> 28;
#endif
	return sqrtf((float)power) / (float)len;
}


AudioAnalyzeToneDetect::operator bool()
{
	int32_t coef, q1, q2, power, trigger;
	uint16_t len;

	__disable_irq();
	coef = coefficient;
	q1 = out1;
	q2 = out2;
	len = length;
	__enable_irq();
#ifdef TONE_DETECT_FAST
	power = multiply_32x32_rshift32_rounded(q2, q2);
	power = multiply_accumulate_32x32_rshift32_rounded(power, q1, q1);
	power = multiply_subtract_32x32_rshift32_rounded(power,
		multiply_32x32_rshift30(q1, q2), coef);
	power <<= 4;
#else
	int64_t power64;
	power64 = (int64_t)q2 * (int64_t)q2;
	power64 += (int64_t)q1 * (int64_t)q1;
	power64 -= (((int64_t)q1 * (int64_t)q2) >> 30) * (int64_t)coef;
	power = power64 >> 28;
#endif
	trigger = (uint32_t)len * thresh;
	trigger = multiply_32x32_rshift32(trigger, trigger);

	//Serial.printf("bool: power=%d, trig=%d\n", power, trigger);
	return (power >= trigger);
	// TODO: this should really remember if it's retuned true previously,
	// so it can give a single true response each time a tone is seen.
}


#elif defined(KINETISL)

void AudioAnalyzeToneDetect::update(void)
{
	audio_block_t *block;
	block = receiveReadOnly();
	if (block) release(block);
}

#endif

