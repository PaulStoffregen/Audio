/*
 * Copyright (c) 2018 John-Michael Reed
 * bleeplabs.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "AudioStream.h"

class AudioEffectGranular : public AudioStream
{
public:
	AudioEffectGranular(void): AudioStream(1,inputQueueArray) { }
	void begin(int16_t *sample_bank_def, int16_t max_len_def);
	void setSpeed(float ratio) {
		if (ratio < 0.125) ratio = 0.125;
		else if (ratio > 8.0) ratio = 8.0;
		playpack_rate = ratio * 65536.0 + 0.499;
	}
	void beginFreeze(float grain_length) {
		if (grain_length <= 0.0) return;
		beginFreeze_int(grain_length * (AUDIO_SAMPLE_RATE_EXACT * 0.001) + 0.5);
	}
	void beginPitchShift(float grain_length) {
		if (grain_length <= 0.0) return;
		beginPitchShift_int(grain_length * (AUDIO_SAMPLE_RATE_EXACT * 0.001) + 0.5);
	}
	void stop();
	virtual void update(void);
private:
	void beginFreeze_int(int grain_samples);
	void beginPitchShift_int(int grain_samples);
	audio_block_t *inputQueueArray[1];
	int16_t *sample_bank;
	uint32_t playpack_rate;
	uint32_t accumulator;
	int16_t max_sample_len;
	int16_t write_head;
	int16_t read_head;
	int16_t grain_mode;
	int16_t freeze_len;
	int16_t prev_input;
	int16_t glitch_len;
	bool allow_len_change;
	bool sample_loaded;
	bool write_en;
	bool sample_req;
};

