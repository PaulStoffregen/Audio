/*
 * AK4558 Passthrough Test
 * 2015 by Michele Perla
 *
 * A simple hardware test which receives audio from the HiFi Audio CODEC Module
 * LIN/RIN pins and send it to the LOUT/ROUT pins
 *
 */

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

AudioInputI2S		     i2s1;
AudioOutputI2S           i2s2;
AudioConnection          patchCord1(i2s1, 0, i2s2, 0);
AudioConnection          patchCord2(i2s1, 1, i2s2, 1);
AudioControlAK4558       ak4558;

void setup() {
  // put your setup code here, to run once:
  AudioMemory(12);
  while (!Serial);
  ak4558.enable();
  ak4558.enableIn();
  ak4558.enableOut();
}

void loop() {
}
