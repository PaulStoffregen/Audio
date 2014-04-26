/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 * rheslip - modified Paul's mixer code to prduce a multiplier
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

#include "multiplier.h"

// Multiply two inputs
void AudioMultiplier2::update(void)
{
  audio_block_t *in, *out=NULL;
  unsigned int i;

  if (!out) {
    out = receiveWritable(0); // first input - we'll use it as the output too
    if (out) {
      in = receiveReadOnly(1); // second input
      if (in) {
        int16_t * input1 = (int16_t *)out->data;
        const int16_t * input2 = (int16_t *)in->data;
        for (i=0; i < AUDIO_BLOCK_SAMPLES; ++i) {
          int32_t tmp =*input1 * *input2++;
//PAH change the scale to 15
          *input1++= (int16_t)(tmp>>15);
        }
        release(in);
      }
    }
  } 
  if (out) {
    transmit(out);
    release(out);
  }
}



