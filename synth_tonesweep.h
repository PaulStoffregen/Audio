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

#ifndef synth_tonesweep_h_
#define synth_tonesweep_h_

#include "Arduino.h"
#include "AudioStream.h"

//                A u d i o S y n t h T o n e S w e e p
// Written by Pete (El Supremo) Feb 2014

class AudioSynthToneSweep : public AudioStream
{
public:
  AudioSynthToneSweep(void) :
  AudioStream(0,NULL), sweep_busy(0)
  { }

  boolean play(float t_amp,int t_lo,int t_hi,float t_time);
  virtual void update(void);
  unsigned char isPlaying(void);

private:
  short tone_amp;
  unsigned int tone_lo;
  unsigned int tone_hi;
  uint64_t tone_freq;
  uint64_t tone_phase;
  uint64_t tone_incr;
  int tone_sign;
  unsigned char sweep_busy;
};

#endif
