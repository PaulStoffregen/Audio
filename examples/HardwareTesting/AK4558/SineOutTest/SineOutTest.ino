#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthWaveformSine   sine2;          //xy=309,245
AudioSynthWaveformSine   sine1;          //xy=320,133
AudioOutputI2S           i2s2;           //xy=460,178
AudioConnection          patchCord1(sine2, 0, i2s2, 1);
AudioConnection          patchCord2(sine1, 0, i2s2, 0);
AudioControlAK4558       ak4558;       //xy=172,178
// GUItool: end automatically generated code

void setup() {
  // put your setup code here, to run once:
  sine1.frequency(440);
  sine2.frequency(220);
  sine1.amplitude(1.0);
  sine2.amplitude(1.0);
  delay(10000);
  Serial.begin(9600);
  Serial.println("test");
  ak4558.enable();
}

void loop() {
  delay(1);
}
