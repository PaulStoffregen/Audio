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

#include "AudioStream.h"
#include "arm_math.h"

// Indicates that the code should just pass through the audio
// without any filtering (as opposed to doing nothing at all)
#define FIR_PASSTHRU ((short *) 1)

#define FIR_MAX_COEFFS 120

class AudioFilterFIR : public AudioStream
{
public:
	AudioFilterFIR(const boolean a_f): AudioStream(1,inputQueueArray),
	  arm_fast(a_f), coeff_p(NULL) { 
	}
	void begin(short *coeff_p,int n_coeffs);
	virtual void update(void);
	void stop(void);
  
private:
	audio_block_t *inputQueueArray[1];
	// arm state arrays and FIR instances for left and right channels
	// the state arrays are defined to handle a maximum of MAX_COEFFS
	// coefficients in a filter
	q15_t l_StateQ15[AUDIO_BLOCK_SAMPLES + FIR_MAX_COEFFS];

	// Whether to use the fast arm FIR code 
	const boolean arm_fast;
	// pointer to current coefficients or NULL or FIR_PASSTHRU
	short *coeff_p;
	arm_fir_instance_q15 l_fir_inst;
};

#endif
