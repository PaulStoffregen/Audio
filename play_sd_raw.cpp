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

#if defined(ARDUINO_ARCH_SAMD)
#include <Arduino.h>
#endif

#include "play_sd_raw.h"
#include "spi_interrupt.h"


// File system object.
extern SdFat SD;


void AudioPlaySdRaw::begin(void)
{
	playing = false;
	file_offset = 0;
	file_size = 0;
}


bool AudioPlaySdRaw::play(const char *filename)
{
	stop();
#if defined(HAS_KINETIS_SDHC)
	if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStartUsingSPI();
#else
	AudioStartUsingSPI();
#endif
	__disable_irq();
	rawfile = SD.open(filename);
	__enable_irq();
	if (!rawfile) {
		//Serial.println("unable to open file");
		#if defined(HAS_KINETIS_SDHC)
			if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStopUsingSPI();
		#else
			AudioStopUsingSPI();
		#endif
		return false;
	}
	file_size = rawfile.size();
	file_offset = 0;
	//Serial.println("able to open file");
	playing = true;
	return true;
}

void AudioPlaySdRaw::stop(void)
{
	__disable_irq();
	if (playing) {
		playing = false;
		__enable_irq();
		rawfile.close();
		#if defined(HAS_KINETIS_SDHC)
			if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStopUsingSPI();
		#else
			AudioStopUsingSPI();
		#endif
	} else {
		__enable_irq();
	}
}


void AudioPlaySdRaw::update(void)
{
	unsigned int i, n;
	audio_block_t *block;

	// only update if we're playing
	if (!playing) return;

	// allocate the audio blocks to transmit
	block = allocate();
	if (block == NULL) return;

	if (rawfile.available()) {
		// we can read more data from the file...
		n = rawfile.read(block->data, AUDIO_BLOCK_SAMPLES*2);
		file_offset += n;
		for (i=n/2; i < AUDIO_BLOCK_SAMPLES; i++) {
			block->data[i] = 0;
		}
		transmit(block);
	} else {
		rawfile.close();
		#if defined(HAS_KINETIS_SDHC)
			if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStopUsingSPI();
		#else
			AudioStopUsingSPI();
		#endif
		playing = false;
	}
	release(block);
}

#define B2M (uint32_t)((double)4294967296000.0 / AUDIO_SAMPLE_RATE_EXACT / 2.0) // 97352592

uint32_t AudioPlaySdRaw::positionMillis(void)
{
	return ((uint64_t)file_offset * B2M) >> 32;
}

uint32_t AudioPlaySdRaw::lengthMillis(void)
{
	return ((uint64_t)file_size * B2M) >> 32;
}
