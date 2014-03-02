/* Mono peak meter example using Analog objects. Assumes Teensy 3.1

At a minimum DC decouple audio signals to/from Teensy pins with capacitors in the signal paths both in and out, 10uF is often used.
Possibly worthwhile to set up virtual ground at 3v3/2 for both, or if changing DAC REF to 1.2V then 1.2V/2 for output side.

This example code is in the public domain
*/

#include <Audio.h>
#include <Wire.h>
#include <SD.h>

const int myInput = AUDIO_INPUT_LINEIN;
// const int myInput = AUDIO_INPUT_MIC;

AudioInputAnalog        audioInput(A0);         // A0 is pin 14, feel free to change.
AudioPeak               peak_M;
AudioOutputAnalog       audioOutput;            // DAC pin.

AudioConnection c1(audioInput,peak_M);
AudioConnection c2(audioInput,audioOutput);

void setup() {
  AudioMemory(4);
  Serial.begin(Serial.baud());
}

elapsedMillis fps;

void loop() {
  if(fps>24) { // for best effect make your terminal/monitor a minimum of 31 chars wide and as high as you can.
    Serial.println();
    fps=0;
    uint8_t monoPeak=peak_M.Dpp()/2184.5321;
    Serial.print("|");
    for(uint8_t cnt=0;cnt<monoPeak;cnt++) Serial.print(">");
    peak_M.begin();
  }
}
