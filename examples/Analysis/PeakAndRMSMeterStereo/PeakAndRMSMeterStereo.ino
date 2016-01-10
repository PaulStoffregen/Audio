/* Adaptation of Stereo peak meter example, including RMS.
assumes Audio adapter but just uses terminal so no more parts required.

This example code is in the public domain
*/

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

const int myInput = AUDIO_INPUT_LINEIN;
// const int myInput = AUDIO_INPUT_MIC;

AudioInputI2S        audioInput;         // audio shield: mic or line-in
AudioAnalyzePeak     peak_L;
AudioAnalyzePeak     peak_R;
AudioAnalyzeRMS      rms_L;
AudioAnalyzeRMS      rms_R;
AudioOutputI2S       audioOutput;        // audio shield: headphones & line-out

AudioConnection c1(audioInput, 0, peak_L, 0);
AudioConnection c2(audioInput, 1, peak_R, 0);
AudioConnection c3(audioInput, 0, rms_L, 0);
AudioConnection c4(audioInput, 1, rms_R, 0);
AudioConnection c5(audioInput, 0, audioOutput, 0);
AudioConnection c6(audioInput, 1, audioOutput, 1);

AudioControlSGTL5000 audioShield;


void setup() {
  AudioMemory(6);
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(0.5);
  Serial.begin(9600);
}

// for best effect make your terminal/monitor a minimum of 62 chars wide and as high as you can.

elapsedMillis fps;
uint8_t cnt=0;

void loop() {
  if(fps > 24) {
    if (peak_L.available() && peak_R.available() && rms_L.available() && rms_R.available()) {
      fps=0;
      uint8_t leftPeak = peak_L.read() * 30.0;
      uint8_t rightPeak = peak_R.read() * 30.0;
      uint8_t leftRMS = rms_L.read() * 30.0;
      uint8_t rightRMS = rms_R.read() * 30.0;

      for (cnt=0; cnt < 30-leftPeak; cnt++) {
        Serial.print(" ");
      }
      while (cnt++ < 29 && cnt < 30-leftRMS) {
        Serial.print("<");
      }
      while (cnt++ < 30) {
        Serial.print("=");
      }
      
      Serial.print("||");
      
      for(cnt=0; cnt < rightRMS; cnt++) {
        Serial.print("=");
      }
      for(; cnt < rightPeak; cnt++) {
        Serial.print(">");
      }
      while(cnt++ < 30) {
        Serial.print(" ");
      }
      Serial.print(AudioProcessorUsage());
      Serial.print("/");
      Serial.print(AudioProcessorUsageMax());
      Serial.println();
    }
  }
}
