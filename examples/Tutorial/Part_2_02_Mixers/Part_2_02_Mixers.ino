// Advanced Microcontroller-based Audio Workshop
//
// http://www.pjrc.com/store/audio_tutorial_kit.html
// https://hackaday.io/project/8292-microcontroller-audio-workshop-had-supercon-2015
// 
// Part 2-2: Mixers & Playing Multiple Sounds


///////////////////////////////////
// copy the Design Tool code here
///////////////////////////////////




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
  sgtl5000_1.volume(0.5);
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
  pinMode(13, OUTPUT); // LED on pin 13
  mixer1.gain(0, 0.5);
  mixer1.gain(1, 0.5);
  mixer2.gain(0, 0.5);
  mixer2.gain(1, 0.5);
  delay(1000);
}

void loop() {
  if (playSdWav1.isPlaying() == false) {
    Serial.println("Start playing 1");
    playSdWav1.play("SDTEST1.WAV");
    delay(10); // wait for library to parse WAV info
  }
  if (playSdWav2.isPlaying() == false) {
    Serial.println("Start playing 2");
    playSdWav2.play("SDTEST4.WAV");
    delay(10); // wait for library to parse WAV info
  }
  // uncomment this code to allow Knob A3 to pan between songs
  /*
  int knob = analogRead(A3);  // knob = 0 to 1023
  float gain1 = (float)knob / 1023.0;
  float gain2 = 1.0 - gain1;
  mixer1.gain(0, gain1);
  mixer1.gain(1, gain2);
  mixer2.gain(0, gain1);
  mixer2.gain(1, gain2);
  */
}










