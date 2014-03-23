/* Stereo peak meter example, assumes Audio adapter but just uses terminal so no more parts required.

This example code is in the public domain
*/

#include <Audio.h>
#include <Wire.h>
#include <SD.h>

const int myInput = AUDIO_INPUT_LINEIN;
// const int myInput = AUDIO_INPUT_MIC;

AudioInputI2S        audioInput;         // audio shield: mic or line-in
AudioPeak            peak_L;
AudioPeak            peak_R;
AudioOutputI2S       audioOutput;        // audio shield: headphones & line-out

AudioConnection c1(audioInput,0,peak_L,0);
AudioConnection c2(audioInput,1,peak_R,0);
AudioConnection c3(audioInput,0,audioOutput,0);
AudioConnection c4(audioInput,1,audioOutput,1);

AudioControlSGTL5000 audioShield;


void setup() {
  AudioMemory(6);
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(0.75);
  audioShield.unmuteLineout();
  Serial.begin(Serial.baud());
}

elapsedMillis fps;
uint8_t cnt=0;

void loop() {
  
  if(fps>24) { // for best effect make your terminal/monitor a minimum of 62 chars wide and as high as you can.
    Serial.println();
    fps=0;
    uint8_t leftPeak=peak_L.Dpp()/2184.5321; // 65536 / 2184.5321 ~ 30.
    for(cnt=0;cnt<30-leftPeak;cnt++) Serial.print(" ");
    while(cnt++<30) Serial.print("<");
    Serial.print("||");
    uint8_t rightPeak=peak_R.Dpp()/2184.5321;
    for(cnt=0;cnt<rightPeak;cnt++) Serial.print(">");
    while(cnt++<30) Serial.print(" ");
    peak_L.begin(); // no need to call .stop if all you want
    peak_R.begin(); // is to zero it.
  }
}
