
// demonstrate pulse with slow changes in pulse width

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthWaveform       waveform1;      //xy=188,240
AudioEffectEnvelope      envelope1;      //xy=371,237
AudioOutputI2S           i2s1;           //xy=565,241
AudioConnection          patchCord1(waveform1, envelope1);
AudioConnection          patchCord2(envelope1, 0, i2s1, 0);
AudioConnection          patchCord3(envelope1, 0, i2s1, 1);
AudioControlSGTL5000     audioShield;     //xy=586,175
// GUItool: end automatically generated code


void setup(void)
{

  // Set up
  AudioMemory(8);
  audioShield.enable();
  audioShield.volume(0.45);

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


