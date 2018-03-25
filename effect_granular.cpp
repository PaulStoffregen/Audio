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

#include <Arduino.h>
#include "effect_granular.h"

void AudioEffectGranular::begin(int16_t *sample_bank_def, int16_t max_len_def)
{
	sample_req = true;
	length(max_len_def - 1);
	sample_bank = sample_bank_def;
}

void AudioEffectGranular::length(int16_t max_len_def)
{
	if (max_len_def < 100) {
		max_sample_len = 100;
		glitch_len = max_sample_len / 3;
	} else {
		max_sample_len = (max_len_def - 1);
		glitch_len = max_sample_len / 3;
	}
}


void AudioEffectGranular::freeze(int16_t activate, int16_t playpack_rate_def, int16_t freeze_length_def)
{
	if (activate==1) {
		grain_mode = 1;
	}
	if (activate==0) {
		grain_mode = 0;
	}
	rate(playpack_rate_def);
	if (freeze_length_def < 50) {
		freeze_len = 50;
	}
	if (freeze_length_def >= max_sample_len) {
		freeze_len = max_sample_len;
	}
	if (freeze_length_def >= 50 && freeze_length_def < max_sample_len) {
		freeze_len = freeze_length_def;
	}
}

void AudioEffectGranular::shift(int16_t activate, int16_t playpack_rate_def, int16_t grain_length_def)
{
	if (activate == 1) {
		grain_mode = 2;
	}
	if (activate == 0) {
		grain_mode = 3;
	}
	rate(playpack_rate_def);
	if (allow_len_change) {
		//  Serial.println("aL");
		length(grain_length_def);
	}
}


void AudioEffectGranular::rate(int16_t playpack_rate_def)
{
	playpack_rate = playpack_rate_def;
}


void AudioEffectGranular::update(void)
{
	audio_block_t *block;

	if (sample_bank == NULL) {
		block = receiveReadOnly(0);
		if (block) release(block);
		return;
	}

	block = receiveWritable(0);
	if (!block) return;

	if (grain_mode == 3) {
		//through
		/*
		for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
			write_head++;
			if (write_head >= max_sample_len) {
				write_head = 0;
			}
			sample_bank[write_head] = block->data[i];
		}
		*/
	}

	if (grain_mode == 0) {
		//capture audio but dosen't output
		//this needs to be happening at all times if you want to
		// use grain_mode = 1, the simple "freeze" sampler.
		for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
			write_head++;

			if (write_head >= max_sample_len) {
				write_head = 0;
			}
			sample_bank[write_head] = block->data[i];
		}
	}


	if (grain_mode == 1) {
		//when activated the last
		for (int j = 0; j < AUDIO_BLOCK_SAMPLES; j++) {
			if (playpack_rate >= 0) {
				accumulator += playpack_rate;
				read_head = accumulator >> 9;
			}
			if (read_head >= freeze_len) {
				accumulator = 0;
				read_head -= max_sample_len;
			}
			block->data[j] = sample_bank[read_head];
		}
	}

	if (grain_mode == 2) {
		//GLITCH SHIFT
		//basic granular synth thingy
		// the shorter the sample the max_sample_len the more tonal it is.
		// Longer it has more definition.  It's a bit roboty either way which
		// is obv great and good enough for noise music.

		for (int k = 0; k < AUDIO_BLOCK_SAMPLES; k++) {
			int16_t current_input = block->data[k];
			// only start recording when the audio is crossing zero to minimize pops

			// TODO: simplify this logic... and check sample_req first for efficiency
			if (current_input < 0 && prev_input >= 0) {
				zero_cross_down = true;
			} else {
				zero_cross_down = false;
			}

			prev_input = current_input;

			if (zero_cross_down && sample_req) {
				write_en = true;
			}
			if (write_en) {
				sample_req = false;
				allow_len_change = true; // Reduces noise by not allowing the
						// length to change after the sample has been
						// recored.  Kind of not too much though
				if (write_head >= glitch_len) {
					glitch_cross_len = glitch_len;
					write_head = 0;
					sample_loaded = true;
					write_en = false;
					allow_len_change = false;
				}
				sample_bank[write_head] = block->data[k];
				write_head++;
			}

			if (sample_loaded) {
				//move it to the middle third of the bank.
				//3 "seperate" banks are used
				float fade_len = 20.00;
				int16_t m2 = fade_len;

				for (int m = 0; m < 2; m++) {
					// I'm off by one somewhere? why is there a tick at the
					// beginning of this only when it's combined with the
					// fade out???? ooor am i osbserving that incorrectly
					// either wait it works enough
					sample_bank[m + glitch_len] = 0;
				}

				for (int m = 2; m < glitch_len-m2; m++) {
					sample_bank[m + glitch_len] = sample_bank[m];
				}

				for (int m = glitch_len-m2; m < glitch_len; m++) {
					// fade out the end. You can just make fadet=0
					// but it's a little too daleky
					float fadet = sample_bank[m] * (m2 / fade_len);
					sample_bank[m + glitch_len] = (int16_t)fadet;
					m2--;
				}
				sample_loaded = false;
				sample_req = true;
			}

			accumulator += playpack_rate;
			read_head = (accumulator >> 9);

			if (read_head >= glitch_len) {
				read_head -= glitch_len;
				accumulator = 0;

				for (int m = 0; m < glitch_len; m++) {
					sample_bank[m + (glitch_len*2)] = sample_bank[m+glitch_len];
					//  sample_bank[m + (glitch_len*2)] = (m%20)*1000;
				}
			}
			block->data[k] = sample_bank[read_head + (glitch_len*2)];
		}
	}
	transmit(block);
	release(block);
}




