// Audio Tutorial Kit Tester
//
// http://www.pjrc.com/store/audio_tutorial_kit.html
//
// Easily test all the tutorial hardware by only listening and pressing buttons.
//  1: listen for microphone  (any button press moves to #2)
//  2: listen for SD card playing  (any button press moves to #2)
//  3: test 3 buttons and 3 knobs  (press all 3 buttons to go back to step #1)
//
// After test is completed, EEPROM storage is used to remember the hardware is good
// Good hardware will begin at #3, which corresponds to the first tutorial example

#include <Audio.h>
#include <SD.h>
#include <Bounce.h>
#include <EEPROM.h>
#include "AudioSampleButton1.h"
#include "AudioSampleButton2.h"
#include "AudioSampleButton3.h"
#include "AudioSampleKnob1.h"
#include "AudioSampleKnob2.h"
#include "AudioSampleKnob3.h"
#include "AudioSampleNosdcard.h"

AudioInputI2S         i2s1;
AudioSynthWaveform    waveform1;
AudioPlaySdWav        playSdWav1;
AudioPlayMemory       sample1;
AudioMixer4           mixer1;
AudioMixer4           mixer2;
AudioOutputI2S        i2s2;
AudioConnection       patchCord1(i2s1, 0, mixer1, 0);
AudioConnection       patchCord2(i2s1, 0, mixer2, 0);
AudioConnection       patchCord3(playSdWav1, 0, mixer1, 1);
AudioConnection       patchCord4(playSdWav1, 1, mixer2, 1);
AudioConnection       patchCord5(waveform1, 0, mixer1, 2);
AudioConnection       patchCord6(waveform1, 0, mixer2, 2);
AudioConnection       patchCord7(sample1, 0, mixer1, 3);
AudioConnection       patchCord8(sample1, 0, mixer2, 3);
AudioConnection       patchCord9(mixer1, 0, i2s2, 0);
AudioConnection       patchCordA(mixer2, 0, i2s2, 1);
AudioControlSGTL5000  sgtl5000_1;

Bounce button0 = Bounce(0, 15);
Bounce button1 = Bounce(1, 15);
Bounce button2 = Bounce(2, 15);

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

int mode;
int count=1;
int a1=0, a2=0, a3=0;
bool anybutton=false;
bool sdcardinit=true;
bool playsamples=false;

void setup() {
  mode = EEPROM.read(400);
  AudioMemory(20);
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  Serial.begin(115200);
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);
  sgtl5000_1.inputSelect(AUDIO_INPUT_MIC);
  sgtl5000_1.micGain(36);
  mixer1.gain(0, 0);
  mixer1.gain(1, 0);
  mixer1.gain(2, 0);
  mixer1.gain(3, 0.4);
  mixer2.gain(0, 0);
  mixer2.gain(1, 0);
  mixer2.gain(2, 0);
  mixer2.gain(3, 0.4);
  waveform1.begin(WAVEFORM_SINE);
  delay(1000);
  button0.update();
  button1.update();
  button2.update();
  a1 = analogRead(A1);
  a2 = analogRead(A2);
  a3 = analogRead(A3);
}

void update() {
  static int state=0;
  
  button0.update();
  button1.update();
  button2.update();
  anybutton = false;
  if (button0.fallingEdge()) {
    anybutton = true;
    Serial.println("Button (pin 0) Press");
    if (playsamples) sample1.play(AudioSampleButton1);
  }
  if (button1.fallingEdge()) {
    anybutton = true;
    Serial.println("Button (pin 1) Press");
    if (playsamples) sample1.play(AudioSampleButton2);
  }
  if (button2.fallingEdge()) {
    anybutton = true;
    Serial.println("Button (pin 2) Press");
    if (playsamples) sample1.play(AudioSampleButton3);
  }
  if (button0.risingEdge()) {
    Serial.println("Button (pin 0) Release");
  }
  if (button1.risingEdge()) {
    Serial.println("Button (pin 1) Release");
  }
  if (button2.risingEdge()) {
    Serial.println("Button (pin 2) Release");
  }
  if (state == 0) {
    int a = analogRead(A1);
    if (a > a1 + 50 || a < a1 - 50) {
      Serial.print("Knob (pin A1) = ");
      Serial.println(a);
      if (playsamples && !sample1.isPlaying()) sample1.play(AudioSampleKnob1);
      a1 = a;
    }
    state = 1;
  } else if (state == 1) {
    int a = analogRead(A2);
    if (a > a2 + 50 || a < a2 - 50) {
      Serial.print("Knob (pin A2) = ");
      Serial.println(a);
      if (playsamples && !sample1.isPlaying()) sample1.play(AudioSampleKnob2);
      a2 = a;
    }
    state = 2;
  } else {
    int a = analogRead(A3);
    if (a > a3 + 50 || a < a3 - 50) {
      Serial.print("Knob (pin A3) = ");
      Serial.println(a);
      if (playsamples && !sample1.isPlaying()) sample1.play(AudioSampleKnob3);
      a3 = a;
    }
    state = 0;
  }
}

elapsedMillis msec=0;

void loop() {
  update();

  // Test microphone
  if (mode == 255) {
    playsamples = true;
    mixer1.gain(0, 1.0);
    mixer2.gain(0, 1.0);
    if (anybutton) {
      mixer1.gain(0, 0);
      mixer2.gain(0, 0);
      if (sdcardinit) {
        if (!(SD.begin(SDCARD_CS_PIN))) {
          while (1) {
            Serial.println("Unable to access the SD card");
            if (playsamples) sample1.play(AudioSampleNosdcard);
            delay(3500);
          }
        }
        sdcardinit = false;
      }
      mode = 123;
    }

  // Play WAV file (test SD card, sound quality)
  } else if (mode == 123) {  
    mixer1.gain(1, 0.75);
    mixer2.gain(1, 0.75);
    if (playSdWav1.isPlaying() == false) {
      Serial.println("Start playing");
      playSdWav1.play("SDTEST2.WAV");
      delay(10); // wait for library to parse WAV info
    }
    if (anybutton) {
      playSdWav1.stop();
      mixer1.gain(1, 0);
      mixer2.gain(1, 0);
      mode = 45;
      EEPROM.write(400, mode);
    }
      
  // Beeping (test buttons & knobs)
  } else {
    mixer1.gain(2, 1.0);
    mixer2.gain(2, 1.0);
    if (mode == 45) {
      Serial.print("Beep #");
      Serial.println(count);
      count = count + 1;
      waveform1.frequency(440);
      waveform1.amplitude(0.35);
      msec = 0;
      mode = 46;
    } else if (mode == 46) {
      if (msec > 250) {
        waveform1.amplitude(0);
        msec = 0;
        mode = 47;
      }
    } else {
      if (msec > 1750) {
        mode = 45;
      }
    }
    if (button0.read() == LOW && button1.read() == LOW && button2.read() == LOW) {
      mixer1.gain(2, 0);
      mixer2.gain(2, 0);
      mode = 255;
    }
  }
}



