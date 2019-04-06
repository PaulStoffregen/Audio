/* Audio Library for Teensy 3.X
 * Copyright (c) 2017, Paul Stoffregen, paul@pjrc.com
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

#include "synth_pwm.h"
#include "utility/dspinst.h"


#if defined(KINETISK) || defined(__SAMD51__)

void AudioSynthWaveformPWM::update(void)
{
	audio_block_t *block, *modinput;
	uint32_t i;
	int32_t out;

	modinput = receiveReadOnly();
	if (magnitude == 0) {
		if (modinput) release(modinput);
		return;
	}
	block = allocate();
	if (!block) {
		// unable to allocate memory, so we'll send nothing
		if (modinput) release(modinput);
		return;
	}
	if (modinput) {
		const uint32_t _duration = duration;
		uint32_t _elapsed = elapsed;
		int32_t _magnitude = magnitude;
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			_elapsed += 65536;
			int32_t in = modinput->data[i];
			if (_magnitude < 0) in = -in;
			uint32_t dur = ((uint64_t)(in + 32768) * _duration) >> 15;
			if (_elapsed < dur) {
				out = _magnitude;
			} else {
				int32_t e = _elapsed - dur;
				signed_saturate_rshift(e, 17, 0);
				if (e < 0) e = 0;
				_elapsed = e;
				// elapsed must be 0 to 65535
				// magnitude must be -32767 to +32767
				out = _magnitude - ((_magnitude * _elapsed) >> 15);
				_magnitude = -_magnitude;
			}
			block->data[i] = out;
		}
		elapsed = _elapsed;
		magnitude = _magnitude;
		release(modinput);
	} else {
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			elapsed += 65536;
			if (elapsed < duration) {
				out = magnitude;
			} else {
				elapsed -= duration;
				// elapsed must be 0 to 65535
				// magnitude must be -32767 to +32767
				out = magnitude - ((magnitude * elapsed) >> 15);
				magnitude = -magnitude;
			}
			block->data[i] = out;
		}
	}
	transmit(block);
	release(block);
}

#elif defined(KINETISL)

void AudioSynthWaveformPWM::update(void)
{
	audio_block_t *block = receiveReadOnly();
	if (block) release(block);
}

#endif

