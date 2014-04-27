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

#include "synth_waveform.h"
#include "arm_math.h"
#include "utility/dspinst.h"


/******************************************************************/
// PAH 140415 - change sin to use Paul's interpolation which is much
//				faster than arm's sin function
// PAH 140316 - fix calculation of sample (amplitude error)
// PAH 140314 - change t_hi from int to float
// PAH - add ramp-up and ramp-down to the onset of the wave
// the length is specified in samples

void AudioSynthWaveform::set_ramp_length(int16_t r_length)
{
  if(r_length < 0) {
    ramp_length = 0;
    return;
  }
  // Don't set the ramp length longer than about 4 milliseconds
  if(r_length > 44*4) {
    ramp_length = 44*4;
    return;
  }
  ramp_length = r_length;
}


boolean AudioSynthWaveform::begin(float t_amp,float t_hi,short type)
{
  tone_type = type;
  amplitude(t_amp);
  tone_freq = t_hi > 0.0;
  if(t_hi <= 0.0)return false;
  if(t_hi >= AUDIO_SAMPLE_RATE_EXACT/2)return false;
  tone_phase = 0;
  frequency(t_hi);
  if(0) {
    Serial.print("AudioSynthWaveform.begin(tone_amp = ");
    Serial.print(t_amp);
    Serial.print(", tone_hi = ");
    Serial.print(t_hi);
    Serial.print(", tone_incr = ");
    Serial.print(tone_incr,HEX);
    //  Serial.print(", tone_hi = ");
    //  Serial.print(t_hi);
    Serial.println(")");
  }
  return(true);
}

// PAH - 140313 fixed a problem with ramping
void AudioSynthWaveform::update(void)
{
  audio_block_t *block;
  short *bp;
  // temporary for ramp in sine
  uint32_t ramp_mag;
  int32_t val1, val2, val3;
  uint32_t index, scale;
  
  // temporaries for TRIANGLE
  uint32_t mag;
  short tmp_amp;
  
  if(tone_freq == 0)return;
  //          L E F T  C H A N N E L  O N L Y
  block = allocate();
  if(block) {
    bp = block->data;
    switch(tone_type) {
    case TONE_TYPE_SINE:
      for(int i = 0;i < AUDIO_BLOCK_SAMPLES;i++) {
      	// Calculate interpolated sin
		index = tone_phase >> 23;
		val1 = AudioWaveformSine[index];
		val2 = AudioWaveformSine[index+1];
		scale = (tone_phase >> 7) & 0xFFFF;
		val2 *= scale;
		val1 *= 0xFFFF - scale;
		val3 = (val1 + val2) >> 16;
        // The value of ramp_up is always initialized to RAMP_LENGTH and then is
        // decremented each time through here until it reaches zero.
        // The value of ramp_up is used to generate a Q15 fraction which varies
        // from [0 - 1), and multiplies this by the current sample
        if(ramp_up) {
          // ramp up to the new magnitude
          // ramp_mag is the Q15 representation of the fraction
          // Since ramp_up can't be zero, this cannot generate +1
          ramp_mag = ((ramp_length-ramp_up)<<15)/ramp_length;
          ramp_up--;
          // adjust tone_phase to Q15 format and then adjust the result
          // of the multiplication
      	  // calculate the sample
          tmp_amp = (short)((val3 * tone_amp) >> 15);
          *bp++ = (tmp_amp * ramp_mag)>>15;
        } 
        else if(ramp_down) {
          // ramp down to zero from the last magnitude
          // The value of ramp_down is always initialized to RAMP_LENGTH and then is
          // decremented each time through here until it reaches zero.
          // The value of ramp_down is used to generate a Q15 fraction which varies
          // from [0 - 1), and multiplies this by the current sample
          // avoid RAMP_LENGTH/RAMP_LENGTH because Q15 format
          // cannot represent +1
          ramp_mag = ((ramp_down - 1)<<15)/ramp_length;
          ramp_down--;
		  tmp_amp = (short)((val3 * last_tone_amp) >> 15);
          *bp++ = (tmp_amp * ramp_mag)>>15;
        } else {
		  *bp++ = (short)((val3 * tone_amp) >> 15);
        } 
        
        // phase and incr are both unsigned 32-bit fractions
        tone_phase += tone_incr;
        // If tone_phase has overflowed, truncate the top bit 
        if(tone_phase & 0x80000000)tone_phase &= 0x7fffffff;
      }
      break;
      
    case TONE_TYPE_SQUARE:
      for(int i = 0;i < AUDIO_BLOCK_SAMPLES;i++) {
        if(tone_phase & 0x40000000)*bp++ = -tone_amp;
        else *bp++ = tone_amp;
        // phase and incr are both unsigned 32-bit fractions
        tone_phase += tone_incr;
      }
      break;
      
    case TONE_TYPE_SAWTOOTH:
      for(int i = 0;i < AUDIO_BLOCK_SAMPLES;i++) {
        *bp++ = ((short)(tone_phase>>15)*tone_amp) >> 15;
        // phase and incr are both unsigned 32-bit fractions
        tone_phase += tone_incr;    
      }
      break;

    case TONE_TYPE_TRIANGLE:
      for(int i = 0;i < AUDIO_BLOCK_SAMPLES;i++) {
        if(tone_phase & 0x80000000) {
          // negative half-cycle
          tmp_amp = -tone_amp;
        } 
        else {
          // positive half-cycle
          tmp_amp = tone_amp;
        }
        mag = tone_phase << 2;
        // Determine which quadrant
        if(tone_phase & 0x40000000) {
          // negate the magnitude
          mag = ~mag + 1;
        }
        *bp++ = ((short)(mag>>17)*tmp_amp) >> 15;
        tone_phase += 2*tone_incr;
      }
      break;
    }
    // send the samples to the left channel
    transmit(block,0);
    release(block);
  }
}




