#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>

// Create the Audio components.  These should be created in the
// order data flows, inputs/sources -> processing -> outputs
//
AudioPlaySdWav     wav;
AudioOutputI2S     dac;

// Create Audio connections between the components
//
AudioConnection c1(wav, 0, dac, 0);
AudioConnection c2(wav, 1, dac, 1);

// Create an object to control the audio shield.
// 
AudioControlSGTL5000 audioShield;


void setup() {
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(5);

  audioShield.enable();
  audioShield.volume(0.5);

  SPI.setMOSI(7);
  SPI.setSCK(14);
  if (SD.begin(10)) {
    wav.play("01_16M.WAV");
  }
}

void loop() {
  float vol = analogRead(15);
  vol = vol / 1024;
  audioShield.volume(vol);
  delay(20);
}

