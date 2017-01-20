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

#ifndef play_sd_raw_h_
#define play_sd_raw_h_

#include "Arduino.h"
#include "AudioStream.h"
#include "SD.h"

class AudioPlaySdRaw : public AudioStream
{
public:
	AudioPlaySdRaw(void) : AudioStream(0, NULL) { begin(); }
	void begin(void);
	bool play(const char *filename);
	bool play(const char *filename, float ratio);
    bool play(const char *filename, float ratio, uint32_t loopStart, uint32_t loopEnd);
    int buffered_read(void *buf, uint16_t nbyte);
	bool setRatio(float ratio);
	bool loop(bool looping);
	void stop(void);
	bool isPlaying(void) { return playing; }
	uint32_t positionMillis(void);
	uint32_t lengthMillis(void);
	virtual void update(void);
		virtual void update2(void);

private:
	// TODO temp_block needs to be at most ... X 
	int16_t temp_block[AUDIO_BLOCK_SAMPLES*5];
	uint32_t loop_start, loop_end;
	bool do_loop;
	float read_ratio;
	File rawfile;
	uint32_t file_size;
	volatile uint32_t file_offset;
	int buffer_read_offset;
	uint8_t buffer[512];
	volatile bool playing;
};

#endif
