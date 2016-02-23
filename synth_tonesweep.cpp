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

#include "synth_tonesweep.h"
#include "arm_math.h"




/******************************************************************/

//                A u d i o S y n t h T o n e S w e e p
// Written by Pete (El Supremo) Feb 2014


boolean AudioSynthToneSweep::play(float t_amp,int t_lo,int t_hi,float t_time)
{
  float tone_tmp;
  
if(0) {
  Serial.print("AudioSynthToneSweep.begin(tone_amp = ");
  Serial.print(t_amp);
  Serial.print(", tone_lo = ");
  Serial.print(t_lo);
  Serial.print(", tone_hi = ");
  Serial.print(t_hi);
  Serial.print(", tone_time = ");
  Serial.print(t_time,1);
  Serial.println(")");
}
  tone_amp = 0;
  if(t_amp < 0)return false;
  if(t_amp > 1)return false;
  if(t_lo < 1)return false;
  if(t_hi < 1)return false;
  if(t_hi >= (int) AUDIO_SAMPLE_RATE_EXACT / 2)return false;
  if(t_lo >= (int) AUDIO_SAMPLE_RATE_EXACT / 2)return false;
  if(t_time < 0)return false;
  tone_lo = t_lo;
  tone_hi = t_hi;
  tone_phase = 0;

  tone_amp = t_amp * 32767.0;

  tone_freq = tone_lo*0x100000000LL;
  if (tone_hi >= tone_lo) {
    tone_tmp = tone_hi - tone_lo;
    tone_sign = 1;
  } else {
    tone_sign = -1;
    tone_tmp = tone_lo - tone_hi;
  }
  tone_tmp = tone_tmp / t_time / AUDIO_SAMPLE_RATE_EXACT;
  tone_incr = (tone_tmp * 0x100000000LL);
  sweep_busy = 1;
  return(true);
}



unsigned char AudioSynthToneSweep::isPlaying(void)
{
  return(sweep_busy);
}


void AudioSynthToneSweep::update(void)
{
  audio_block_t *block;
  short *bp;
  int i;
  
  if(!sweep_busy)return;

  //          L E F T  C H A N N E L  O N L Y
  block = allocate();
  if(block) {
    bp = block->data;
    uint32_t tmp  = tone_freq >> 32; 
    uint64_t tone_tmp = (0x400000000000LL * (int)(tmp&0x7fffffff)) / (int) AUDIO_SAMPLE_RATE_EXACT;
    // Generate the sweep
    for(i = 0;i < AUDIO_BLOCK_SAMPLES;i++) {
      *bp++ = (short)(( (short)(arm_sin_q31((uint32_t)((tone_phase >> 15)&0x7fffffff))>>16) *tone_amp) >> 16);

      tone_phase +=  tone_tmp;
      if(tone_phase & 0x800000000000LL)tone_phase &= 0x7fffffffffffLL;

      if(tone_sign > 0) {
        if(tmp > tone_hi) {
          sweep_busy = 0;
          break;
        }
        tone_freq += tone_incr;
      } else {
        if(tmp < tone_hi) {
          sweep_busy = 0;

          break;
        }
        tone_freq -= tone_incr;        
      }
    }
    while(i < AUDIO_BLOCK_SAMPLES) {
      *bp++ = 0;
      i++;
    }    
    // send the samples to the left channel
    transmit(block,0);
    release(block);
  }
}
