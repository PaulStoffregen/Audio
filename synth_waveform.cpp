#include "Audio.h"
#include "arm_math.h"
#include "utility/dspinst.h"




#ifdef ORIGINAL_AUDIOSYNTHWAVEFORM
/******************************************************************/
// PAH - add ramp-up and ramp-down to the onset of the wave
// the length is specified in samples
void AudioSynthWaveform::set_ramp_length(uint16_t r_length)
{
	if(r_length < 0) {
		ramp_length = 0;
		return;
	}
	// Don't set the ramp length longer than about 4 milliseconds
	if(r_length > 44*4) {
		ramp_length = 44*4;
		return;
	}
	ramp_length = r_length;
}

void AudioSynthWaveform::update(void)
{
	audio_block_t *block;
	uint32_t i, ph, inc, index, scale;
	int32_t val1, val2, val3;

	//Serial.println("AudioSynthWaveform::update");
	if (((magnitude > 0) || ramp_down) && (block = allocate()) != NULL) {
		ph = phase;
		inc = phase_increment;
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			index = ph >> 24;
			val1 = wavetable[index];
			val2 = wavetable[index+1];
			scale = (ph >> 8) & 0xFFFF;
			val2 *= scale;
			val1 *= 0xFFFF - scale;
			val3 = (val1 + val2) >> 16;


// The value of ramp_up is always initialized to RAMP_LENGTH and then is
// decremented each time through here until it reaches zero.
// The value of ramp_up is used to generate a Q15 fraction which varies
// from [0 - 1), and multiplies this by the current sample
			if(ramp_up) {
				// ramp up to the new magnitude
				// ramp_mag is the Q15 representation of the fraction
				// Since ramp_up can't be zero, this cannot generate +1
				ramp_mag = ((ramp_length-ramp_up)<<15)/ramp_length;
				ramp_up--;
				block->data[i] = (val3 * ((ramp_mag * magnitude)>>15)) >> 15;

			} else if(ramp_down) {
				// ramp down to zero from the last magnitude
// The value of ramp_down is always initialized to RAMP_LENGTH and then is
// decremented each time through here until it reaches zero.
// The value of ramp_down is used to generate a Q15 fraction which varies
// from (1 - 0], and multiplies this by the current sample
				// avoid RAMP_LENGTH/RAMP_LENGTH because Q15 format
				// cannot represent +1
				ramp_mag = ((ramp_down - 1)<<15)/ramp_length;
				ramp_down--;
				block->data[i] = (val3 * ((ramp_mag * last_magnitude)>>15)) >> 15;
			} else {			
				block->data[i] = (val3 * magnitude) >> 15;
			}

			 //Serial.print(block->data[i]);
			 //Serial.print(", ");
			 //if ((i % 12) == 11) Serial.println();
			ph += inc;
		}
		 //Serial.println();
		phase = ph;
		transmit(block);
		release(block);
	} else {
		// is this numerical overflow ok?
		phase += phase_increment * AUDIO_BLOCK_SAMPLES;
	}
}
#else
/******************************************************************/
// PAH - add ramp-up and ramp-down to the onset of the wave
// the length is specified in samples
void AudioSynthWaveform::set_ramp_length(uint16_t r_length)
{
  if(r_length < 0) {
    ramp_length = 0;
    return;
  }
  // Don't set the ramp length longer than about 4 milliseconds
  if(r_length > 44*4) {
    ramp_length = 44*4;
    return;
  }
  ramp_length = r_length;
}

boolean AudioSynthWaveform::begin(float t_amp,int t_hi,short type)
{
  tone_type = type;
//  tone_amp = t_amp;
  amplitude(t_amp);
  tone_freq = t_hi;
  if(t_hi < 1)return false;
  if(t_hi >= AUDIO_SAMPLE_RATE_EXACT/2)return false;
  tone_phase = 0;
  tone_incr = (0x100000000LL*t_hi)/AUDIO_SAMPLE_RATE_EXACT;
  if(0) {
    Serial.print("AudioSynthWaveform.begin(tone_amp = ");
    Serial.print(t_amp);
    Serial.print(", tone_hi = ");
    Serial.print(t_hi);
    Serial.print(", tone_incr = ");
    Serial.print(tone_incr,HEX);
    //  Serial.print(", tone_hi = ");
    //  Serial.print(t_hi);
    Serial.println(")");
  }
  return(true);
}


void AudioSynthWaveform::update(void)
{
  audio_block_t *block;
  short *bp;
  // temporary for ramp in sine
  uint32_t ramp_mag;
  // temporaries for TRIANGLE
  uint32_t mag;
  short tmp_amp;

  
  if(tone_freq == 0)return;
  //          L E F T  C H A N N E L  O N L Y
  block = allocate();
  if(block) {
    bp = block->data;
    switch(tone_type) {
    case TONE_TYPE_SINE:
      for(int i = 0;i < AUDIO_BLOCK_SAMPLES;i++) {
        // The value of ramp_up is always initialized to RAMP_LENGTH and then is
        // decremented each time through here until it reaches zero.
        // The value of ramp_up is used to generate a Q15 fraction which varies
        // from [0 - 1), and multiplies this by the current sample
        if(ramp_up) {
          // ramp up to the new magnitude
          // ramp_mag is the Q15 representation of the fraction
          // Since ramp_up can't be zero, this cannot generate +1
          ramp_mag = ((ramp_length-ramp_up)<<15)/ramp_length;
          ramp_up--;
          // adjust tone_phase to Q15 format and then adjust the result
          // of the multiplication
          *bp = (short)((arm_sin_q15(tone_phase>>17) * tone_amp) >> 15);
          *bp++ = (*bp * ramp_mag)>>15;
        } 
        else if(ramp_down) {
          // ramp down to zero from the last magnitude
          // The value of ramp_down is always initialized to RAMP_LENGTH and then is
          // decremented each time through here until it reaches zero.
          // The value of ramp_down is used to generate a Q15 fraction which varies
          // from (1 - 0], and multiplies this by the current sample
          // avoid RAMP_LENGTH/RAMP_LENGTH because Q15 format
          // cannot represent +1
          ramp_mag = ((ramp_down - 1)<<15)/ramp_length;
          ramp_down--;
          // adjust tone_phase to Q15 format and then adjust the result
          // of the multiplication
          *bp = (short)((arm_sin_q15(tone_phase>>17) * last_tone_amp) >> 15);
          *bp++ = (*bp * ramp_mag)>>15;
        } else {
          // adjust tone_phase to Q15 format and then adjust the result
          // of the multiplication
          *bp++ = (short)((arm_sin_q15(tone_phase>>17) * tone_amp) >> 15);
        } 
        
        // phase and incr are both unsigned 32-bit fractions
        tone_phase += tone_incr;
      }
      break;
      
    case TONE_TYPE_SQUARE:
      for(int i = 0;i < AUDIO_BLOCK_SAMPLES;i++) {
        if(tone_phase & 0x80000000)*bp++ = -tone_amp;
        else *bp++ = tone_amp;
        // phase and incr are both unsigned 32-bit fractions
        tone_phase += tone_incr;
      }
      break;
      
    case TONE_TYPE_SAWTOOTH:
      for(int i = 0;i < AUDIO_BLOCK_SAMPLES;i++) {
        *bp = ((short)(tone_phase>>16)*tone_amp) >> 15;
        bp++;
        // phase and incr are both unsigned 32-bit fractions
        tone_phase += tone_incr;
      }
      break;

    case TONE_TYPE_TRIANGLE:
      for(int i = 0;i < AUDIO_BLOCK_SAMPLES;i++) {
        if(tone_phase & 0x80000000) {
          // negative half-cycle
          tmp_amp = -tone_amp;
        } 
        else {
          // positive half-cycle
          tmp_amp = tone_amp;
        }
        mag = tone_phase << 2;
        // Determine which quadrant
        if(tone_phase & 0x40000000) {
          // negate the magnitude
          mag = ~mag + 1;
        }
        *bp++ = ((short)(mag>>17)*tmp_amp) >> 15;
        tone_phase += tone_incr;
      }
      break;
    }
    // send the samples to the left channel
    transmit(block,0);
    release(block);
  }
}


#endif








#if 0
void AudioSineWaveMod::frequency(float f)
{
	if (f > AUDIO_SAMPLE_RATE_EXACT / 2 || f < 0.0) return;
	phase_increment = (f / AUDIO_SAMPLE_RATE_EXACT) * 4294967296.0f;
}

void AudioSineWaveMod::update(void)
{
	audio_block_t *block, *modinput;
	uint32_t i, ph, inc, index, scale;
	int32_t val1, val2;

	//Serial.println("AudioSineWave::update");
	modinput = receiveReadOnly();
	ph = phase;
	inc = phase_increment;
	block = allocate();
	if (!block) {
		// unable to allocate memory, so we'll send nothing
		if (modinput) {
			// but if we got modulation data, update the phase
			for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
				ph += inc + modinput->data[i] * modulation_factor;
			}
			release(modinput);
		} else {
			ph += phase_increment * AUDIO_BLOCK_SAMPLES;
		}
		phase = ph;
		return;
	}
	if (modinput) {
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			index = ph >> 24;
			val1 = sine_table[index];
			val2 = sine_table[index+1];
			scale = (ph >> 8) & 0xFFFF;
			val2 *= scale;
			val1 *= 0xFFFF - scale;
			block->data[i] = (val1 + val2) >> 16;
			 //Serial.print(block->data[i]);
			 //Serial.print(", ");
			 //if ((i % 12) == 11) Serial.println();
			ph += inc + modinput->data[i] * modulation_factor;
		}
		release(modinput);
	} else {
		ph = phase;
		inc = phase_increment;
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			index = ph >> 24;
			val1 = sine_table[index];
			val2 = sine_table[index+1];
			scale = (ph >> 8) & 0xFFFF;
			val2 *= scale;
			val1 *= 0xFFFF - scale;
			block->data[i] = (val1 + val2) >> 16;
			 //Serial.print(block->data[i]);
			 //Serial.print(", ");
			 //if ((i % 12) == 11) Serial.println();
			ph += inc;
		}
	}
	phase = ph;
	transmit(block);
	release(block);
}
#endif





