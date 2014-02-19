#include "filter_fir.h"

void AudioFilterFIR::begin(short *cp,int n_coeffs)
{
  // pointer to coefficients
  coeff_p = cp;
  // Initialize FIR instances for the left and right channels
  if(coeff_p && (coeff_p != FIR_PASSTHRU)) {
    arm_fir_init_q15(&l_fir_inst, n_coeffs, coeff_p, &l_StateQ15[0], AUDIO_BLOCK_SAMPLES);
    arm_fir_init_q15(&r_fir_inst, n_coeffs, coeff_p, &r_StateQ15[0], AUDIO_BLOCK_SAMPLES);
  }
}

// This has the same effect as begin(NULL,0);
void AudioFilterFIR::stop(void)
{
  coeff_p = NULL;
}


void AudioFilterFIR::update(void)
{
  audio_block_t *block,*b_new;
  
  // If there's no coefficient table, give up.  
  if(coeff_p == NULL)return;

  // do passthru
  if(coeff_p == FIR_PASSTHRU) {
    // Just passthrough
    block = receiveWritable(0);
    if(block) {
      transmit(block,0);
      release(block);
    }
    block = receiveWritable(1);
    if(block) {
      transmit(block,1);
      release(block);
    }
    return;
  }
  // Left Channel
  block = receiveWritable(0);
  // get a block for the FIR output
  b_new = allocate();
  if(block && b_new) {
    arm_fir_q15(&l_fir_inst, (q15_t *)block->data, (q15_t *)b_new->data, AUDIO_BLOCK_SAMPLES);
    // send the FIR output to the left channel
    transmit(b_new,0);
  }
  if(block)release(block);
  if(b_new)release(b_new);

  // Right Channel
  block = receiveWritable(1);
  b_new = allocate();
  if(block && b_new) {
    arm_fir_q15(&r_fir_inst, (q15_t *)block->data, (q15_t *)b_new->data, AUDIO_BLOCK_SAMPLES);
    transmit(b_new,1);
  }
  if(block)release(block);
  if(b_new)release(b_new);
}


