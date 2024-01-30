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

#include <Arduino.h>
#include "effect_fade.h"
#include "utility/dspinst.h"

extern "C" {
extern const int16_t fader_table[257];
};

void AudioEffectFade::update(void)
{
	audio_block_t *block;
	uint32_t pos;

	pos = position;
	if (pos == 0) {
		// output is silent
		block = receiveReadOnly();
		if (block) release(block);
		return;
	} else if (pos == MAX_FADE) {
		// output is 100%
		block = receiveReadOnly();
		if (!block) return;
		transmit(block);
		release(block);
		return;
	}
	
	block = receiveWritable();
	if (block)
	{
		uint32_t inc = rate;
		uint8_t dir = direction;
		for (uint32_t i=0; i < AUDIO_BLOCK_SAMPLES; i++) 
		{
			int32_t val1, val2, val, sample;
			uint32_t index = pos >> 24;
			val1 = fader_table[index];
			val2 = fader_table[index+1];
			uint32_t scale = (pos >> 8) & 0xFFFF;
			val2 *= scale;
			val1 *= 0x10000 - scale;
			val = (val1 + val2) >> 16;
			sample = block->data[i];
			sample = (sample * val) >> 15;
			block->data[i] = sample;
			if (dir > 0) {
				// output is increasing
				if (inc < MAX_FADE - pos) pos += inc;
				else pos = MAX_FADE;
			} else {
				// output is decreasing
				if (inc < pos) pos -= inc;
				else pos = 0;
			}
		}
		position = pos;
		transmit(block);
		release(block);
	}
	else // no audio, but must still change fader position!
	{
		int64_t newPos = (int64_t) rate * AUDIO_BLOCK_SAMPLES;
		if (direction <= 0) // change is downwards
			newPos = (int64_t) position - newPos;
		else
			newPos = (int64_t) position + newPos;
		
		if (newPos <= 0LL) 			position = 0;
		else if (newPos > MAX_FADE) position = MAX_FADE;
		else 						position = newPos;
	}
}

void AudioEffectFade::fadeBegin(uint32_t samples, uint8_t dir)
{
	__disable_irq();
	if (0 == samples) samples = 1; // avoid divide-by-zero error
	uint32_t newrate = MAX_FADE / samples; // worst case is 1: takes 27 hours...
	uint32_t pos = position;
	
	// fader sticks at ends, so ensure it runs if needed
	if (pos == 0 && dir > 0) 			  position = 1;
	else if (pos == MAX_FADE && dir <= 0) position = MAX_FADE - 1;
	
	rate = newrate;
	direction = dir;
	__enable_irq();
}



