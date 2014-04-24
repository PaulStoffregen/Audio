/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Pete (El Supremo)
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

#include "filter_fir.h"

void AudioFilterFIR::begin(short *cp,int n_coeffs)
{
  // pointer to coefficients
  coeff_p = cp;
  // Initialize FIR instances for the left and right channels
  if(coeff_p && (coeff_p != FIR_PASSTHRU)) {
    arm_fir_init_q15(&l_fir_inst, n_coeffs, coeff_p, &l_StateQ15[0], AUDIO_BLOCK_SAMPLES);
  }
}

// This has the same effect as begin(NULL,0);
void AudioFilterFIR::stop(void)
{
  coeff_p = NULL;
}


void AudioFilterFIR::update(void)
{
  audio_block_t *block,*b_new;
  
  // If there's no coefficient table, give up.  
  if(coeff_p == NULL)return;

  // do passthru
  if(coeff_p == FIR_PASSTHRU) {
    // Just passthrough
    block = receiveReadOnly(0);
    if(block) {
      transmit(block,0);
      release(block);
    }
    return;
  }
  // Left Channel
  block = receiveReadOnly(0);
  // get a block for the FIR output
  b_new = allocate();
  if(block && b_new) {
    if(arm_fast)
    	arm_fir_fast_q15(&l_fir_inst, (q15_t *)block->data, (q15_t *)b_new->data, AUDIO_BLOCK_SAMPLES);
    else
    	arm_fir_q15(&l_fir_inst, (q15_t *)block->data, (q15_t *)b_new->data, AUDIO_BLOCK_SAMPLES); 
    // send the FIR output to the left channel
    transmit(b_new,0);
  }
  if(block)release(block);
  if(b_new)release(b_new);
}


