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
#include "extmem.h"
#include "effect_delay_ext.h"


void AudioEffectDelayExternal::update(void)
{
	audio_block_t *block;
	uint32_t channel, read_offset;

	// grab incoming data and put it into the memory
	block = receiveReadOnly();
	if (memory_type >= AUDIO_MEMORY_UNDEFINED
	 || !initialisationDone) 
	{
		// ignore input and do nothing if undefined memory type
		if (nullptr != block)
			release(block);
		return;
	}
	
	if (block) 
	{
		writeWrap(head_offset, AUDIO_BLOCK_SAMPLES, block->data);
		release(block);
	} 
	else 
	{
		// if no input, store zeros, so later playback will
		// not be random garbage previously stored in memory
		zero(head_offset, AUDIO_BLOCK_SAMPLES);
	}
	head_offset += AUDIO_BLOCK_SAMPLES;
	if (head_offset >= memory_length)
		head_offset -= memory_length;

	// transmit the delayed outputs
	for (channel = 0; channel < CHANNEL_COUNT; channel++) 
	{
		if (!(activemask & (1<<channel))) continue;
		block = allocate();
		if (!block) continue;
		
		// compute the delayed location where we read
		read_offset = head_offset - delay_length[channel];
		if (delay_length[channel] > head_offset) 
			read_offset += memory_length;
		
		// read in delayed samples, wrapping as needed
		readWrap(read_offset, AUDIO_BLOCK_SAMPLES, block->data); 
		
		// transmit the result, and relinquish 
		// ownership of the block
		transmit(block, channel);
		release(block);
	}
}

