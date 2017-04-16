/*
 * Waveshaper for Teensy 3.X audio
 *
 * Copyright (c) 2017 Damien Clarke, http://damienclarke.me
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "effect_waveshaper.h"

void AudioEffectWaveshaper::shape(int16_t* waveshape, int length)
{
  // length must be bigger than 1 and equal to a power of two + 1
  // anything else means we don't continue
  if(!waveshape || length < 2 || length > 32769 || ((length - 1) & (length - 2))) return;

  this->waveshape = waveshape;

  // set lerpshift to the number of bits to shift while interpolating
  // to cover the entire waveshape over a uint16_t input range
  int index = length - 1;
  lerpshift = 16;
  while (index >>= 1) --lerpshift;
}

void AudioEffectWaveshaper::update(void)
{
  if(!waveshape) return;

  audio_block_t *block;
  block = receiveWritable();
  if (!block) return;

  // performance testing...
  // unsigned long mcs = micros();

  uint16_t x, xa;
  int16_t i, ya, yb;
  for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
    // bring int16_t data into uint16_t range
    x = block->data[i] + 32768;
    // lerp waveshape (from http://coranac.com/tonc/text/fixed.htm)
    xa = x >> lerpshift;
    ya = waveshape[xa];
    yb = waveshape[xa + 1];
    block->data[i] = ya + ((yb - ya) * (x - (xa << lerpshift)) >> lerpshift);
  }

  // log performance test without compensating for rollover...
  // Serial.print("(micros)");
  // Serial.println(micros() - mcs);

  transmit(block);
  release(block);
}
