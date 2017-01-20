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
#include "spi_interrupt.h"


void AudioPlaySdRaw::begin(void)
{
	read_ratio = 1.0f;
	playing = false;
	file_offset = 0;
	file_size = 0;
	loop_start = 0;
	loop_end = 0;
	do_loop = false;
	buffer_read_offset = -1;

}

bool AudioPlaySdRaw::play(const char *filename, float ratio, uint32_t loopStart, uint32_t loopEnd)
{
	setRatio(ratio);
	do_loop = true;
	loop_start = loopStart;
	loop_end = loopEnd;
	return play(filename);
}

bool AudioPlaySdRaw::setRatio(float ratio) {
	read_ratio = ratio;
	if(read_ratio > 4.0) read_ratio = 4;
	if(read_ratio < 0.01) read_ratio = 0.01;
	return true;
}

bool AudioPlaySdRaw::loop(bool looping) {
	do_loop = looping;
	return true;
}

bool AudioPlaySdRaw::play(const char *filename, float ratio)
{
	setRatio(ratio);
	return play(filename);
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


int AudioPlaySdRaw::buffered_read(void *buf, uint16_t nbyte) {
	// this is not as hard as I am making it. 
	// you call buffered_read and it first pull the data out of a 512 byte buffer is there is any
	// if you ask for more bytes i then read from the card, 512 at a time
	// i keep doing that until we are out of bytes to read or the card ends


	uint8_t* dst = reinterpret_cast<uint8_t*>(buf);
	int bytes_read_total = 0;
	int bytes_left = nbyte; 
	// How many bytes to read from the buffer (first pass)
	if(buffer_read_offset >= 0) { // is there a buffer?
		
		bytes_read_total = 512 - buffer_read_offset;
		if(bytes_left < bytes_read_total) bytes_read_total = bytes_left; 
		memcpy(dst, buffer + buffer_read_offset, bytes_read_total);
		bytes_left = bytes_left - bytes_read_total;
		buffer_read_offset = buffer_read_offset + bytes_read_total;
		if(buffer_read_offset == 512) buffer_read_offset = -1; // next time we have to read from SD

    }
	while(bytes_left > 0) {
	    // Only go in here if we need to read from the card
		int bytes_read_from_sd = rawfile.read(buffer, 512);
		buffer_read_offset = 0;
		if(bytes_left <= bytes_read_from_sd) {
			// we have everything we need here
			memcpy(dst + bytes_read_total, buffer, bytes_left);
			buffer_read_offset = bytes_left; 
			if(buffer_read_offset == 512) buffer_read_offset = -1;
			bytes_read_total += bytes_left;
			bytes_left = 0; // we are all done, exit the loop
		} else {
			memcpy(dst + bytes_read_total, buffer, bytes_read_from_sd);
			buffer_read_offset = -1;
			bytes_left = bytes_left - bytes_read_from_sd;
			bytes_read_total += bytes_read_from_sd;
		}
		if(bytes_read_from_sd != 512) { // end of file
			bytes_left = 0; // end
		}
	}
	return bytes_read_total;
}


void AudioPlaySdRaw::update(void)
{
	unsigned int i, j, n=0;

	audio_block_t *block;
	// only update if we're playing
	if (!playing) return;

	// allocate the audio blocks to transmit
	block = allocate();
	if (block == NULL) return;

	if (rawfile.available()) {
		// Clear out the block
		for(i=0;i<AUDIO_BLOCK_SAMPLES;i++) block->data[i] = 0;
		
		// No matter what we have to return AUDIO_BLOCK_SAMPLES worth of int16 data at 44KHz
		// If ratio is 0.5, we read only AUDIO_BLOCK_SAMPLES/2 samples from SD (2x bytes)
		// If ratio is 2.0 we read AUDIO_BLOCK_SAMPLES*2 samples (2x bytes)
		uint32_t samples_to_read = AUDIO_BLOCK_SAMPLES*read_ratio;

		// First, let's figure out where we are and see if we'll need to seek if we're looping
		if(do_loop) {
			uint32_t samples_to_read_left_in_loop = ((loop_end*2)-file_offset)/2;
			if (samples_to_read > samples_to_read_left_in_loop) {
				// We need to read what's left and then seek and read the rest
				// just check first if samples_to_read_left_in_loop > 0 (rare case of the pointer being precisely at loops' end)
				if(samples_to_read_left_in_loop > 0) {
					n = buffered_read(temp_block, samples_to_read_left_in_loop * 2);
				}
				rawfile.seek(loop_start * 2);
				buffer_read_offset = -1; // reset the buffer after seek
				uint32_t left_bytes_read = buffered_read(temp_block + samples_to_read_left_in_loop, (samples_to_read - samples_to_read_left_in_loop) * 2);
				file_offset = (loop_start*2) + left_bytes_read;
				n = n + left_bytes_read;
			} else {
				// Just read it normally (TODO change flow)
				n = buffered_read(temp_block, samples_to_read*2); 
				file_offset += n;
			}
		} else { 
			n = buffered_read(temp_block, samples_to_read*2); 
			file_offset += n;
		}
		// Now interpolate the data into block->data
		uint16_t last_pos = 0, new_pos = 0;
		for(i=0;i<n/2;i++) {
			new_pos = (int)(((float)i/(float)samples_to_read) * AUDIO_BLOCK_SAMPLES);
			block->data[new_pos] = temp_block[i];
			for(j=last_pos+1;j<new_pos;j++) {
				block->data[j] =  block->data[last_pos];
			}
			last_pos = new_pos;
		}
		// Now do the bit from new_pos until the end		
		for(i=new_pos+1; i<AUDIO_BLOCK_SAMPLES; i++) {
			block->data[i] = block->data[new_pos];
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


void AudioPlaySdRaw::update2(void)
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
		//n = rawfile.read(block->data, AUDIO_BLOCK_SAMPLES*2);
		n = buffered_read(block->data, AUDIO_BLOCK_SAMPLES*2);
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


