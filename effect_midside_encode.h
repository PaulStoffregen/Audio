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

#ifndef effect_midside_encode_h_
#define effect_midside_encode_h_

#include "AudioStream.h"

//#include "utility/dspinst.h"
// additional dspinst:
// computes out = (((a[31:16]+b[31:16])/2) <<16) | ((a[15:0]+b[15:0])/2)
static inline int32_t signed_halving_add_16_and_16(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline int32_t signed_halving_add_16_and_16(int32_t a, int32_t b)
{
	int32_t out;
	asm volatile("shadd16 %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// additional dspinst:
// computes out = (((a[31:16]-b[31:16])/2) <<16) | ((a[15:0]-b[15:0])/2)
static inline int32_t signed_halving_subtract_16_and_16(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline int32_t signed_halving_subtract_16_and_16(int32_t a, int32_t b)
{
	int32_t out;
	asm volatile("shsub16 %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}




/**
 * This object performs the encoding of a stereo signal to its mid/side components.
 * That is, mid = (left+right)/2, side = (left-right)/2
 * Such encoding, together with the decoding, enables stereo field modifications 
 * (widening, narrowing) and, for instance, the application of filters or dynamic 
 * processing on the mid and 'side' components of the audio signal.
 * (e.g. cut the bass on the 'side' component for your woofer's pleasure)
 ***************************************************************/
class AudioEffectMidSideEncode : public AudioStream
{
public:
  AudioEffectMidSideEncode(void): AudioStream(2,inputQueueArray) { }
  virtual void update(void);
  
private:
  audio_block_t *inputQueueArray[2];
};

#endif

