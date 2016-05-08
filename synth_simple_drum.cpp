/* Audio Library for Teensy 3.X
 * Copyright (c) 2016, Byron Jacquot, SparkFun Electronics
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

#include "synth_simple_drum.h"

extern "C" {
extern const int16_t AudioWaveformSine[257];
}

void AudioSynthSimpleDrum::noteOn(void)
{
  __disable_irq();

  wav_phasor = 0;
  wav_phasor2 = 0;

  env_lin_current = 0x7fff0000;
  
  __enable_irq();
}

void AudioSynthSimpleDrum::secondMix(float level)
{
  // As level goes from 0.0 to 1.0,
  // second goes from 0 to 1/2 scale
  // first goes from full scale to half scale. 

  if(level < 0)
  {
    level = 0;
  }
  else if(level > 1.0)
  {
    level = 1.0;
  }

  __disable_irq();
  wav_amplitude2 = level * 0x3fff;
  wav_amplitude1 = 0x7fff - wav_amplitude2;
  __enable_irq();
}


void AudioSynthSimpleDrum::pitchMod(float depth)
{
  int32_t intdepth, calc;

  // Validate parameter
  if(depth < 0)
  {
    depth = 0;
  }
  else if(depth > 1.0)
  {
    depth = 1.0;
  }

  // Depth is float, 0.0..1.0
  // turn 0.0 to 1.0 into
  // 0x0 to 0x3fff;
  intdepth = depth * 0x7fff;

  // Lets turn it into 2.14, in range between -0.75 and 2.9999, woth 0 at 0.5
  // It becomes the scalar for the modulation component of the phasor increment.
  if(intdepth < 0x4000)
  {
    // 0 to 0.5 becomes
    // -0x3000 (0xffffCfff) to 0 ()
    calc = ((0x4000 - intdepth) * 0x3000 )>> 14;
    calc = -calc;
  }
  else
  {
    // 0.5 to 1.0 becomes
    // 0x00 to 0xbfa0
    calc = ((intdepth - 0x4000) * 0xc000)>> 14;
  }

  // Call result 2.14 format (max of ~3.99...approx 4)
  // See note in update().
  wav_pitch_mod = calc;
}



void AudioSynthSimpleDrum::update(void)
{
  audio_block_t *block_wav;
  int16_t *p_wave, *end;
  int32_t sin_l, sin_r, interp, mod, mod2, delta;
  int32_t interp2;
  int32_t index, scale;
  bool do_second;

  int32_t env_sqr_current; // the square of the linear value - inexpensive quasi exponential decay.

  block_wav = allocate();
  if (!block_wav) return;
  p_wave = (block_wav->data);
  end = p_wave + AUDIO_BLOCK_SAMPLES;

  // 50 is arbitrary threshold...
  // low values of second are inaudible, and we can save CPU cycles
  // by not calculating second when it's really quiet.
  do_second = (wav_amplitude2 > 50);

  while(p_wave < end)
  {
    // Do envelope first
    if(env_lin_current < 0x0000ffff)
    {
      // If envelope has expired, then stuff zeros into output buffer.
      *p_wave = 0;
      p_wave++; 
    }
    else
    {
      env_lin_current -= env_decrement;
      env_sqr_current = multiply_16tx16t(env_lin_current, env_lin_current) ;

      // do wave second;
      wav_phasor  += wav_increment;

      // modulation changes how we use the increment
      // the increment will be scaled by the modulation amount.
      //
      // Pitch mod is in range [-0.75 .. 3.99999] in 2.14 format
      // Current envelope value gets scaled by mod depth.
      // Then phasor increment gets scaled by that.
      mod = signed_multiply_32x16b((env_sqr_current), (wav_pitch_mod>>1)) >> 13;      
      mod2 = signed_multiply_32x16b(wav_increment<<3, mod>>1);

      wav_phasor += (mod2);
      wav_phasor &= 0x7fffffff;

      if(do_second)
      {
        // A perfect fifth uses increment of 1.5 times regular increment
        wav_phasor2 += wav_increment;
        wav_phasor2 += (wav_increment >> 1);
        wav_phasor2 += mod2;
        wav_phasor2 += (mod2 >> 1);
        wav_phasor2 &= 0x7fffffff;
      }
    
      // Phase to Sine lookup * interp:
      index = wav_phasor >> 23; // take top valid 8 bits
      sin_l = AudioWaveformSine[index];
      sin_r = AudioWaveformSine[index+1];

      // The fraction of the phasor in time we are between L and R
      // is the same as the fraction of the ampliture of that interval we should add 
      // to L.
      delta = sin_r-sin_l;
      scale = (wav_phasor >> 7) & 0xfFFF;
      delta = (delta * scale)>> 16;
      interp = sin_l + delta;
      
      if(do_second)
      {
        index = wav_phasor2 >> 23; // take top valid 8 bits
        sin_l = AudioWaveformSine[index];
        sin_r = AudioWaveformSine[index+1];

        delta = sin_r-sin_l;
        scale = (wav_phasor2 >> 7) & 0xFFFF;
        delta = (delta * scale)>> 16;
        interp2 = sin_l + delta;

        // Then scale and add the two waves
        interp2 = (interp2 * wav_amplitude2 ) >> 15;
        interp = (interp * wav_amplitude1) >> 15;
        interp = interp + interp2;
      }

      *p_wave = signed_multiply_32x16b(env_sqr_current, interp ) >> 15 ;
      p_wave++; 
    }
  }

  transmit(block_wav, 0);
  release(block_wav);

}

