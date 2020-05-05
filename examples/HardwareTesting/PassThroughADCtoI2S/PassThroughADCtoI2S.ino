/*
 * A simple hardware test which receives audio on the A2 analog pin
 * and sends it to the audio shield (I2S digital audio)
 *
 * This example code is in the public domain.
 */

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioInputAnalog         adc1;           //xy=197,73
AudioOutputI2S           i2s1;           //xy=375,85
AudioConnection          patchCord1(adc1, 0, i2s1, 0);
AudioConnection          patchCord2(adc1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=314,158
// GUItool: end automatically generated code


void setup() {
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(12);

  // Enable the audio shield
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);
}

void loop() {
  // Do nothing here.  The Audio flows automatically

  // When AudioInputAnalog is running, analogRead() must NOT be used.
}


