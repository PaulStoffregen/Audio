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
#include "record_queue.h"
#include "utility/dspinst.h"


int AudioRecordQueue::available(void)
{
	uint32_t h, t;

	h = head;
	t = tail;
	if (h >= t) return h - t;
	return max_buffers + h - t;
}

void AudioRecordQueue::clear(void)
{
	uint32_t t;

	if (NULL != userblock) {
		release(userblock);
		userblock = NULL;
	}
	t = tail;
	while (t != head) {
		if (++t >= max_buffers) t = 0;
		if (SILENT_BLOCK_FLAG != queue[t]) // real block?
			release(queue[t]);
	}
	tail = t;
}

int16_t * AudioRecordQueue::readBuffer(void)
{
	uint32_t t;

	if (NULL == userblock) // no block passed to foreground yet
	{
		t = tail;
		if (t == head) return NULL;
		if (++t >= max_buffers) t = 0;
		userblock = queue[t];
		if (SILENT_BLOCK_FLAG == userblock) // got a NULL block, i.e. silence
		{
			userblock = allocate(); // allocate a block now, rather than one for every silent entry
			if (NULL != userblock)	// if we got one...
				memset(userblock->data,0,sizeof userblock->data); // ...set it to silent
		}
		tail = t;
	}
	// otherwise return data address previously sent, 
	// rather than NULL, which according to the 
	// documentation says no data available
	
	return userblock->data;
}

void AudioRecordQueue::freeBuffer(void)
{
	if (userblock == NULL) return;
	release(userblock);
	userblock = NULL;
}

void AudioRecordQueue::update(void)
{
	audio_block_t *block;
	uint32_t h;

	block = receiveReadOnly();
	//if (!block) return;
	if (!enabled) {
		if (NULL != block)
			release(block);
		return;
	}
		
	h = head + 1;
	if (h >= max_buffers) h = 0;
	if (h == tail) { // no room in queue
		if (NULL != block)
			release(block);
	} else {
		// No block supplied, but we are active: make a place-holder
		if (NULL == block)
			block = SILENT_BLOCK_FLAG;
		
		// queue the block or place-holder
		queue[h] = block;
		head = h;
	}
}


