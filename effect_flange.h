#ifndef effect_flange_h_
#define effect_flange_h_

#include "AudioStream.h"

/******************************************************************/
//                A u d i o E f f e c t F l a n g e
// Written by Pete (El Supremo) Jan 2014

#define DELAY_PASSTHRU -1

class AudioEffectFlange : 
public AudioStream
{
public:
  AudioEffectFlange(void): 
  AudioStream(2,inputQueueArray) { 
  }

  boolean begin(short *delayline,int d_length,int delay_offset,int d_depth,float delay_rate);
  boolean modify(int delay_offset,int d_depth,float delay_rate);
  virtual void update(void);
  void stop(void);
  
private:
  audio_block_t *inputQueueArray[2];
  static short *l_delayline;
  static short *r_delayline;
  static int delay_length;
  static short l_circ_idx;
  static short r_circ_idx;
  static int delay_depth;
  static int delay_offset_idx;
  static int   delay_rate_incr;
  static unsigned int l_delay_rate_index;
  static unsigned int r_delay_rate_index;
};

#endif
