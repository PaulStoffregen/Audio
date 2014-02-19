#include "Audio.h"
#include "arm_math.h"
#include "utility/dspinst.h"



static arm_cfft_radix4_instance_q15 fft_inst;

void AudioAnalyzeFFT256::init(void)
{
	// TODO: replace this with static const version
	arm_cfft_radix4_init_q15(&fft_inst, 256, 0, 1);

	//for (int i=0; i<2048; i++) {
		//buffer[i] = i * 3;
	//}
	//__disable_irq();
	//ARM_DEMCR |= ARM_DEMCR_TRCENA;
	//ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
	//uint32_t n = ARM_DWT_CYCCNT;
	//arm_cfft_radix2_q15(&fft_inst, buffer);
	//n = ARM_DWT_CYCCNT - n;
	//__enable_irq();
	//cycles = n;
	//arm_cmplx_mag_q15(buffer, buffer, 512);

	// each audio block is 278525 cycles @ 96 MHz
	// 256 point fft2 takes 65408 cycles
	// 256 point fft4 takes 49108 cycles
	// 128 point cmag takes 10999 cycles
	// 1024 point fft2 takes 125948 cycles
	// 1024 point fft4 takes 125840 cycles
	// 512 point cmag takes 43764 cycles

	//state = 0;
	//outputflag = false;
}

static void copy_to_fft_buffer(void *destination, const void *source)
{
	const int16_t *src = (const int16_t *)source;
	int16_t *dst = (int16_t *)destination;

	// TODO: optimize this
	for (int i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
		*dst++ = *src++;  // real
		*dst++ = 0;       // imaginary
	}
}

static void apply_window_to_fft_buffer(void *buffer, const void *window)
{
	int16_t *buf = (int16_t *)buffer;
	const int16_t *win = (int16_t *)window;;

	for (int i=0; i < 256; i++) {
		int32_t val = *buf * *win++;
		//*buf = signed_saturate_rshift(val, 16, 15);
		*buf = val >> 15;
		buf += 2;
	}

}

void AudioAnalyzeFFT256::update(void)
{
	audio_block_t *block;

	block = receiveReadOnly();
	if (!block) return;
	if (!prevblock) {
		prevblock = block;
		return;
	}
	copy_to_fft_buffer(buffer, prevblock->data);
	copy_to_fft_buffer(buffer+256, block->data);
	//window = AudioWindowBlackmanNuttall256;
	//window = NULL;
	if (window) apply_window_to_fft_buffer(buffer, window);
	arm_cfft_radix4_q15(&fft_inst, buffer);
	// TODO: is this averaging correct?  G. Heinzel's paper says we're
	// supposed to average the magnitude squared, then do the square
	// root at the end after dividing by naverage.
	arm_cmplx_mag_q15(buffer, buffer, 128);
	if (count == 0) {
		for (int i=0; i < 128; i++) {
			output[i] = buffer[i];
		}
	} else {
		for (int i=0; i < 128; i++) {
			output[i] += buffer[i];
		}
	}
	if (++count == naverage) {
		count = 0;
		for (int i=0; i < 128; i++) {
			output[i] /= naverage;
		}
		outputflag = true;
	}

	release(prevblock);
	prevblock = block;

#if 0
	block = receiveReadOnly();
	if (state == 0) {
		//Serial.print("0");
		if (block == NULL) return;
		copy_to_fft_buffer(buffer, block->data);
		state = 1;
	} else if (state == 1) {
		//Serial.print("1");
		if (block == NULL) return;
		copy_to_fft_buffer(buffer+256, block->data);
		arm_cfft_radix4_q15(&fft_inst, buffer);
		state = 2;
	} else {
		//Serial.print("2");
		arm_cmplx_mag_q15(buffer, output, 128);
		outputflag = true;
		if (block == NULL) return;
		copy_to_fft_buffer(buffer, block->data);
		state = 1;
	}
	release(block);
#endif
}



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






/******************************************************************/


void AudioPrint::update(void)
{
	audio_block_t *block;
	uint32_t i;

	Serial.println("AudioPrint::update");
	Serial.println(name);
	block = receiveReadOnly();
	if (block) {
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			Serial.print(block->data[i]);
			Serial.print(", ");
			if ((i % 12) == 11) Serial.println();
		}
		Serial.println();
		release(block);
	}
}



/******************************************************************/


#define STATE_DIRECT_8BIT_MONO		0  // playing mono at native sample rate
#define STATE_DIRECT_8BIT_STEREO	1  // playing stereo at native sample rate
#define STATE_DIRECT_16BIT_MONO		2  // playing mono at native sample rate
#define STATE_DIRECT_16BIT_STEREO	3  // playing stereo at native sample rate
#define STATE_CONVERT_8BIT_MONO		4  // playing mono, converting sample rate
#define STATE_CONVERT_8BIT_STEREO	5  // playing stereo, converting sample rate
#define STATE_CONVERT_16BIT_MONO	6  // playing mono, converting sample rate
#define STATE_CONVERT_16BIT_STEREO	7  // playing stereo, converting sample rate
#define STATE_PARSE1			8  // looking for 20 byte ID header
#define STATE_PARSE2			9  // looking for 16 byte format header
#define STATE_PARSE3			10 // looking for 8 byte data header
#define STATE_PARSE4			11 // ignoring unknown chunk
#define STATE_STOP			12

void AudioPlaySDcardWAV::begin(void)
{
	state = STATE_STOP;
	state_play = STATE_STOP;
	data_length = 0;
	if (block_left) {
		release(block_left);
		block_left = NULL;
	}
	if (block_right) {
		release(block_right);
		block_right = NULL;
	}
}


bool AudioPlaySDcardWAV::play(const char *filename)
{
	stop();
	wavfile = SD.open(filename);
	if (!wavfile) return false;
	buffer_remaining = 0;
	state_play = STATE_STOP;
	data_length = 0;
	state = STATE_PARSE1;
	return true;
}

void AudioPlaySDcardWAV::stop(void)
{
	__disable_irq();
	if (state != STATE_STOP) {
		audio_block_t *b1 = block_left;
		block_left = NULL;
		audio_block_t *b2 = block_right;
		block_right = NULL;
		state = STATE_STOP;
		__enable_irq();
		if (b1) release(b1);
		if (b2) release(b2);
		wavfile.close();
	} else {
		__enable_irq();
	}
}

bool AudioPlaySDcardWAV::start(void)
{
	__disable_irq();
	if (state == STATE_STOP) {
		if (state_play == STATE_STOP) {
			__enable_irq();
			return false;
		}
		state = state_play;
	}
	__enable_irq();
	return true;
}


void AudioPlaySDcardWAV::update(void)
{
	// only update if we're playing
	if (state == STATE_STOP) return;

	// allocate the audio blocks to transmit
	block_left = allocate();
	if (block_left == NULL) return;
	if (state < 8 && (state & 1) == 1) {
		// if we're playing stereo, allocate another
		// block for the right channel output
		block_right = allocate();
		if (block_right == NULL) {
			release(block_left);
			return;
		}
	} else {
		// if we're playing mono or just parsing
		// the WAV file header, no right-side block
		block_right = NULL;
	}
	block_offset = 0;

	//Serial.println("update");

	// is there buffered data?
	if (buffer_remaining > 0) {
		// we have buffered data
		if (consume()) return; // it was enough to transmit audio
	}

	// we only get to this point when buffer[512] is empty
	if (state != STATE_STOP && wavfile.available()) {
		// we can read more data from the file...
		buffer_remaining = wavfile.read(buffer, 512);
		if (consume()) {
			// good, it resulted in audio transmit
			return;
		} else {
			// not good, no audio was transmitted
			buffer_remaining = 0;
			if (block_left) {
				release(block_left);
				block_left = NULL;
			}
			if (block_right) {
				release(block_right);
				block_right = NULL;
			}
			// if we're still playing, well, there's going to
			// be a gap in output, but we can't keep burning
			// time trying to read more data.  Hopefully things
			// will go better next time?
			if (state != STATE_STOP) return;
		}
	}
	// end of file reached or other reason to stop
	wavfile.close();
	if (block_left) {
		release(block_left);
		block_left = NULL;
	}
	if (block_right) {
		release(block_right);
		block_right = NULL;
	}
	state_play = STATE_STOP;
	state = STATE_STOP;
}


// https://ccrma.stanford.edu/courses/422/projects/WaveFormat/

// Consume already buffered data.  Returns true if audio transmitted.
bool AudioPlaySDcardWAV::consume(void)
{
	uint32_t len, size;
	uint8_t lsb, msb;
	const uint8_t *p;

	size = buffer_remaining;
	p = buffer + 512 - size;
start:
	if (size == 0) return false;
	//Serial.print("AudioPlaySDcardWAV write, size = ");
	//Serial.print(size);
	//Serial.print(", data_length = ");
	//Serial.print(data_length);
	//Serial.print(", state = ");
	//Serial.println(state);
	switch (state) {
	  // parse wav file header, is this really a .wav file?
	  case STATE_PARSE1:
		len = 20 - data_length;
		if (size < len) len = size;
		memcpy((uint8_t *)header + data_length, p, len);
		data_length += len;
		if (data_length < 20) return false;
		// parse the header...
		if (header[0] == 0x46464952 && header[2] == 0x45564157
		  && header[3] == 0x20746D66 && header[4] == 16) {
			//Serial.println("header ok");
			state = STATE_PARSE2;
			p += len;
			size -= len;
			data_length = 0;
			goto start;
		}
		//Serial.println("unknown WAV header");
		break;

	  // check & extract key audio parameters
	  case STATE_PARSE2:
		len = 16 - data_length;
		if (size < len) len = size;
		memcpy((uint8_t *)header + data_length, p, len);
		data_length += len;
		if (data_length < 16) return false;
		if (parse_format()) {
			//Serial.println("audio format ok");
			p += len;
			size -= len;
			data_length = 0;
			state = STATE_PARSE3;
			goto start;
		}
		//Serial.println("unknown audio format");
		break;

	  // find the data chunk
	  case STATE_PARSE3:
		len = 8 - data_length;
		if (size < len) len = size;
		memcpy((uint8_t *)header + data_length, p, len);
		data_length += len;
		if (data_length < 8) return false;
		//Serial.print("chunk id = ");
		//Serial.print(header[0], HEX);
		//Serial.print(", length = ");
		//Serial.println(header[1]);
		p += len;
		size -= len;
		data_length = header[1];
		if (header[0] == 0x61746164) {
			//Serial.println("found data chunk");
			// TODO: verify offset in file is an even number
			// as required by WAV format.  abort if odd.  Code
			// below will depend upon this and fail if not even.
			leftover_bytes = 0;
			state = state_play;
			if (state & 1) {
				// if we're going to start stereo
				// better allocate another output block
				block_right = allocate();
				if (!block_right) return false;
			}
		} else {
			state = STATE_PARSE4;
		}
		goto start;

	  // ignore any extra unknown chunks (title & artist info)
	  case STATE_PARSE4:
		if (size < data_length) {
			data_length -= size;
			return false;
		}
		p += data_length;
		size -= data_length;
		data_length = 0;
		state = STATE_PARSE3;
		goto start;

	  // playing mono at native sample rate
	  case STATE_DIRECT_8BIT_MONO:
		return false;

	  // playing stereo at native sample rate
	  case STATE_DIRECT_8BIT_STEREO:
		return false;

	  // playing mono at native sample rate
	  case STATE_DIRECT_16BIT_MONO:
		if (size > data_length) size = data_length;
		data_length -= size;
		while (1) {
			lsb = *p++;
			msb = *p++;
			size -= 2;
			block_left->data[block_offset++] = (msb << 8) | lsb;
			if (block_offset >= AUDIO_BLOCK_SAMPLES) {
				transmit(block_left, 0);
				transmit(block_left, 1);
				 //Serial1.print('%');
				 //delayMicroseconds(90);
				release(block_left);
				block_left = NULL;
				data_length += size;
				buffer_remaining = size;
				if (block_right) release(block_right);
				return true;
			}
			if (size == 0) {
				if (data_length == 0) break;
				return false;
			}
		}
		// end of file reached
		if (block_offset > 0) {
			// TODO: fill remainder of last block with zero and transmit
		}
		state = STATE_STOP;
		return false;

	  // playing stereo at native sample rate
	  case STATE_DIRECT_16BIT_STEREO:
		if (size > data_length) size = data_length;
		data_length -= size;
		if (leftover_bytes) {
			block_left->data[block_offset] = header[0];
			goto right16;
		}
		while (1) {
			lsb = *p++;
			msb = *p++;
			size -= 2;
			if (size == 0) {
				if (data_length == 0) break;
				header[0] = (msb << 8) | lsb;
				leftover_bytes = 2;
				return false;
			}
			block_left->data[block_offset] = (msb << 8) | lsb;
			right16:
			lsb = *p++;
			msb = *p++;
			size -= 2;
			block_right->data[block_offset++] = (msb << 8) | lsb;
			if (block_offset >= AUDIO_BLOCK_SAMPLES) {
				transmit(block_left, 0);
				release(block_left);
				block_left = NULL;
				transmit(block_right, 1);
				release(block_right);
				block_right = NULL;
				data_length += size;
				buffer_remaining = size;
				return true;
			}
			if (size == 0) {
				if (data_length == 0) break;
				leftover_bytes = 0;
				return false;
			}
		}
		// end of file reached
		if (block_offset > 0) {
			// TODO: fill remainder of last block with zero and transmit
		}
		state = STATE_STOP;
		return false;

	  // playing mono, converting sample rate
	  case STATE_CONVERT_8BIT_MONO :
		return false;

	  // playing stereo, converting sample rate
	  case STATE_CONVERT_8BIT_STEREO:
		return false;

	  // playing mono, converting sample rate
	  case STATE_CONVERT_16BIT_MONO:
		return false;

	  // playing stereo, converting sample rate
	  case STATE_CONVERT_16BIT_STEREO:
		return false;

	  // ignore any extra data after playing
	  // or anything following any error
	  case STATE_STOP:
		return false;

	  // this is not supposed to happen!
	  //default:
		//Serial.println("AudioPlaySDcardWAV, unknown state");
	}
	state_play = STATE_STOP;
	state = STATE_STOP;
	return false;
}


/*
00000000  52494646 66EA6903 57415645 666D7420  RIFFf.i.WAVEfmt 
00000010  10000000 01000200 44AC0000 10B10200  ........D.......
00000020  04001000 4C495354 3A000000 494E464F  ....LIST:...INFO
00000030  494E414D 14000000 49205761 6E742054  INAM....I Want T
00000040  6F20436F 6D65204F 76657200 49415254  o Come Over.IART
00000050  12000000 4D656C69 73736120 45746865  ....Melissa Ethe
00000060  72696467 65006461 746100EA 69030100  ridge.data..i...
00000070  FEFF0300 FCFF0400 FDFF0200 0000FEFF  ................
00000080  0300FDFF 0200FFFF 00000100 FEFF0300  ................
00000090  FDFF0300 FDFF0200 FFFF0100 0000FFFF  ................
*/





// SD library on Teensy3 at 96 MHz
//  256 byte chunks, speed is 443272 bytes/sec
//  512 byte chunks, speed is 468023 bytes/sec





bool AudioPlaySDcardWAV::parse_format(void)
{
	uint8_t num = 0;
	uint16_t format;
	uint16_t channels;
	uint32_t rate;
	uint16_t bits;

	format = header[0];
	//Serial.print("  format = ");
	//Serial.println(format);
	if (format != 1) return false;

	channels = header[0] >> 16;
	//Serial.print("  channels = ");
	//Serial.println(channels);
	if (channels == 1) {
	} else if (channels == 2) {
		num = 1;
	} else {
		return false;
	}

	bits = header[3] >> 16;
	//Serial.print("  bits = ");
	//Serial.println(bits);
	if (bits == 8) {
	} else if (bits == 16) {
		num |= 2;
	} else {
		return false;
	}

	rate = header[1];
	//Serial.print("  rate = ");
	//Serial.println(rate);
	if (rate == AUDIO_SAMPLE_RATE) {
	} else if (rate >= 8000 && rate <= 48000) {
		num |= 4;
	} else {
		return false;
	}
	// we're not checking the byte rate and block align fields
	// if they're not the expected values, all we could do is
	// return false.  Do any real wav files have unexpected
	// values in these other fields?
	state_play = num;
	return true;
}



/******************************************************************/


void AudioPlaySDcardRAW::begin(void)
{
	playing = false;
	if (block) {
		release(block);
		block = NULL;
	}
}


bool AudioPlaySDcardRAW::play(const char *filename)
{
	stop();
	rawfile = SD.open(filename);
	if (!rawfile) {
		Serial.println("unable to open file");
		return false;
	}
	Serial.println("able to open file");
	playing = true;
	return true;
}

void AudioPlaySDcardRAW::stop(void)
{
	__disable_irq();
	if (playing) {
		playing = false;
		__enable_irq();
		rawfile.close();
	} else {
		__enable_irq();
	}
}


void AudioPlaySDcardRAW::update(void)
{
	unsigned int i, n;

	// only update if we're playing
	if (!playing) return;

	// allocate the audio blocks to transmit
	block = allocate();
	if (block == NULL) return;

	if (rawfile.available()) {
		// we can read more data from the file...
		n = rawfile.read(block->data, AUDIO_BLOCK_SAMPLES*2);
		for (i=n/2; i < AUDIO_BLOCK_SAMPLES; i++) {
			block->data[i] = 0;
		}
		transmit(block);
		release(block);
	} else {
		rawfile.close();
		playing = false;
	}
}


/******************************************************************/



void AudioPlayMemory::play(const unsigned int *data)
{
	uint32_t format;

	playing = 0;
	prior = 0;
	format = *data++;
	next = data;
	length = format & 0xFFFFFF;
	playing = format >> 24;
}

void AudioPlayMemory::stop(void)
{
	playing = 0;
}

extern "C" {
extern const int16_t ulaw_decode_table[256];
};

void AudioPlayMemory::update(void)
{
	audio_block_t *block;
	const unsigned int *in;
	int16_t *out;
	uint32_t tmp32, consumed;
	int16_t s0, s1, s2, s3, s4;
	int i;

	if (!playing) return;
	block = allocate();
	if (block == NULL) return;

	//Serial.write('.');

	out = block->data;
	in = next;
	s0 = prior;

	switch (playing) {
	  case 0x01: // u-law encoded, 44100 Hz
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i += 4) {
			tmp32 = *in++;
			*out++ = ulaw_decode_table[(tmp32 >> 0) & 255];
			*out++ = ulaw_decode_table[(tmp32 >> 8) & 255];
			*out++ = ulaw_decode_table[(tmp32 >> 16) & 255];
			*out++ = ulaw_decode_table[(tmp32 >> 24) & 255];
		}
		consumed = 128;
		break;

	  case 0x81: // 16 bit PCM, 44100 Hz
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i += 2) {
			tmp32 = *in++;
			*out++ = (int16_t)(tmp32 & 65535);
			*out++ = (int16_t)(tmp32 >> 16);
		}
		consumed = 128;
		break;

	  case 0x02: // u-law encoded, 22050 Hz 
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i += 8) {
			tmp32 = *in++;
			s1 = ulaw_decode_table[(tmp32 >> 0) & 255];
			s2 = ulaw_decode_table[(tmp32 >> 8) & 255];
			s3 = ulaw_decode_table[(tmp32 >> 16) & 255];
			s4 = ulaw_decode_table[(tmp32 >> 24) & 255];
			*out++ = (s0 + s1) >> 1;
			*out++ = s1;
			*out++ = (s1 + s2) >> 1;
			*out++ = s2;
			*out++ = (s2 + s3) >> 1;
			*out++ = s3;
			*out++ = (s3 + s4) >> 1;
			*out++ = s4;
			s0 = s4;
		}
		consumed = 64;
		break;

	  case 0x82: // 16 bits PCM, 22050 Hz
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i += 4) {
			tmp32 = *in++;
			s1 = (int16_t)(tmp32 & 65535);
			s2 = (int16_t)(tmp32 >> 16);
			*out++ = (s0 + s1) >> 1;
			*out++ = s1;
			*out++ = (s1 + s2) >> 1;
			*out++ = s2;
			s0 = s2;
		}
		consumed = 64;
		break;

	  case 0x03: // u-law encoded, 11025 Hz
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i += 16) {
			tmp32 = *in++;
			s1 = ulaw_decode_table[(tmp32 >> 0) & 255];
			s2 = ulaw_decode_table[(tmp32 >> 8) & 255];
			s3 = ulaw_decode_table[(tmp32 >> 16) & 255];
			s4 = ulaw_decode_table[(tmp32 >> 24) & 255];
			*out++ = (s0 * 3 + s1) >> 2;
			*out++ = (s0 + s1)     >> 1;
			*out++ = (s0 + s1 * 3) >> 2;
			*out++ = s1;
			*out++ = (s1 * 3 + s2) >> 2;
			*out++ = (s1 + s2)     >> 1;
			*out++ = (s1 + s2 * 3) >> 2;
			*out++ = s2;
			*out++ = (s2 * 3 + s3) >> 2;
			*out++ = (s2 + s3)     >> 1;
			*out++ = (s2 + s3 * 3) >> 2;
			*out++ = s3;
			*out++ = (s3 * 3 + s4) >> 2;
			*out++ = (s3 + s4)     >> 1;
			*out++ = (s3 + s4 * 3) >> 2;
			*out++ = s4;
			s0 = s4;
		}
		consumed = 32;
		break;

	  case 0x83: // 16 bit PCM, 11025 Hz
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i += 8) {
			tmp32 = *in++;
			s1 = (int16_t)(tmp32 & 65535);
			s2 = (int16_t)(tmp32 >> 16);
			*out++ = (s0 * 3 + s1) >> 2;
			*out++ = (s0 + s1)     >> 1;
			*out++ = (s0 + s1 * 3) >> 2;
			*out++ = s1;
			*out++ = (s1 * 3 + s2) >> 2;
			*out++ = (s1 + s2)     >> 1;
			*out++ = (s1 + s2 * 3) >> 2;
			*out++ = s2;
			s0 = s2;
		}
		consumed = 32;
		break;

	  default:
		release(block);
		playing = 0;
		return;
	}
	prior = s0;
	next = in;
	if (length > consumed) {
		length -= consumed;
	} else {
		playing = 0;
	}
	transmit(block);
	release(block);
}



/******************************************************************/


void applyGain(int16_t *data, int32_t mult)
{
	uint32_t *p = (uint32_t *)data;
	const uint32_t *end = (uint32_t *)(data + AUDIO_BLOCK_SAMPLES);

	do {
		uint32_t tmp32 = *p; // read 2 samples from *data
		int32_t val1 = signed_multiply_32x16b(mult, tmp32);
		int32_t val2 = signed_multiply_32x16t(mult, tmp32);
		val1 = signed_saturate_rshift(val1, 16, 0);
		val2 = signed_saturate_rshift(val2, 16, 0);
		*p++ = pack_16x16(val2, val1);
	} while (p < end);
}

// page 133

void applyGainThenAdd(int16_t *data, const int16_t *in, int32_t mult)
{
	uint32_t *dst = (uint32_t *)data;
	const uint32_t *src = (uint32_t *)in;
	const uint32_t *end = (uint32_t *)(data + AUDIO_BLOCK_SAMPLES);

	if (mult == 65536) {
		do {
			uint32_t tmp32 = *dst;
			*dst++ = signed_add_16_and_16(tmp32, *src++);
			tmp32 = *dst;
			*dst++ = signed_add_16_and_16(tmp32, *src++);
		} while (dst < end);
	} else {
		do {
			uint32_t tmp32 = *src++; // read 2 samples from *data
			int32_t val1 = signed_multiply_32x16b(mult, tmp32);
			int32_t val2 = signed_multiply_32x16t(mult, tmp32);
			val1 = signed_saturate_rshift(val1, 16, 0);
			val2 = signed_saturate_rshift(val2, 16, 0);
			tmp32 = pack_16x16(val2, val1);
			uint32_t tmp32b = *dst;
			*dst++ = signed_add_16_and_16(tmp32, tmp32b);
		} while (dst < end);
	}
}


void AudioMixer4::update(void)
{
	audio_block_t *in, *out=NULL;
	unsigned int channel;

	for (channel=0; channel < 4; channel++) {
		if (!out) {
			out = receiveWritable(channel);
			if (out) {
				int32_t mult = multiplier[channel];
				if (mult != 65536) applyGain(out->data, mult);
			}
		} else {
			in = receiveReadOnly(channel);
			if (in) {
				applyGainThenAdd(out->data, in->data, multiplier[channel]);
				release(in);
			}
		}
	}
	if (out) {
		transmit(out);
		release(out);
	}
}


/******************************************************************/





void AudioFilterBiquad::update(void)
{
	audio_block_t *block;
	int32_t a0, a1, a2, b1, b2, sum;
	uint32_t in2, out2, aprev, bprev, flag;
	uint32_t *data, *end;
	int32_t *state;

	block = receiveWritable();
	if (!block) return;
	data = (uint32_t *)(block->data);
	end = data + AUDIO_BLOCK_SAMPLES/2;
	state = (int32_t *)definition;
	do {
		a0 = *state++;
		a1 = *state++;
		a2 = *state++;
		b1 = *state++;
		b2 = *state++;
		aprev = *state++;
		bprev = *state++;
		sum = *state & 0x3FFF;
		do {
			in2 = *data;
			sum = signed_multiply_accumulate_32x16b(sum, a0, in2);
			sum = signed_multiply_accumulate_32x16t(sum, a1, aprev);
			sum = signed_multiply_accumulate_32x16b(sum, a2, aprev);
			sum = signed_multiply_accumulate_32x16t(sum, b1, bprev);
			sum = signed_multiply_accumulate_32x16b(sum, b2, bprev);
			out2 = (uint32_t)sum >> 14;
			sum &= 0x3FFF;
			sum = signed_multiply_accumulate_32x16t(sum, a0, in2);
			sum = signed_multiply_accumulate_32x16b(sum, a1, in2);
			sum = signed_multiply_accumulate_32x16t(sum, a2, aprev);
			sum = signed_multiply_accumulate_32x16b(sum, b1, out2);
			sum = signed_multiply_accumulate_32x16t(sum, b2, bprev);
			aprev = in2;
			bprev = pack_16x16(sum >> 14, out2);
			sum &= 0x3FFF;
			aprev = in2;
			*data++ = bprev;
		} while (data < end);
		flag = *state & 0x80000000;
		*state++ = sum | flag;
		*(state-2) = bprev;
		*(state-3) = aprev;
	} while (flag);
	transmit(block);
	release(block);
}

void AudioFilterBiquad::updateCoefs(int *source, bool doReset)
{
	int32_t *dest=(int32_t *)definition;
	int32_t *src=(int32_t *)source;
	__disable_irq();
	for(uint8_t index=0;index<5;index++)
	{
		*dest++=*src++;
	}
	if(doReset)
	{
		*dest++=0;
		*dest++=0;
		*dest++=0;
	}
	__enable_irq();
}

void AudioFilterBiquad::updateCoefs(int *source)
{
	updateCoefs(source,false);
}

/******************************************************************/


extern "C" {
extern const int16_t fader_table[256];
};


void AudioEffectFade::update(void)
{
	audio_block_t *block;
	uint32_t i, pos, inc, index, scale;
	int32_t val1, val2, val, sample;
	uint8_t dir;

	pos = position;
	if (pos == 0) {
		// output is silent
		block = receiveReadOnly();
		if (block) release(block);
		return;
	} else if (pos == 0xFFFFFFFF) {
		// output is 100%
		block = receiveReadOnly();
		if (!block) return;
		transmit(block);
		release(block);
		return;
	}
	block = receiveWritable();
	if (!block) return;
	inc = rate;
	dir = direction;
	for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
		index = pos >> 24;
		val1 = fader_table[index];
		val2 = fader_table[index+1];
		scale = (pos >> 8) & 0xFFFF;
		val2 *= scale;
		val1 *= 0x10000 - scale;
		val = (val1 + val2) >> 16;
		sample = block->data[i];
		sample = (sample * val) >> 15;
		block->data[i] = sample;
		if (dir > 0) {
			// output is increasing
			if (inc < 0xFFFFFFFF - pos) pos += inc;
			else pos = 0xFFFFFFFF;
		} else {
			// output is decreasing
			if (inc < pos) pos -= inc;
			else pos = 0;
		}
	}
	position = pos;
	transmit(block);
	release(block);
}

void AudioEffectFade::fadeBegin(uint32_t newrate, uint8_t dir)
{
	__disable_irq();
	uint32_t pos = position;
	if (pos == 0) position = 1;
	else if (pos == 0xFFFFFFFF) position = 0xFFFFFFFE;
	rate = newrate;
	direction = dir;
	__enable_irq();
}



/******************************************************************/


static inline int32_t multiply_32x32_rshift30(int32_t a, int32_t b) __attribute__((always_inline));
static inline int32_t multiply_32x32_rshift30(int32_t a, int32_t b)
{
	return ((int64_t)a * (int64_t)b) >> 30;
}

//#define TONE_DETECT_FAST

void AudioAnalyzeToneDetect::update(void)
{
	audio_block_t *block;
	int32_t q0, q1, q2, coef;
	const int16_t *p, *end;
	uint16_t n;

	block = receiveReadOnly();
	if (!block) return;
	if (!enabled) {
		release(block);
		return;
	}
	p = block->data;
	end = p + AUDIO_BLOCK_SAMPLES;
	n = count;
	coef = coefficient;
	q1 = s1;
	q2 = s2;
	do {
		// the Goertzel algorithm is kinda magical ;-)
#ifdef TONE_DETECT_FAST
		q0 = (*p++) + (multiply_32x32_rshift32_rounded(coef, q1) << 2) - q2;
#else
		q0 = (*p++) + multiply_32x32_rshift30(coef, q1) - q2;
		// TODO: is this only 1 cycle slower?  if so, always use it
#endif
		q2 = q1;
		q1 = q0;
		if (--n == 0) {
			out1 = q1;
			out2 = q2;
			q1 = 0;  // TODO: does clearing these help or hinder?
			q2 = 0;
			new_output = true;
			n = length;
		}
	} while (p < end);
	count = n;
	s1 = q1;
	s2 = q2;
	release(block);
}

void AudioAnalyzeToneDetect::set_params(int32_t coef, uint16_t cycles, uint16_t len)
{
	__disable_irq();
	coefficient = coef;
	ncycles = cycles;
	length = len;
	count = len;
	s1 = 0;
	s2 = 0;
	enabled = true;
	__enable_irq();
	//Serial.printf("Tone: coef=%d, ncycles=%d, length=%d\n", coefficient, ncycles, length);
}

float AudioAnalyzeToneDetect::read(void)
{
	int32_t coef, q1, q2, power;
	uint16_t len;

	__disable_irq();
	coef = coefficient;
	q1 = out1;
	q2 = out2;
	len = length;
	__enable_irq();
#ifdef TONE_DETECT_FAST
	power = multiply_32x32_rshift32_rounded(q2, q2);
	power = multiply_accumulate_32x32_rshift32_rounded(power, q1, q1);
	power = multiply_subtract_32x32_rshift32_rounded(power,
		multiply_32x32_rshift30(q1, q2), coef);
	power <<= 4;
#else
	int64_t power64;
	power64 = (int64_t)q2 * (int64_t)q2;
	power64 += (int64_t)q1 * (int64_t)q1;
	power64 -= (((int64_t)q1 * (int64_t)q2) >> 30) * (int64_t)coef;
	power = power64 >> 28;
#endif
	return sqrtf((float)power) / (float)len;
}


AudioAnalyzeToneDetect::operator bool()
{
	int32_t coef, q1, q2, power, trigger;
	uint16_t len;

	__disable_irq();
	coef = coefficient;
	q1 = out1;
	q2 = out2;
	len = length;
	__enable_irq();
#ifdef TONE_DETECT_FAST
	power = multiply_32x32_rshift32_rounded(q2, q2);
	power = multiply_accumulate_32x32_rshift32_rounded(power, q1, q1);
	power = multiply_subtract_32x32_rshift32_rounded(power,
		multiply_32x32_rshift30(q1, q2), coef);
	power <<= 4;
#else
	int64_t power64;
	power64 = (int64_t)q2 * (int64_t)q2;
	power64 += (int64_t)q1 * (int64_t)q1;
	power64 -= (((int64_t)q1 * (int64_t)q2) >> 30) * (int64_t)coef;
	power = power64 >> 28;
#endif
	trigger = (uint32_t)len * thresh;
	trigger = multiply_32x32_rshift32(trigger, trigger);

	Serial.printf("bool: power=%d, trig=%d\n", power, trigger);
	return (power >= trigger);
}




/******************************************************************/


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


/******************************************************************/
//                A u d i o E f f e c t F l a n g e
// Written by Pete (El Supremo) Jan 2014
// 140207 - fix calculation of delay_rate_incr which is expressed as
//			a fraction of 2*PI
// 140207 - cosmetic fix to begin()

// circular addressing indices for left and right channels
short AudioEffectFlange::l_circ_idx;
short AudioEffectFlange::r_circ_idx;

short * AudioEffectFlange::l_delayline = NULL;
short * AudioEffectFlange::r_delayline = NULL;

// User-supplied offset for the delayed sample
// but start with passthru
int AudioEffectFlange::delay_offset_idx = DELAY_PASSTHRU;
int AudioEffectFlange::delay_length;

int AudioEffectFlange::delay_depth;
int AudioEffectFlange::delay_rate_incr;
unsigned int AudioEffectFlange::l_delay_rate_index;
unsigned int AudioEffectFlange::r_delay_rate_index;
// fails if the user provides unreasonable values but will
// coerce them and go ahead anyway. e.g. if the delay offset
// is >= CHORUS_DELAY_LENGTH, the code will force it to
// CHORUS_DELAY_LENGTH-1 and return false.
// delay_rate is the rate (in Hz) of the sine wave modulation
// delay_depth is the maximum variation around delay_offset
// i.e. the total offset is delay_offset + delay_depth * sin(delay_rate)
boolean AudioEffectFlange::begin(short *delayline,int d_length,int delay_offset,int d_depth,float delay_rate)
{
  boolean all_ok = true;

if(0) {
  Serial.print("AudioEffectFlange.begin(offset = ");
  Serial.print(delay_offset);
  Serial.print(", depth = ");
  Serial.print(d_depth);
  Serial.print(", rate = ");
  Serial.print(delay_rate,3);
  Serial.println(")");
  Serial.print("    FLANGE_DELAY_LENGTH = ");
  Serial.println(d_length);
}
  delay_length = d_length/2;
  l_delayline = delayline;
  r_delayline = delayline + delay_length;
  
  delay_depth = d_depth;
  // initial index
  l_delay_rate_index = 0;
  r_delay_rate_index = 0;
  l_circ_idx = 0;
  r_circ_idx = 0;
  delay_rate_incr = delay_rate/44100.*2147483648.; 
//Serial.println(delay_rate_incr,HEX);

  delay_offset_idx = delay_offset;
  // Allow the passthru code to go through
  if(delay_offset_idx < -1) {
    delay_offset_idx = 0;
    all_ok = false;
  }
  if(delay_offset_idx >= delay_length) {
    delay_offset_idx = delay_length - 1;
    all_ok = false;
  }  
  return(all_ok);
}


boolean AudioEffectFlange::modify(int delay_offset,int d_depth,float delay_rate)
{
  boolean all_ok = true;
  
  delay_depth = d_depth;

  delay_rate_incr = delay_rate/44100.*2147483648.;
  
  delay_offset_idx = delay_offset;
  // Allow the passthru code to go through
  if(delay_offset_idx < -1) {
    delay_offset_idx = 0;
    all_ok = false;
  }
  if(delay_offset_idx >= delay_length) {
    delay_offset_idx = delay_length - 1;
    all_ok = false;
  }
  l_delay_rate_index = 0;
  r_delay_rate_index = 0;
  l_circ_idx = 0;
  r_circ_idx = 0;
  return(all_ok);
}

void AudioEffectFlange::update(void)
{
  audio_block_t *block;
  int idx;
  short *bp;
  short frac;
  int idx1;

  if(l_delayline == NULL)return;
  if(r_delayline == NULL)return; 

  // do passthru
  if(delay_offset_idx == DELAY_PASSTHRU) {
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
      idx = arm_sin_q15( (q15_t)((l_delay_rate_index >> 16) & 0x7fff));
      idx = (idx * delay_depth) >> 15;
//Serial.println(idx);
      idx = l_circ_idx - (delay_offset_idx + idx);
      if(idx < 0) {
        idx += delay_length;
      }
      if(idx >= delay_length) {
        idx -= delay_length;
      }

      if(frac < 0)
        idx1 = idx - 1;
      else
        idx1 = idx + 1;
      if(idx1 < 0) {
        idx1 += delay_length;
      }
      if(idx1 >= delay_length) {
        idx1 -= delay_length;
      }
      frac = (l_delay_rate_index >> 1) &0x7fff;
      frac = (( (int)(l_delayline[idx1] - l_delayline[idx])*frac) >> 15);

      *bp++ = (l_delayline[l_circ_idx]
                + l_delayline[idx] + frac               
              )/2;

      l_delay_rate_index += delay_rate_incr;
      if(l_delay_rate_index & 0x80000000) {
        l_delay_rate_index &= 0x7fffffff;
      }
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
      idx = arm_sin_q15( (q15_t)((r_delay_rate_index >> 16)&0x7fff));
       idx = (idx * delay_depth) >> 15;

      idx = r_circ_idx - (delay_offset_idx + idx);
      if(idx < 0) {
        idx += delay_length;
      }
      if(idx >= delay_length) {
        idx -= delay_length;
      }

      if(frac < 0)
        idx1 = idx - 1;
      else
        idx1 = idx + 1;
      if(idx1 < 0) {
        idx1 += delay_length;
      }
      if(idx1 >= delay_length) {
        idx1 -= delay_length;
      }
      frac = (r_delay_rate_index >> 1) &0x7fff;
      frac = (( (int)(r_delayline[idx1] - r_delayline[idx])*frac) >> 15);

      *bp++ = (r_delayline[r_circ_idx]
                + r_delayline[idx] + frac
               )/2;

      r_delay_rate_index += delay_rate_incr;
      if(r_delay_rate_index & 0x80000000) {
        r_delay_rate_index &= 0x7fffffff;
      }

    }
    // send the effect output to the right channel
    transmit(block,1);
    release(block);
  }
}



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



/******************************************************************/

//                A u d i o T o n e S w e e p
// Written by Pete (El Supremo) Feb 2014


boolean AudioToneSweep::begin(short t_amp,int t_lo,int t_hi,float t_time)
{
  double tone_tmp;
  
if(0) {
  Serial.print("AudioToneSweep.begin(tone_amp = ");
  Serial.print(t_amp);
  Serial.print(", tone_lo = ");
  Serial.print(t_lo);
  Serial.print(", tone_hi = ");
  Serial.print(t_hi);
  Serial.print(", tone_time = ");
  Serial.print(t_time,1);
  Serial.println(")");
}
  tone_amp = 0;
  if(t_amp < 0)return false;
  if(t_lo < 1)return false;
  if(t_hi < 1)return false;
  if(t_hi >= 44100/2)return false;
  if(t_lo >= 44100/2)return false;
  if(t_time < 0)return false;
  tone_lo = t_lo;
  tone_hi = t_hi;
  tone_phase = 0;

  tone_amp = t_amp;
  // Limit the output amplitude to prevent aliasing
  // until I can figure out why this "overtops"
  // above 29000.
  if(tone_amp > 29000)tone_amp = 29000;
  tone_tmp = tone_hi - tone_lo;
  tone_sign = 1;
  tone_freq = tone_lo*0x100000000LL;
  if(tone_tmp < 0) {
    tone_sign = -1;
    tone_tmp = -tone_tmp;
  }
  tone_tmp = tone_tmp/t_time/44100.;
  tone_incr = (tone_tmp * 0x100000000LL);
  sweep_busy = 1;
  return(true);
}



unsigned char AudioToneSweep::busy(void)
{
  return(sweep_busy);
}

int b_count = 0;
void AudioToneSweep::update(void)
{
  audio_block_t *block;
  short *bp;
  int i;
  
  if(!sweep_busy)return;

  //          L E F T  C H A N N E L  O N L Y
  block = allocate();
  if(block) {
    bp = block->data;
    // Generate the sweep
    for(i = 0;i < AUDIO_BLOCK_SAMPLES;i++) {
      *bp++ = (short)(( (short)(arm_sin_q31((uint32_t)((tone_phase >> 15)&0x7fffffff))>>16) *tone_amp) >> 16);
      uint64_t tone_tmp = (0x400000000000LL * (int)((tone_freq >> 32)&0x7fffffff))/44100;

      tone_phase +=  tone_tmp;
      if(tone_phase & 0x800000000000LL)tone_phase &= 0x7fffffffffffLL;

      if(tone_sign > 0) {
        if((tone_freq >> 32) > tone_hi) {
          sweep_busy = 0;
          break;
        }
        tone_freq += tone_incr;
      } else {
        if((tone_freq >> 32) < tone_hi) {
          sweep_busy = 0;

          break;
        }
        tone_freq -= tone_incr;        
      }
    }
    while(i < AUDIO_BLOCK_SAMPLES) {
      *bp++ = 0;
      i++;
    }
    b_count++;
    // send the samples to the left channel
    transmit(block,0);
    release(block);
  }
}
