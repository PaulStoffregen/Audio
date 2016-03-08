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

#include "effect_chorus.h"

/******************************************************************/

//                A u d i o E f f e c t C h o r u s
// Written by Pete (El Supremo) Jan 2014
// 140529 - change to handle mono stream - change modify() to voices()
// 140219 - correct storage class (not static)

boolean AudioEffectChorus::begin(short *delayline,int d_length,int n_chorus)
{
#if 0
Serial.print("AudioEffectChorus.begin(Chorus delay line length = ");
Serial.print(d_length);
Serial.print(", n_chorus = ");
Serial.print(n_chorus);
Serial.println(")");
#endif

  l_delayline = NULL;
  delay_length = 0;
  l_circ_idx = 0;

  if(delayline == NULL) {
    return(false);
  }
  if(d_length < 10) {
    return(false);
  }
  if(n_chorus < 1) {
    return(false);
  }
  
  l_delayline = delayline;
  delay_length = d_length/2;
  num_chorus = n_chorus;
 
  return(true);
}

void AudioEffectChorus::voices(int n_chorus)
{
  num_chorus = n_chorus;
}

//int last_idx = 0;
void AudioEffectChorus::update(void)
{
  audio_block_t *block;
  short *bp;
  int sum;
  int c_idx;

  if(l_delayline == NULL)return;
  
  // do passthru
  // It stores the unmodified data in the delay line so that
  // it isn't as likely to click
  if(num_chorus <= 1) {
    // Just passthrough
    block = receiveWritable(0);
    if(block) {
      bp = block->data;
      for(int i = 0;i < AUDIO_BLOCK_SAMPLES;i++) {
        l_circ_idx++;
        if(l_circ_idx >= delay_length) {
          l_circ_idx = 0;
        }
        l_delayline[l_circ_idx] = *bp++;
      }
      transmit(block,0);
      release(block);
    }
    return;
  }

  //          L E F T  C H A N N E L

  block = receiveWritable(0);
  if(block) {
    bp = block->data;
    uint32_t tmp = delay_length/(num_chorus - 1) - 1;
    for(int i = 0;i < AUDIO_BLOCK_SAMPLES;i++) {
      l_circ_idx++;
      if(l_circ_idx >= delay_length) {
        l_circ_idx = 0;
      }
      l_delayline[l_circ_idx] = *bp;
      sum = 0;
      c_idx = l_circ_idx;
      for(int k = 0; k < num_chorus; k++) {
        sum += l_delayline[c_idx];
        if(num_chorus > 1)c_idx -= tmp;
        if(c_idx < 0) {
          c_idx += delay_length;
        }
      }
      *bp++ = sum/num_chorus;
    }

    // transmit the block
    transmit(block,0);
    release(block);
  }
}



