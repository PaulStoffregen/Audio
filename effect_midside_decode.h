/* Audio Library for Teensy 3.X
 * Copyright (c) 2015, Hedde Bosman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef effect_midside_decode_h_
#define effect_midside_decode_h_

#include "AudioStream.h"

#include "utility/dspinst.h"

// computes limit(val, 2**bits)
static inline int16_t saturate16(int32_t val) __attribute__((always_inline, unused));
static inline int16_t saturate16(int32_t val)
{
	int16_t out;
	int32_t tmp;
	asm volatile("ssat %0, %1, %2" : "=r" (tmp) : "I" (16), "r" (val) );
	out = (int16_t) (tmp & 0xffff); // not sure if the & 0xffff is necessary. test.
	return out;
}
// saturating add: signed_add_16_and_16()

// saturating subtract:
static inline int32_t signed_subtract_16_and_16(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline int32_t signed_subtract_16_and_16(int32_t a, int32_t b)
{
	int32_t out;
	asm volatile("qsub16 %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

/**
 * This object performs the decoding of mid/side signals into a stereo signal.
 * That is, left = (mid+side), right = (mid-side)
 * After encoding and processing, this object decodes the mid/side components 
 * back into enjoyable stereo audio.
 * Caution: processed mid/side signals may cause digital saturation (clipping).
 ***************************************************************/
class AudioEffectMidSideDecode : public AudioStream
{
public:
  AudioEffectMidSideDecode(void): AudioStream(2,inputQueueArray) { }
  virtual void update(void);
  
private:
  audio_block_t *inputQueueArray[2];
};

#endif

