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

static inline uint32_t SearchMinMax16_DSP(int16_t* pSrc, int32_t pSize, uint32_t min, uint32_t max);

void AudioAnalyzePeak::update(void)
{
	audio_block_t *block;
	const int16_t *p, *end;
	int32_t min, max;

	block = receiveReadOnly();
	if (!block) {
		return;
	}
#if 1	
	p = block->data;
	end = p + AUDIO_BLOCK_SAMPLES;
	min = min_sample;
	max = max_sample;
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
	uint32_t r = SearchMinMax16_DSP( block->data, AUDIO_BLOCK_SAMPLES, min_sample, max_sample );
	max_sample = r >> 16;
  min_sample = (int16_t) (r & 0xffff);
#endif	
	new_output = true;
	release(block);
}

// yes, we can. 3.5 times faster!
// https://www.waybackmachine.org/web/20161111030547/http://www.m4-unleashed.com/parallel-comparison/
//uint32_t SearchMinMax16_DSP(int16_t* pSrc, int32_t pSize);
static inline uint32_t SearchMinMax16_DSP(int16_t* pSrc, int32_t pSize, uint32_t min, uint32_t max)
{
  int16_t data16;
#if 0
  // ( original version )
   uint32_t data, min, max;

  // max variable will hold two max : one on each 16-bits half
  // same thing for min
  

  // Load two first samples in one 32-bit access
  data = *__SIMD32(pSrc)++;
  // Initialize Min and Max to these first samples
  min = data; 
  max = data;
  // decrement sample count
  pSize -= 2;
#else
	uint32_t data;
#endif

  /* Loop as long as there remains at least two samples */
  while (pSize > 1)
  {
    /* Load next two samples in a single access */
    data = *__SIMD32(pSrc)++;
    /* Parallel comparison of max and new samples */
    (void)__SSUB16(max, data);
    /* Select max on each 16-bits half */
    max = __SEL(max, data);
    /* Parallel comparison of new samples and min */
    (void)__SSUB16(data, min);
    /* Select min on each 16-bits half */
    min = __SEL(min, data);

    pSize -= 2;
  }
  /* Now we have maximum on even samples on low halfword of max
     and maximum on odd samples on high halfword */
  /* look for max between halfwords 1 & 0 by comparing on low halfword */
  (void)__SSUB16(max, max >> 16);
  /* Select max on low 16-bits */
  max = __SEL(max, max >> 16);

  /* look for min between halfwords 1 & 0 by comparing on low halfword */
  (void)__SSUB16(min >> 16, min);
  /* Select min on low 16-bits */
  min = __SEL(min, min >> 16);

  /* Test if odd number of samples */
  if (pSize > 0)
  {
    data16 = *pSrc;
    /* look for max between on low halfwords */
    (void)__SSUB16(max, data16);
    /* Select max on low 16-bits */
    max = __SEL(max, data16);

    /* look for min on low halfword */
    (void)__SSUB16(data16, min);
    /* Select min on low 16-bits */
    min = __SEL(min, data16);
  }

  /* Pack result : Min on Low halfword, Max on High halfword */
  return __PKHBT(min, max, 16); /* PKHBT documentation */
}
