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

#include "effect_envelope.h"


void AudioEffectEnvelope::noteOn(void)
{
	__disable_irq();
	mult = 0;
	count = delay_count;
	if (count > 0) {
		state = STATE_DELAY;
		inc = 0;
	} else {
		state = STATE_ATTACK;
		count = attack_count;
		inc = (0x10000 / count) >> 3;
	}
	__enable_irq();
}

void AudioEffectEnvelope::noteOff(void)
{
	__disable_irq();
	state = STATE_RELEASE;
	count = release_count;
	mult = sustain_mult;
	inc = (-mult / ((int32_t)count << 3));
	__enable_irq();
}

void AudioEffectEnvelope::update(void)
{
	audio_block_t *block;
	uint32_t *p, *end;
	uint32_t sample12, sample34, sample56, sample78, tmp1, tmp2;

	block = receiveWritable();
	if (!block) return;
	if (state == STATE_IDLE) {
		release(block);
		return;
	}
	p = (uint32_t *)(block->data);
	end = p + AUDIO_BLOCK_SAMPLES/2;

	while (p < end) {
		// we only care about the state when completing a region
		if (count == 0) {
			if (state == STATE_ATTACK) {
				count = hold_count;
				if (count > 0) {
					state = STATE_HOLD;
					mult = 0x10000;
					inc = 0;
				} else {
					count = decay_count;
					state = STATE_DECAY;
					inc = ((sustain_mult - 0x10000) / ((int32_t)count << 3));
				}
				continue;
			} else if (state == STATE_HOLD) {
				state = STATE_DECAY;
				count = decay_count;
				inc = ((sustain_mult - 0x10000) / (int32_t)count) >> 3;
				continue;
			} else if (state == STATE_DECAY) {
			  if (sustain_mult) {
          state = STATE_SUSTAIN;
          count = 0xFFFF;
          mult = sustain_mult;
          inc = 0;
				} else {
				  state = STATE_RELEASE;
				  continue;
				}
			} else if (state == STATE_SUSTAIN) {
				count = 0xFFFF;
			} else if (state == STATE_RELEASE) {
				state = STATE_IDLE;
				while (p < end) {
					*p++ = 0;
					*p++ = 0;
					*p++ = 0;
					*p++ = 0;
				}
				break;
			} else if (state == STATE_DELAY) {
				state = STATE_ATTACK;
				count = attack_count;
				inc = (0x10000 / count) >> 3;
				continue;
			}
		}
		// process 8 samples, using only mult and inc
		sample12 = *p++;
		sample34 = *p++;
		sample56 = *p++;
		sample78 = *p++;
		p -= 4;
		mult += inc;
		tmp1 = signed_multiply_32x16b(mult, sample12);
		mult += inc;
		tmp2 = signed_multiply_32x16t(mult, sample12);
		sample12 = pack_16b_16b(tmp2, tmp1);
		mult += inc;
		tmp1 = signed_multiply_32x16b(mult, sample34);
		mult += inc;
		tmp2 = signed_multiply_32x16t(mult, sample34);
		sample34 = pack_16b_16b(tmp2, tmp1);
		mult += inc;
		tmp1 = signed_multiply_32x16b(mult, sample56);
		mult += inc;
		tmp2 = signed_multiply_32x16t(mult, sample56);
		sample56 = pack_16b_16b(tmp2, tmp1);
		mult += inc;
		tmp1 = signed_multiply_32x16b(mult, sample78);
		mult += inc;
		tmp2 = signed_multiply_32x16t(mult, sample78);
		sample78 = pack_16b_16b(tmp2, tmp1);
		*p++ = sample12;
		*p++ = sample34;
		*p++ = sample56;
		*p++ = sample78;
		count--;
	}
	transmit(block);
	release(block);
}

