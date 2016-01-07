/*
 * AK4558 Sine Out Test
 * 2015 by Michele Perla
 *
 * A simple hardware test which sends two 440 Hz sinewaves to the
 * LOUT/ROUT pins of the HiFi Audio CODEC Module. One of the waves
 * is out-of-phase by 90° with the other.
 *
 */

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

AudioSynthWaveformSine   sine2;
AudioSynthWaveformSine   sine1;
AudioOutputI2S           i2s1;
AudioConnection          patchCord1(sine2, 0, i2s1, 0);
AudioConnection          patchCord2(sine1, 0, i2s1, 1);
AudioControlAK4558       ak4558;

int phase = 0;

void setup() {
  // put your setup code here, to run once:
  AudioMemory(12);
  while (!Serial);
  ak4558.enable();
  ak4558.enableOut();
  AudioNoInterrupts();
  sine1.frequency(440);
  sine2.frequency(440);
  sine1.amplitude(1.0);
  sine2.amplitude(1.0);
  AudioInterrupts();
}

void loop() {
	phase+=10;
	if (phase==360) phase=0;
	AudioNoInterrupts();
	sine2.phase(phase);
	AudioInterrupts();
	delay(250);
}
