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

#include "play_sd_raw.h"


void AudioPlaySDcardRAW::begin(void)
{
	playing = false;
	if (block) {
		release(block);
		block = NULL;
	}
}


bool AudioPlaySDcardRAW::play(const char *filename)
{
	stop();
	rawfile = SD.open(filename);
	if (!rawfile) {
		Serial.println("unable to open file");
		return false;
	}
	Serial.println("able to open file");
	playing = true;
	return true;
}

void AudioPlaySDcardRAW::stop(void)
{
	__disable_irq();
	if (playing) {
		playing = false;
		__enable_irq();
		rawfile.close();
	} else {
		__enable_irq();
	}
}


void AudioPlaySDcardRAW::update(void)
{
	unsigned int i, n;

	// only update if we're playing
	if (!playing) return;

	// allocate the audio blocks to transmit
	block = allocate();
	if (block == NULL) return;

	if (rawfile.available()) {
		// we can read more data from the file...
		n = rawfile.read(block->data, AUDIO_BLOCK_SAMPLES*2);
		for (i=n/2; i < AUDIO_BLOCK_SAMPLES; i++) {
			block->data[i] = 0;
		}
		transmit(block);
		release(block);
	} else {
		rawfile.close();
		playing = false;
	}
}


