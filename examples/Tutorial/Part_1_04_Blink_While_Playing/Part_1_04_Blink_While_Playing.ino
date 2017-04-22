// Advanced Microcontroller-based Audio Workshop
//
// http://www.pjrc.com/store/audio_tutorial_kit.html
// https://hackaday.io/project/8292-microcontroller-audio-workshop-had-supercon-2015
// 
// Part 1-4: Blink LED while Playing Music
//
// Do something while playing a music file.  Admittedly
// only blink Teensy's LED and print info about the
// playing time in the file.  The point is that Arduino
// sketch code is free to do other work while the audio
// library streams data from the SD card to the headphones.

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

AudioPlaySdWav           playSdWav1;
AudioOutputI2S           audioOutput;
AudioConnection          patchCord1(playSdWav1, 0, audioOutput, 0);
AudioConnection          patchCord2(playSdWav1, 1, audioOutput, 1);
AudioControlSGTL5000     sgtl5000_1;

// Use these with the Teensy Audio Shield
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14

// Use these with the Teensy 3.5 & 3.6 SD card
//#define SDCARD_CS_PIN    BUILTIN_SDCARD
//#define SDCARD_MOSI_PIN  11  // not actually used
//#define SDCARD_SCK_PIN   13  // not actually used

// Use these for the SD+Wiz820 or other adaptors
//#define SDCARD_CS_PIN    4
//#define SDCARD_MOSI_PIN  11
//#define SDCARD_SCK_PIN   13

void setup() {
  Serial.begin(9600);
  AudioMemory(8);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.45);
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
  pinMode(13, OUTPUT); // LED on pin 13
  delay(1000);
}

void loop() {
  if (playSdWav1.isPlaying() == false) {
    Serial.println("Start playing");
    playSdWav1.play("SDTEST3.WAV");
    delay(10); // wait for library to parse WAV info
  }

  // print the play time offset
  Serial.print("Playing, now at ");
  Serial.print(playSdWav1.positionMillis());
  Serial.println(" ms");

  // blink LED and print info while playing
  digitalWrite(13, HIGH);
  delay(250);
  digitalWrite(13, LOW);
  delay(250);

  // read the knob position (analog input A2)
  /*
  int knob = analogRead(A2);
  float vol = (float)knob / 1280.0;
  sgtl5000_1.volume(vol);
  Serial.print("volume = ");
  Serial.println(vol);
  */
}




