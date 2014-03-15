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

#ifndef synth_waveform_h_
#define synth_waveform_h_

#include "AudioStream.h"
#include "arm_math.h"


#define AUDIO_SAMPLE_RATE_ROUNDED (44118)

#define DELAY_PASSTHRU -1

#define TONE_TYPE_SINE     0
#define TONE_TYPE_SAWTOOTH 1
#define TONE_TYPE_SQUARE   2
#define TONE_TYPE_TRIANGLE 3

class AudioSynthWaveform : 
public AudioStream
{
public:
  AudioSynthWaveform(void) : 
  AudioStream(0,NULL), 
  tone_freq(0), tone_phase(0), tone_incr(0), tone_type(0),
  ramp_down(0), ramp_up(0), ramp_length(0)
  { 
  }
  
  void frequency(int t_hi)
  {
    tone_incr = (0x80000000LL*t_hi)/AUDIO_SAMPLE_RATE_EXACT;
  }
  
  // If ramp_length is non-zero this will set up
  // either a rmap up or a ramp down when a wave
  // first starts or when the amplitude is set
  // back to zero.
  // Note that if the ramp_length is N, the generated
  // wave will be N samples longer than when it is not
  // ramp
  void amplitude(float n) {        // 0 to 1.0
    if (n < 0) n = 0;
    else if (n > 1.0) n = 1.0;
    // Ramp code
    if(tone_amp && (n == 0)) {
      ramp_down = ramp_length;
      ramp_up = 0;
      last_tone_amp = tone_amp;
    }
    else if((tone_amp == 0) && n) {
      ramp_up = ramp_length;
      ramp_down = 0;
      // reset the phase when the amplitude was zero
      // and has now been increased. Note that this
      // happens even if the wave is not ramped
      // so that the signal starts at zero
      tone_phase = 0;
    }
    // set new magnitude
    tone_amp = n * 32767.0;
  }
  
  boolean begin(float t_amp,int t_hi,short t_type);
  virtual void update(void);
  void set_ramp_length(int16_t r_length);
  
private:
  short    tone_amp;
  short    last_tone_amp;
  short    tone_freq;
  uint32_t tone_phase;
  // volatile prevents the compiler optimizing out the frequency function
  volatile uint32_t tone_incr;
  short    tone_type;

  uint32_t ramp_down;
  uint32_t ramp_up;
  uint16_t ramp_length;
};
#endif
