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

#ifndef effect_flange_h_
#define effect_flange_h_

#include "Arduino.h"
#include "AudioStream.h"

/******************************************************************/
//                A u d i o E f f e c t F l a n g e
// Written by Pete (El Supremo) Jan 2014
// 140529 - change to handle mono stream and change modify() to voices()

#define FLANGE_DELAY_PASSTHRU 0

class AudioEffectFlange : 
public AudioStream
{
public:
  AudioEffectFlange(void): 
  AudioStream(1,inputQueueArray) { 
  }

  boolean begin(short *delayline,int d_length,int delay_offset,int d_depth,float delay_rate);
  boolean voices(int delay_offset,int d_depth,float delay_rate);
  virtual void update(void);
  
private:
  audio_block_t *inputQueueArray[1];
  short *l_delayline;
  int delay_length;
  short l_circ_idx;
  int delay_depth;
  int delay_offset_idx;
  int   delay_rate_incr;
  unsigned int l_delay_rate_index;
};

#endif
