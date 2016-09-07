/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Pete (El Supremo)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef filter_fir_h_
#define filter_fir_h_

#include "Arduino.h"
#include "AudioStream.h"
#include "arm_math.h"

// Indicates that the code should just pass through the audio
// without any filtering (as opposed to doing nothing at all)
#define FIR_PASSTHRU ((const short *) 1)

#define FIR_MAX_COEFFS 200

class AudioFilterFIR : public AudioStream
{
public:
	AudioFilterFIR(void): AudioStream(1,inputQueueArray), coeff_p(NULL) {
	}
	void begin(const short *cp, int n_coeffs) {
		coeff_p = cp;
		// Initialize FIR instance (ARM DSP Math Library)
		if (coeff_p && (coeff_p != FIR_PASSTHRU) && n_coeffs <= FIR_MAX_COEFFS) {
			if (arm_fir_init_q15(&fir_inst, n_coeffs, (q15_t *)coeff_p,
			  &StateQ15[0], AUDIO_BLOCK_SAMPLES) != ARM_MATH_SUCCESS) {
				// n_coeffs must be an even number, 4 or larger
				coeff_p = NULL;
			}
		}
	}
	void end(void) {
		coeff_p = NULL;
	}
	virtual void update(void);
private:
	audio_block_t *inputQueueArray[1];

	// pointer to current coefficients or NULL or FIR_PASSTHRU
	const short *coeff_p;

	// ARM DSP Math library filter instance
	arm_fir_instance_q15 fir_inst;
	q15_t StateQ15[AUDIO_BLOCK_SAMPLES + FIR_MAX_COEFFS];
};

#endif
