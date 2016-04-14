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

#include "Arduino.h"
#include "AudioStream.h"
#include "arm_math.h"

// waveforms.c
extern "C" {
extern const int16_t AudioWaveformSine[257];
}

#define AUDIO_SAMPLE_RATE_ROUNDED (44118)

#define DELAY_PASSTHRU -1

#define WAVEFORM_SINE      0
#define WAVEFORM_SAWTOOTH  1
#define WAVEFORM_SQUARE    2
#define WAVEFORM_TRIANGLE  3
#define WAVEFORM_ARBITRARY 4
#define WAVEFORM_PULSE     5
#define WAVEFORM_SAWTOOTH_REVERSE 6
#define WAVEFORM_SAMPLE_HOLD 7

// todo: remove these...
#define TONE_TYPE_SINE     0
#define TONE_TYPE_SAWTOOTH 1
#define TONE_TYPE_SQUARE   2
#define TONE_TYPE_TRIANGLE 3

class AudioSynthWaveform : 
public AudioStream
{
public:
  AudioSynthWaveform(void) : 
  AudioStream(0,NULL), tone_amp(0), tone_freq(0),
  tone_phase(0), tone_width(0.25), tone_incr(0), tone_type(0),
  tone_offset(0), arbdata(NULL)
  { 
  }
  
  void frequency(float t_freq) {
    if (t_freq < 0.0) t_freq = 0.0;
    else if (t_freq > AUDIO_SAMPLE_RATE_EXACT / 2) t_freq = AUDIO_SAMPLE_RATE_EXACT / 2;
    tone_incr = (t_freq * (0x80000000LL/AUDIO_SAMPLE_RATE_EXACT)) + 0.5;
  }
  void phase(float angle) {
    if (angle < 0.0) angle = 0.0;
    else if (angle > 360.0) {
      angle = angle - 360.0;
      if (angle >= 360.0) return;
    }
    tone_phase = angle * (2147483648.0 / 360.0);
  }
  void amplitude(float n) {        // 0 to 1.0
    if (n < 0) n = 0;
    else if (n > 1.0) n = 1.0;
    if ((tone_amp == 0) && n) {
      // reset the phase when the amplitude was zero
      // and has now been increased.
      tone_phase = 0;
    }
    // set new magnitude
    tone_amp = n * 32767.0;
  }
  void offset(float n) {
    if (n < -1.0) n = -1.0;
    else if (n > 1.0) n = 1.0;
    tone_offset = n * 32767.0;
  }
  void pulseWidth(float n) {          // 0.0 to 1.0
    if (n < 0) n = 0;
    else if (n > 1.0) n = 1.0;
    tone_width = n * 0x7fffffffLL; 
    // pulse width is stored as the equivalent phase
  }
  void begin(short t_type) {
	tone_phase = 0;
	tone_type = t_type;
  }
  void begin(float t_amp, float t_freq, short t_type) {
	amplitude(t_amp);
	frequency(t_freq);
	begin(t_type);
  }
  void arbitraryWaveform(const int16_t *data, float maxFreq) {
	arbdata = data;
  }
  virtual void update(void);
  
private:
  short    tone_amp;
  short    tone_freq;
  uint32_t tone_phase;
  uint32_t tone_width;
  // sample for SAMPLE_HOLD
  short sample;
  // volatile prevents the compiler optimizing out the frequency function
  volatile uint32_t tone_incr;
  short    tone_type;
  int16_t  tone_offset;
  const int16_t *arbdata;
};



#endif
