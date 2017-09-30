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

#include "effect_delay.h"

void AudioEffectDelay::update(void)
{
	audio_block_t *output;
	uint32_t head, tail, count, channel, index, prev, offset;
	const int16_t *src, *end;
	int16_t *dst;

	// grab incoming data and put it into the queue
	head = headindex;
	tail = tailindex;
	if (++head >= DELAY_QUEUE_SIZE) head = 0;
	if (head == tail) {
		if (queue[tail] != NULL) release(queue[tail]);
		if (++tail >= DELAY_QUEUE_SIZE) tail = 0;
	}
	queue[head] = receiveReadOnly();
	headindex = head;

	// testing only.... don't allow null pointers into the queue
	// instead, fill the empty times with blocks of zeros
	//if (queue[head] == NULL) {
	//	queue[head] = allocate();
	//	if (queue[head]) {
	//		dst = queue[head]->data;
	//		end = dst + AUDIO_BLOCK_SAMPLES;
	//		do {
	//			*dst++ = 0;
	//		} while (dst < end);
	//	} else {
	//		digitalWriteFast(2, HIGH);
	//		delayMicroseconds(5);
	//		digitalWriteFast(2, LOW);
	//	}
	//}

	// discard unneeded blocks from the queue
	if (head >= tail) {
		count = head - tail;
	} else {
		count = DELAY_QUEUE_SIZE + head - tail;
	}
	if (count > maxblocks) {
		count -= maxblocks;
		do {
			if (queue[tail] != NULL) {
				release(queue[tail]);
				queue[tail] = NULL;
			}
			if (++tail >= DELAY_QUEUE_SIZE) tail = 0;
		} while (--count > 0);
	}
	tailindex = tail;

	// transmit the delayed outputs using queue data
	for (channel = 0; channel < 8; channel++) {
		if (!(activemask & (1<<channel))) continue;
		index =  position[channel] / AUDIO_BLOCK_SAMPLES;
		offset = position[channel] % AUDIO_BLOCK_SAMPLES;
		if (head >= index) {
			index = head - index;
		} else {
			index = DELAY_QUEUE_SIZE + head - index;
		}
		if (offset == 0) {
			// delay falls on the block boundary
			if (queue[index]) {
				transmit(queue[index], channel);
			}
		} else {
			// delay requires grabbing data from 2 blocks
			output = allocate();
			if (!output) continue;
			dst = output->data;
			if (index > 0) {
				prev = index - 1;
			} else {
				prev = DELAY_QUEUE_SIZE-1;
			}
			if (queue[prev]) {
				end = queue[prev]->data + AUDIO_BLOCK_SAMPLES;
				src = end - offset;
				while (src < end) {
					*dst++ = *src++; // TODO: optimize
				}
			} else {
				end = dst + offset;
				while (dst < end) {
					*dst++ = 0;
				}
			}
			end = output->data + AUDIO_BLOCK_SAMPLES;
			if (queue[index]) {
				src = queue[index]->data;
				while (dst < end) {
					*dst++ = *src++; // TODO: optimize
				}
			} else {
				while (dst < end) {
					*dst++ = 0;
				}
			}
			transmit(output, channel);
			release(output);
		}
	}

}


