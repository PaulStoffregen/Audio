#include <Audio.h>
#include <SD.h>

// Get the six WAV files this program needs on your SD card:
// https://forum.pjrc.com/threads/64235?p=258095&viewfull=1#post258095

// Reducing this delay will attempt to play the files simultaneously.
// The clips are about 3.0 to 3.4 seconds, even though the voice is
// heard only for the about the first half second.
const int milliseconds = 3500;

// GUItool: begin automatically generated code
AudioPlaySdWav           playSdWav4;     //xy=259,267
AudioPlaySdWav           playSdWav3;     //xy=260,218
AudioPlaySdWav           playSdWav5;     //xy=260,317
AudioPlaySdWav           playSdWav6;     //xy=261,369
AudioPlaySdWav           playSdWav2;     //xy=262,169
AudioPlaySdWav           playSdWav1;     //xy=263,118
AudioMixer4              mixer1;         //xy=460,176
AudioMixer4              mixer2;         //xy=571,271
AudioOutputI2S           i2s1;           //xy=763,245
AudioConnection          patchCord1(playSdWav4, 0, mixer1, 3);
AudioConnection          patchCord2(playSdWav3, 0, mixer1, 2);
AudioConnection          patchCord3(playSdWav5, 0, mixer2, 1);
AudioConnection          patchCord4(playSdWav6, 0, mixer2, 2);
AudioConnection          patchCord5(playSdWav2, 0, mixer1, 1);
AudioConnection          patchCord6(playSdWav1, 0, mixer1, 0);
AudioConnection          patchCord7(mixer1, 0, mixer2, 0);
AudioConnection          patchCord8(mixer2, 0, i2s1, 0);
AudioConnection          patchCord9(mixer2, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=625,368
// GUItool: end automatically generated code

// SD card on Teensy 3.x Audio Shield (Rev C)
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7   // Teensy 4 ignores this, uses pin 11
#define SDCARD_SCK_PIN   14  // Teensy 4 ignores this, uses pin 13

// Built in SD card on Teensy 3.5, 3.6 & 4.1
//#define SDCARD_CS_PIN    BUILTIN_SDCARD
//#define SDCARD_MOSI_PIN  11  // not actually used
//#define SDCARD_SCK_PIN   13  // not actually used

// SD card on Teensy 4.x Audio Shield (Rev D)
//#define SDCARD_CS_PIN    10
//#define SDCARD_MOSI_PIN  11
//#define SDCARD_SCK_PIN   13

void setup() {
  AudioMemory(40);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.6);
  mixer1.gain(0, 0.167);
  mixer1.gain(1, 0.167);
  mixer1.gain(2, 0.167);
  mixer1.gain(3, 0.167);
  mixer2.gain(0, 1.0);
  mixer2.gain(1, 0.167);
  mixer2.gain(2, 0.167);

  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    // stop here if no SD card, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
}

AudioPlaySdWav * const playerlist[6] = {
  &playSdWav1, &playSdWav2, &playSdWav3, &playSdWav4, &playSdWav5, &playSdWav6
};

void playNumber(int n)
{
  String filename = String("NUM") + n + ".WAV";
  Serial.print("Playing File: ");
  Serial.println(filename);
  Serial.flush();
  playerlist[n % 6]->play(filename.c_str());
}

void loop() {
  for (int i=1; i <= 6; i++) {
    playNumber(i);
    delay(milliseconds);
  }
}
