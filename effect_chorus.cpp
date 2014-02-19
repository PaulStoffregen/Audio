#include "Audio.h"
#include "arm_math.h"
#include "utility/dspinst.h"


/******************************************************************/

//                A u d i o E f f e c t C h o r u s
// Written by Pete (El Supremo) Jan 2014

// circular addressing indices for left and right channels
short AudioEffectChorus::l_circ_idx;
short AudioEffectChorus::r_circ_idx;

short * AudioEffectChorus::l_delayline = NULL;
short * AudioEffectChorus::r_delayline = NULL;
int AudioEffectChorus::delay_length;
// An initial value of zero indicates passthru
int AudioEffectChorus::num_chorus = 0;


// All three must be valid.
boolean AudioEffectChorus::begin(short *delayline,int d_length,int n_chorus)
{
Serial.print("AudioEffectChorus.begin(Chorus delay line length = ");
Serial.print(d_length);
Serial.print(", n_chorus = ");
Serial.print(n_chorus);
Serial.println(")");

l_delayline = NULL;
r_delayline = NULL;
delay_length = 0;
l_circ_idx = 0;
r_circ_idx = 0;

  if(delayline == NULL) {
    return(false);
  }
  if(d_length < 10) {
    return(false);
  }
  if(n_chorus < 1) {
    return(false);
  }
  
  l_delayline = delayline;
  r_delayline = delayline + d_length/2;
  delay_length = d_length/2;
  num_chorus = n_chorus;
 
  return(true);
}

// This has the same effect as begin(NULL,0);
void AudioEffectChorus::stop(void)
{

}

void AudioEffectChorus::modify(int n_chorus)
{
  num_chorus = n_chorus;
}

int iabs(int x)
{
  if(x < 0)return(-x);
  return(x);
}
//static int d_count = 0;

int last_idx = 0;
void AudioEffectChorus::update(void)
{
  audio_block_t *block;
  short *bp;
  int sum;
  int c_idx;

  if(l_delayline == NULL)return;
  if(r_delayline == NULL)return;  
  
  // do passthru
  // It stores the unmodified data in the delay line so that
  // it isn't as likely to click
  if(num_chorus < 1) {
    // Just passthrough
    block = receiveWritable(0);
    if(block) {
      bp = block->data;
      for(int i = 0;i < AUDIO_BLOCK_SAMPLES;i++) {
        l_circ_idx++;
        if(l_circ_idx >= delay_length) {
          l_circ_idx = 0;
        }
        l_delayline[l_circ_idx] = *bp++;
      }
      transmit(block,0);
      release(block);
    }
    block = receiveWritable(1);
    if(block) {
      bp = block->data;
      for(int i = 0;i < AUDIO_BLOCK_SAMPLES;i++) {
        r_circ_idx++;
        if(r_circ_idx >= delay_length) {
          r_circ_idx = 0;
        }
        r_delayline[r_circ_idx] = *bp++;
      }
      transmit(block,1);
      release(block);
    }
    return;
  }

  //          L E F T  C H A N N E L

  block = receiveWritable(0);
  if(block) {
    bp = block->data;
    for(int i = 0;i < AUDIO_BLOCK_SAMPLES;i++) {
      l_circ_idx++;
      if(l_circ_idx >= delay_length) {
        l_circ_idx = 0;
      }
      l_delayline[l_circ_idx] = *bp;
      sum = 0;
      c_idx = l_circ_idx;
      for(int k = 0; k < num_chorus; k++) {
        sum += l_delayline[c_idx];
        if(num_chorus > 1)c_idx -= delay_length/(num_chorus - 1) - 1;
        if(c_idx < 0) {
          c_idx += delay_length;
        }
      }
      *bp++ = sum/num_chorus;
    }

    // send the effect output to the left channel
    transmit(block,0);
    release(block);
  }

  //          R I G H T  C H A N N E L

  block = receiveWritable(1);
  if(block) {
    bp = block->data;
    for(int i = 0;i < AUDIO_BLOCK_SAMPLES;i++) {
      r_circ_idx++;
      if(r_circ_idx >= delay_length) {
        r_circ_idx = 0;
      }
      r_delayline[r_circ_idx] = *bp;
      sum = 0;
      c_idx = r_circ_idx;
      for(int k = 0; k < num_chorus; k++) {
        sum += r_delayline[c_idx];
        if(num_chorus > 1)c_idx -= delay_length/(num_chorus - 1) - 1;
        if(c_idx < 0) {
          c_idx += delay_length;
        }
      }
      *bp++ = sum/num_chorus;
    }

    // send the effect output to the left channel
    transmit(block,1);
    release(block);
  }
}



