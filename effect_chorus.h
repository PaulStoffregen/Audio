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

#ifndef effect_chorus_h_
#define effect_chorus_h_

#include "AudioStream.h"

/******************************************************************/

//                A u d i o E f f e c t C h o r u s
// Written by Pete (El Supremo) Jan 2014

class AudioEffectChorus : 
public AudioStream
{
public:
  AudioEffectChorus(void): 
  AudioStream(2,inputQueueArray) { 
  }

  boolean begin(short *delayline,int delay_length,int n_chorus);
  virtual void update(void);
  void stop(void);
  void modify(int n_chorus);
  
private:
  audio_block_t *inputQueueArray[2];
  static short *l_delayline;
  static short *r_delayline;
  static short l_circ_idx;
  static short r_circ_idx;
  static int num_chorus;
  static int delay_length;
};

#endif
