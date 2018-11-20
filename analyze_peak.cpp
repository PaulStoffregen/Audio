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
#include "analyze_peak.h"
#include "arm_math.h"

void AudioAnalyzePeak::update(void)
{
  audio_block_t *block;
  int32_t min, max;

  block = receiveReadOnly();
  if (!block) {
    return;
  }
  min = min_sample;
  max = max_sample;

#if defined(KINETISL)
  const int16_t *p, *end;
  p = block->data;
  end = p + AUDIO_BLOCK_SAMPLES;
  do {
    int16_t d=*p++;
    // TODO: can we speed this up with SSUB16 and SEL
    // http://www.m4-unleashed.com/parallel-comparison/
    if (d<min) min=d;
    if (d>max) max=d;
  } while (p < end);
  min_sample = min;
  max_sample = max;
#else
  // yes, we can. 3.5 times faster!
  // https://www.waybackmachine.org/web/20161111030547/http://www.m4-unleashed.com/parallel-comparison/
  const int32_t *p, *end;
  p = (int32_t*)block->data;
  end = p + AUDIO_BLOCK_SAMPLES/2;
  do {
    uint32_t data = *__SIMD32(p)++;// Load next two samples in a single access
    (void)__SSUB16(max, data);// Parallel comparison of max and new samples
    max = __SEL(max, data);// Select max on each 16-bits half
    (void)__SSUB16(data, min);// Parallel comparison of new samples and min
    min = __SEL(min, data);// Select min on each 16-bits half
  } while (p < end);
  // Now we have maximum on even samples on low halfword of max
  // and maximum on odd samples on high halfword
  // look for max between halfwords 1 & 0 by comparing on low halfword
  (void)__SSUB16(max, max >> 16);
  max_sample = __SEL(max, max >> 16);// Select max on low 16-bits
  (void)__SSUB16(min >> 16, min);// look for min between halfwords 1 & 0 by comparing on low halfword
  min_sample = __SEL(min, min >> 16);// Select min on low 16-bits
#endif
  new_output = true;
  release(block);
}
