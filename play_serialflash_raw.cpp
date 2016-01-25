/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 * Modified to use SerialFlash instead of SD library by Wyatt Olson <wyatt@digitalcave.ca>
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

#include "play_serialflash_raw.h"
#include "spi_interrupt.h"

extern "C" {
extern const int16_t lsx_ulaw2linear16[256];
};

void AudioPlaySerialflashRaw::begin(void)
{
	playing = 0;
	file_offset = 0;
	file_size = 0;
}


bool AudioPlaySerialflashRaw::play(const char *filename)
{
	stop();
	AudioStartUsingSPI();
	rawfile = SerialFlash.open(filename);
	if (!rawfile) {
		//Serial.println("unable to open file");
		AudioStopUsingSPI();
		return false;
	}
	file_size = rawfile.size();
	file_offset = 0;
	//Serial.println("able to open file");
	if(!strcmp(filename + strlen(filename) - 3, "ULW")) playing = 0x01; //ulaw
	else playing = 0x81;	//PCM 16 bit
	return true;
}

void AudioPlaySerialflashRaw::stop(void)
{
	__disable_irq();
	if (playing) {
		playing = 0;
		__enable_irq();
		rawfile.close();
		AudioStopUsingSPI();
	} else {
		__enable_irq();
	}
}

void AudioPlaySerialflashRaw::update(void)
{
	int16_t i;		//This needs to be signed for ulaw decoding
	uint16_t n;
	audio_block_t *block;

	// only update if we're playing
	if (!playing) return;

	// allocate the audio blocks to transmit
	block = allocate();
	if (block == NULL) return;

	if (rawfile.available()) {
		switch (playing) {
			case 0x01: // u-law encoded, 44100 Hz
				//In ulaw we encode 16 bits of audio data (well, effectively 14 bits...) into 8 bits on file.
				// To decode it, we first read AUDIO_BLOCK_SAMPLES bytes.  We then expand these into 
				// AUDIO_BLOCK_SAMPLES * 2 bytes (or, AUDIO_BLOCK_SAMPLES * 16 bit samples) using the ulaw
				// lookup table.  Be sure to zero out unused block data if this is at the end of the
				// file.
				n = rawfile.read(block->data, AUDIO_BLOCK_SAMPLES);
				file_offset += n;
				n &= 0xFFFE;	//We don't want an odd number (which would only happen at the end), or else we end samples with clicks.
				for (i = AUDIO_BLOCK_SAMPLES-1; i >= 0; i-=2) {
					if (i > n) {
						block->data[i] = 0;	//Zero out data after the end of the file
						block->data[i-1] = 0;
					}
					else {
						block->data[i] = lsx_ulaw2linear16[block->data[i>>1] & 0xFF];
						block->data[i-1] = lsx_ulaw2linear16[(block->data[i>>1] >> 8) & 0xFF];
					}
				}
				break;
			case 0x81: // 16 bit PCM, 44100 Hz
				// we can read more data from the file...
				n = rawfile.read(block->data, AUDIO_BLOCK_SAMPLES*2);
				file_offset += n;
				//Zero out any data after the end of the file
				for (i=n/2; i < AUDIO_BLOCK_SAMPLES; i++) {
					block->data[i] = 0;
				}
				break;
		}
		transmit(block);
	} else {
		rawfile.close();
		AudioStopUsingSPI();
		playing = 0;
		//Serial.println("Finished playing sample");		//TODO
	}
	release(block);
}

#define B2M (uint32_t)((double)4294967296000.0 / AUDIO_SAMPLE_RATE_EXACT / 2.0) // 97352592

uint32_t AudioPlaySerialflashRaw::positionMillis(void)
{
	return ((uint64_t)file_offset * B2M) >> 32;
}

uint32_t AudioPlaySerialflashRaw::lengthMillis(void)
{
	return ((uint64_t)file_size * B2M) >> 32;
}


