#ifndef synth_tonesweep_h_
#define synth_tonesweep_h_

#include "AudioStream.h"

//                A u d i o T o n e S w e e p
// Written by Pete (El Supremo) Feb 2014

class AudioToneSweep : public AudioStream
{
public:
  AudioToneSweep(void) : 
  AudioStream(0,NULL), sweep_busy(0)
  { }

  boolean begin(short t_amp,int t_lo,int t_hi,float t_time);
  virtual void update(void);
  unsigned char busy(void);

private:
  short tone_amp;
  int tone_lo;
  int tone_hi;
  uint64_t tone_freq;
  uint64_t tone_phase;
  uint64_t tone_incr;
  int tone_sign;
  unsigned char sweep_busy;
};

#endif
