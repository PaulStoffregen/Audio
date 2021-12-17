/* Wavefolder effect for Teensy Audio library
 *
 * Copyright (c) 2020, Mark Tillotson
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

#include "effect_wavefolder.h"

void AudioEffectWaveFolder::update()
{
  audio_block_t * blocka = receiveWritable (0);
  if (!blocka)
    return;
  audio_block_t * blockb = receiveReadOnly (1);
  if (!blockb)
  {
    release (blocka);
    return;
  }
  int16_t * pa = blocka->data ;
  int16_t * pb = blockb->data ;
  for (int i = 0 ; i < AUDIO_BLOCK_SAMPLES ; i++)
  {
    int32_t a12 = pa[i];
    int32_t b12 = pb[i];

    // scale upto 16 times input, so that can fold upto 16 times in each polarity
    int32_t s1 = (a12 * b12 + 0x400) >> 11 ;
    // if in a band where the sense needs to be reverse, detect this
    bool flip1 = ((s1 + 0x8000) >> 16) & 1 ;
    // reverse and truncate to 16 bits
    s1 = 0xFFFF & (flip1 ? ~s1 : +s1) ;

    pa[i] = s1;
  }
  transmit(blocka);
  release(blocka);
  release(blockb);
}

