#ifndef synth_waveform_h_
#define synth_waveform_h_

#include "AudioStream.h"
#include "arm_math.h"

// waveforms.c
extern "C" {
extern const int16_t AudioWaveformSine[257];
extern const int16_t AudioWaveformTriangle[257];
extern const int16_t AudioWaveformSquare[257];
extern const int16_t AudioWaveformSawtooth[257];
}

#ifdef ORIGINAL_AUDIOSYNTHWAVEFORM
class AudioSynthWaveform : public AudioStream
{
public:
	AudioSynthWaveform(const int16_t *waveform)
	  : AudioStream(0, NULL), wavetable(waveform), magnitude(0), phase(0)
					, ramp_down(0), ramp_up(0), ramp_mag(0), ramp_length(0)
	  				 { }
	void frequency(float freq) {
		if (freq > AUDIO_SAMPLE_RATE_EXACT / 2 || freq < 0.0) return;
		phase_increment = (freq / AUDIO_SAMPLE_RATE_EXACT) * 4294967296.0f;
	}
	void amplitude(float n) {        // 0 to 1.0
		if (n < 0) n = 0;
		else if (n > 1.0) n = 1.0;
// Ramp code
		if(magnitude && (n == 0)) {
			ramp_down = ramp_length;
			ramp_up = 0;
			last_magnitude = magnitude;
		}
		else if((magnitude == 0) && n) {
			ramp_up = ramp_length;
			ramp_down = 0;
		}
// set new magnitude
		magnitude = n * 32767.0;
	}
	virtual void update(void);
	void set_ramp_length(uint16_t r_length);
	
private:
	const int16_t *wavetable;
	uint16_t magnitude;
	uint16_t last_magnitude;
	uint32_t phase;
	uint32_t phase_increment;
	uint32_t ramp_down;
	uint32_t ramp_up;
	uint32_t ramp_mag;
	uint16_t ramp_length;
};

#else

#define AUDIO_SAMPLE_RATE_ROUNDED (44118)

#define DELAY_PASSTHRU -1

#define TONE_TYPE_SINE     0
#define TONE_TYPE_SAWTOOTH 1
#define TONE_TYPE_SQUARE   2
#define TONE_TYPE_TRIANGLE 3

class AudioSynthWaveform : 
public AudioStream
{
public:
  AudioSynthWaveform(void) : 
  AudioStream(0,NULL), 
  tone_freq(0), tone_phase(0), tone_incr(0), tone_type(0),
  ramp_down(0), ramp_up(0), ramp_length(0)
  { 
  }
  // Change the frequency on-the-fly to permit a phase-continuous
  // change between two frequencies.
  void frequency(int t_hi)
  {
    tone_incr = (0x100000000LL*t_hi)/AUDIO_SAMPLE_RATE_EXACT;  
  }
  // If ramp_length is non-zero this will set up
  // either a rmap up or a ramp down when a wave
  // first starts or when the amplitude is set
  // back to zero.
  // Note that if the ramp_length is N, the generated
  // wave will be N samples longer than when it is not
  // ramp
  void amplitude(float n) {        // 0 to 1.0
    if (n < 0) n = 0;
    else if (n > 1.0) n = 1.0;
    // Ramp code
    if(tone_amp && (n == 0)) {
      ramp_down = ramp_length;
      ramp_up = 0;
      last_tone_amp = tone_amp;
    }
    else if((tone_amp == 0) && n) {
      ramp_up = ramp_length;
      ramp_down = 0;
      // reset the phase when the amplitude was zero
      // and has now been increased. Note that this
      // happens even if the wave is not ramped
      // so that the signal starts at zero
      tone_phase = 0;
    }
    // set new magnitude
    tone_amp = n * 32767.0;
  }
  boolean begin(float t_amp,int t_hi,short t_type);
  virtual void update(void);
  void set_ramp_length(uint16_t r_length);

private:
  short    tone_amp;
  short    last_tone_amp;
  short    tone_freq;
  uint32_t tone_phase;
  uint32_t tone_incr;
  short    tone_type;

  uint32_t ramp_down;
  uint32_t ramp_up;
  uint16_t ramp_length;
};

#endif



#if 0
class AudioSineWaveMod : public AudioStream
{
public:
	AudioSineWaveMod() : AudioStream(1, inputQueueArray) {}
	void frequency(float freq);
	//void amplitude(q15 n);
	virtual void update(void);
private:
	uint32_t phase;
	uint32_t phase_increment;
	uint32_t modulation_factor;
	audio_block_t *inputQueueArray[1];
};
#endif



#endif
