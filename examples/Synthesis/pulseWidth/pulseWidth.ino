
// demonstrate pulse with slow changes in pulse width

#include <Audio.h>
#include <Wire.h>
#ifndef __SAMD51__
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#endif

// GUItool: begin automatically generated code
AudioSynthWaveform       waveform1;      //xy=188,240
AudioEffectEnvelope      envelope1;      //xy=371,237

#ifndef __SAMD51__
AudioOutputI2S           audioOut;           //xy=565,241
#else
AudioOutputAnalogStereo  audioOut;
#endif 
AudioConnection          patchCord1(waveform1, envelope1);
AudioConnection          patchCord2(envelope1, 0, audioOut, 0);
AudioConnection          patchCord3(envelope1, 0, audioOut, 1);
#ifndef __SAMD51__
AudioControlSGTL5000     audioShield;     //xy=586,175
#endif
// GUItool: end automatically generated code


void setup(void)
{

  // Set up
  AudioMemory(8);
#ifndef __SAMD51__
  audioShield.enable();
  audioShield.volume(0.45);
#endif

  waveform1.pulseWidth(0.5);
  waveform1.begin(0.4, 220, WAVEFORM_PULSE);

  envelope1.attack(50);
  envelope1.decay(50);
  envelope1.release(250);

}

void loop() {
  
  float w;
  for (uint32_t i =1; i<20; i++) {
    w = i / 20.0;
    waveform1.pulseWidth(w);
    envelope1.noteOn();
    delay(800);
    envelope1.noteOff();
    delay(600);
  }
}


