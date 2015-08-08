// Simple sine wave test for WM8731 Audio Codec Board
//
// Requires the MikroElektronika Audio Codec board or similar hardware
//   http://www.mikroe.com/add-on-boards/audio-voice/audio-codec-proto/
//
// Recommended connections:
//
//    Mikroe   Teensy 3.1
//    ------   ----------
//     SCK         9
//     MISO       13
//     MOSI       22
//     ADCL
//     DACL       23
//     SDA        18
//     SCL        19
//     3.3V      +3.3V
//     GND        GND
//
// This example code is in the public domain.

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthWaveform       waveform1;      //xy=110,75
AudioOutputI2Sslave      i2ss1;          //xy=303,78
AudioConnection          patchCord1(waveform1, 0, i2ss1, 0);
AudioConnection          patchCord2(waveform1, 0, i2ss1, 1);
AudioControlWM8731master wm8731m1;       //xy=230,154
// GUItool: end automatically generated code


void setup() {
  wm8731m1.enable();

  AudioMemory(15);

  waveform1.begin(WAVEFORM_SINE);
  waveform1.frequency(440);
  waveform1.amplitude(0.9);

  wm8731m1.volume(0.50);
}


void loop() {
}
