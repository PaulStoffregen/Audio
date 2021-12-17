#include <Audio.h>

AudioOutputI2S         i2sout ;

AudioSynthWaveform     sig ;   // signal to be modulated
AudioSynthWaveformDc   mod  ;  // modulate the amplitude into folder
AudioSynthWaveformDc   mod2  ;  // modulate the amplitude afterwards
AudioEffectWaveFolder  folder ;
AudioEffectMultiply    mult ;

AudioConnection        c0 (mod, 0, folder, 0) ;
AudioConnection        c1 (sig, 0, folder, 1) ;
AudioConnection        c2 (folder, 0, mult, 0) ;
AudioConnection        c3 (mod2, 0, mult, 1) ;
AudioConnection        c4 (mult, 0, i2sout, 0) ;
AudioConnection        c5 (mult, 0, i2sout, 1) ;

// The Audio Shield chip
AudioControlSGTL5000 codec;

#define MIN_GAIN 0.2
#define MAX_GAIN 5.0
#define GAIN_STEP 1.002

float gain = MIN_GAIN ;
bool increasing = true ;

void set_gains () // the output gain (mod2) compensates to keep vol roughly level
{
  AudioNoInterrupts () ;
  mod.amplitude (0.2 * gain) ;
  mod2.amplitude (gain > 1 ? 1 : 1 / gain) ;
  AudioInterrupts () ;
}

void audio_setup()
{
  AudioMemory(4);
  
  sig.begin (1, 70, WAVEFORM_SINE) ; // test tone
  set_gains () ;
  
  codec.enable();  
  codec.volume(0.3);  // headphone volume low for harsh waveform!
  codec.lineOutLevel (13) ;  // turn up line out to max
}

void loop () // constantly changing the gains
{
  if (increasing)
  {
    gain *= GAIN_STEP ;
    if (gain > MAX_GAIN)
    {
      gain = MAX_GAIN ;
      increasing = false ;
    }
  }
  else
  {
    gain /= GAIN_STEP ;
    if (gain < MIN_GAIN)
    {
      gain = MIN_GAIN ;
      increasing = true;
    }
  }

  set_gains () ;
  delay (3) ;   // wait for another audio block
}

void setup ()
{
  Serial.begin (115200) ;
  audio_setup () ;
}
