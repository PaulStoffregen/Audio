#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

// GUItool: begin automatically generated code
AudioInputI2S            i2s1;           //xy=99,60
AudioFilterBiquad        biquad1;        //xy=257,60
AudioOutputI2S           i2s2;           //xy=416,60
AudioConnection          patchCord1(i2s1, 0, biquad1, 0);
AudioConnection          patchCord2(biquad1, 0, i2s2, 0);
AudioConnection          patchCord3(biquad1, 0, i2s2, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=305,132
// GUItool: end automatically generated code


const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;

void setup() {
  AudioMemory(12);

  sgtl5000_1.enable();  // Enable the audio shield
  sgtl5000_1.inputSelect(myInput);
  sgtl5000_1.volume(0.5);

  // Butterworth filter, 12 db/octave
  biquad1.setLowpass(0, 800, 0.707);

  // Linkwitz-Riley filter, 48 dB/octave
  //biquad1.setLowpass(0, 800, 0.54);
  //biquad1.setLowpass(1, 800, 1.3);
  //biquad1.setLowpass(2, 800, 0.54);
  //biquad1.setLowpass(3, 800, 1.3);
}


void loop() {
}


