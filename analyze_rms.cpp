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


#include "analyze_rms.h"
#include <arm_math.h>

void AudioAnalyzeRMS::update(void)
{
	audio_block_t *block;
	int16_t rmsResult;

	block = receiveReadOnly();
	if (!block) {
		return;
	}

	// not reinventing the wheel:
	// use DSP packed 32i instructions as found in arm_math.h, with 64b accumulator
	arm_rms_q15(block->data, AUDIO_BLOCK_SAMPLES, &rmsResult); // seems to use ~2% CPU
	lastRMS = rmsResult; // prevent threading issues
	// for optimization, one could re-implement arm_rms_q15 to do the sqrt on read().
	// This way, the rms in dB could also be implemented faster:
	// Instead of 20*log10(sqrt(MSerror)), one could do write 10*log10(MSerror)

	new_output = true;
	release(block);
}

