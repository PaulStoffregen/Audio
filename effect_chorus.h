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
