#ifndef filter_fir_h_
#define filter_fir_h_

#include "AudioStream.h"

// Maximum number of coefficients in a FIR filter
// The audio breaks up with 128 coefficients so a
// maximum of 150 is more than sufficient
#define MAX_COEFFS 150

// Indicates that the code should just pass through the audio
// without any filtering (as opposed to doing nothing at all)
#define FIR_PASSTHRU ((short *) 1)

class AudioFilterFIR : 
public AudioStream
{
public:
  AudioFilterFIR(void): 
  AudioStream(2,inputQueueArray), coeff_p(NULL)
  { 
  }

  void begin(short *coeff_p,int f_pin);
  virtual void update(void);
  void stop(void);
  
private:
  audio_block_t *inputQueueArray[2];
  // arm state arrays and FIR instances for left and right channels
  // the state arrays are defined to handle a maximum of MAX_COEFFS
  // coefficients in a filter
  q15_t l_StateQ15[AUDIO_BLOCK_SAMPLES + MAX_COEFFS];
  q15_t r_StateQ15[AUDIO_BLOCK_SAMPLES + MAX_COEFFS];
  arm_fir_instance_q15 l_fir_inst;
  arm_fir_instance_q15 r_fir_inst;
  // pointer to current coefficients or NULL or FIR_PASSTHRU
  short *coeff_p;
};

#endif
