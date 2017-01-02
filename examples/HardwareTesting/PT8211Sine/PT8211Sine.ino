#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthWaveform       waveform1;      //xy=110,75
AudioOutputPT8211        pt8211_1;          //xy=303,78
AudioConnection          patchCord1(waveform1, 0, pt8211_1, 0);
AudioConnection          patchCord2(waveform1, 0, pt8211_1, 1);
// GUItool: end automatically generated code

void setup() {
  AudioMemory(15);
  waveform1.begin(WAVEFORM_SINE);
  waveform1.frequency(440);
  waveform1.amplitude(0.99);
}

void loop() {
}
