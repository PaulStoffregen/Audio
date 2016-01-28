#include <LiquidCrystal.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioInputI2S            i2s1;           //xy=139,91
AudioMixer4              mixer1;         //xy=312,134
AudioOutputI2S           i2s2;           //xy=392,32
AudioAnalyzeFFT1024      fft1024;        //xy=467,147
AudioConnection          patchCord1(i2s1, 0, mixer1, 0);
AudioConnection          patchCord2(i2s1, 0, i2s2, 0);
AudioConnection          patchCord3(i2s1, 1, mixer1, 1);
AudioConnection          patchCord4(i2s1, 1, i2s2, 1);
AudioConnection          patchCord5(mixer1, fft1024);
AudioControlSGTL5000     audioShield;    //xy=366,225
// GUItool: end automatically generated code


const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;


// The scale sets how much sound is needed in each frequency range to
// show all 8 bars.  Higher numbers are more sensitive.
float scale = 60.0;

// An array to hold the 16 frequency bands
float level[16];

// This array holds the on-screen levels.  When the signal drops quickly,
// these are used to lower the on-screen level 1 bar per update, which
// looks more pleasing to corresponds to human sound perception.
int   shown[16];



// Use the LiquidCrystal library to display the spectrum
//
LiquidCrystal lcd(0, 1, 2, 3, 4, 5);
byte bar1[8] = {0,0,0,0,0,0,0,255};
byte bar2[8] = {0,0,0,0,0,0,255,255};        // 8 bar graph
byte bar3[8] = {0,0,0,0,0,255,255,255};      // custom
byte bar4[8] = {0,0,0,0,255,255,255,255};    // characters
byte bar5[8] = {0,0,0,255,255,255,255,255};
byte bar6[8] = {0,0,255,255,255,255,255,255};
byte bar7[8] = {0,255,255,255,255,255,255,255};
byte bar8[8] = {255,255,255,255,255,255,255,255};


void setup() {
  // Audio requires memory to work.
  AudioMemory(12);

  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(0.5);

  // turn on the LCD and define the custom characters
  lcd.begin(16, 2);
  lcd.print("Audio Spectrum");
  lcd.createChar(0, bar1);
  lcd.createChar(1, bar2);
  lcd.createChar(2, bar3);
  lcd.createChar(3, bar4);
  lcd.createChar(4, bar5);
  lcd.createChar(5, bar6);
  lcd.createChar(6, bar7);
  lcd.createChar(7, bar8);

  // configure the mixer to equally add left & right
  mixer1.gain(0, 0.5);
  mixer1.gain(1, 0.5);

  // pin 21 will select rapid vs animated display
  pinMode(21, INPUT_PULLUP);
}


void loop() {
  if (fft1024.available()) {
    // read the 512 FFT frequencies into 16 levels
    // music is heard in octaves, but the FFT data
    // is linear, so for the higher octaves, read
    // many FFT bins together.
    level[0] =  fft1024.read(0);
    level[1] =  fft1024.read(1);
    level[2] =  fft1024.read(2, 3);
    level[3] =  fft1024.read(4, 6);
    level[4] =  fft1024.read(7, 10);
    level[5] =  fft1024.read(11, 15);
    level[6] =  fft1024.read(16, 22);
    level[7] =  fft1024.read(23, 32);
    level[8] =  fft1024.read(33, 46);
    level[9] =  fft1024.read(47, 66);
    level[10] = fft1024.read(67, 93);
    level[11] = fft1024.read(94, 131);
    level[12] = fft1024.read(132, 184);
    level[13] = fft1024.read(185, 257);
    level[14] = fft1024.read(258, 359);
    level[15] = fft1024.read(360, 511);
    // See this conversation to change this to more or less than 16 log-scaled bands?
    // https://forum.pjrc.com/threads/32677-Is-there-a-logarithmic-function-for-FFT-bin-selection-for-any-given-of-bands

    // if you have the volume pot soldered to your audio shield
    // uncomment this line to make it adjust the full scale signal
    //scale = 8.0 + analogRead(A1) / 5.0;

    // begin drawing at the first character on the 2nd row
    lcd.setCursor(0, 1);
 
    for (int i=0; i<16; i++) {
      Serial.print(level[i]);

      // TODO: conversion from FFT data to display bars should be
      // exponentially scaled.  But how keep it a simple example?
      int val = level[i] * scale;
      if (val > 8) val = 8;

      if (val >= shown[i]) {
        shown[i] = val;
      } else {
        if (shown[i] > 0) shown[i] = shown[i] - 1;
        val = shown[i];
      }

      //Serial.print(shown[i]);
      Serial.print(" ");

      // print each custom digit
      if (shown[i] == 0) {
        lcd.write(' ');
      } else {
        lcd.write(shown[i] - 1);
      }
    }
    Serial.print(" cpu:");
    Serial.println(AudioProcessorUsageMax());
  }
}


