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

#include "analyze_print.h"

#define STATE_IDLE          0  // doing nothing
#define STATE_WAIT_TRIGGER  1  // looking for trigger condition
#define STATE_DELAY         2  // waiting from trigger to print
#define STATE_PRINTING      3  // printing data

void AudioAnalyzePrint::update(void)
{
	audio_block_t *block;
	uint32_t offset = 0;
	uint32_t remain, n;

	block = receiveReadOnly();
	if (!block) return;

	while (offset < AUDIO_BLOCK_SAMPLES) {
		remain = AUDIO_BLOCK_SAMPLES - offset;
		switch (state) {
		  case STATE_WAIT_TRIGGER:
			// TODO: implement this....
			offset = AUDIO_BLOCK_SAMPLES;
			break;

		  case STATE_DELAY:
			//Serial.printf("STATE_DELAY, count = %u\n", count);
			if (remain < count) {
				count -= remain;
				offset = AUDIO_BLOCK_SAMPLES;
			} else {
				offset += count;
				count = print_length;
				state = STATE_PRINTING;
			}
			break;

		  case STATE_PRINTING:
			n = count;
			if (n > remain) n = remain;
			count -= n;
			while (n > 0) {
				Serial.println(block->data[offset++]);
				n--;
			}
			if (count == 0) state = STATE_IDLE;
			break;

		  default: // STATE_IDLE
			offset = AUDIO_BLOCK_SAMPLES;
			break;
		}
	}
	release(block);
}

void AudioAnalyzePrint::trigger(void)
{
	uint32_t n = delay_length;

	if (n > 0) {
		Serial.print("trigger ");
		if (myname) Serial.print(myname);
		Serial.print(", delay=");
		Serial.println(n);
		count = n;
		state = 2;
	} else {
		Serial.print("trigger ");
		if (myname) Serial.println(myname);
		count = print_length;
		state = 3;
	}
}

