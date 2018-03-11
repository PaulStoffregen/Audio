// ADAT 8 channel Toslink output test, by Ernstjan Freriks
// https://www.youtube.com/watch?v=e5ov3q02zxo
// https://forum.pjrc.com/threads/28639-S-pdif?p=159530&viewfull=1#post159530

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// WAV files converted to code by wav2sketch
#include "AudioSampleSnare.h"        // http://www.freesound.org/people/KEVOY/sounds/82583/
#include "AudioSampleTomtom.h"       // http://www.freesound.org/people/zgump/sounds/86334/
#include "AudioSampleHihat.h"        // http://www.freesound.org/people/mhc/sounds/102790/
#include "AudioSampleKick.h"         // http://www.freesound.org/people/DWSD/sounds/171104/
#include "AudioSampleGong.h"         // http://www.freesound.org/people/juskiddink/sounds/86773/
#include "AudioSampleCashregister.h" // http://www.freesound.org/people/kiddpark/sounds/201159/

// Create the Audio components.  These should be created in the
// order data flows, inputs/sources -> processing -> outputs
//
AudioPlayMemory    sound0;
AudioPlayMemory    sound1;  // five memory players, so we can play
AudioPlayMemory    sound2;  // all five sounds simultaneously
AudioPlayMemory    sound3;
AudioPlayMemory    sound4;
AudioPlayMemory    sound5;

// GUItool: begin automatically generated code
AudioSynthWaveform       sine1;         
AudioOutputADAT          adat1;        
AudioConnection          patchCord1(sine1, 0, adat1, 0);
AudioConnection          patchCord2(sine1, 0, adat1, 1);
AudioConnection          patchCord3(sound0, 0, adat1, 2);
AudioConnection          patchCord4(sound1, 0, adat1, 3);
AudioConnection          patchCord5(sound2, 0, adat1, 4);
AudioConnection          patchCord6(sound4, 0, adat1, 5);
AudioConnection          patchCord7(sound5, 0, adat1, 6);
// GUItool: end automatically generated code

void setup() {
  AudioMemory(10);
  sine1.begin(WAVEFORM_SINE);
  sine1.frequency(2000);

}

// the loop() methor runs over and over again,
// as long as the board has power

void loop() {
  sine1.amplitude(1.0);

  sound0.play(AudioSampleKick);

  delay(250);

  sound1.play(AudioSampleHihat);

  delay(250);
  
  sound1.play(AudioSampleHihat);
  sound2.play(AudioSampleSnare);

  delay(250);

  sound1.play(AudioSampleHihat);

  delay(250);

  sound0.play(AudioSampleKick);
  sound3.play(AudioSampleGong);
  

  delay(1000);
}
