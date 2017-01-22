// Advanced Microcontroller-based Audio Workshop
//
// http://www.pjrc.com/store/audio_tutorial_kit.html
// https://hackaday.io/project/8292-microcontroller-audio-workshop-had-supercon-2015
// 
// Part 1-2: Test Hardware
//
// Simple beeping is pre-loaded on the Teensy, so
// it will create sound and print info to the serial
// monitor when plugged into a PC.
//
// This program is supposed to be pre-loaded before
// the workshop, so Teensy+Audio will beep when
// plugged in.

#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>
#include <Bounce.h>

AudioSynthWaveform    waveform1;
AudioOutputI2S        i2s1;
AudioConnection       patchCord1(waveform1, 0, i2s1, 0);
AudioConnection       patchCord2(waveform1, 0, i2s1, 1);
AudioControlSGTL5000  sgtl5000_1;

Bounce button0 = Bounce(0, 15);
Bounce button1 = Bounce(1, 15);
Bounce button2 = Bounce(2, 15);

int count=1;
int a1history=0, a2history=0, a3history=0;

void setup() {
  AudioMemory(10);
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  Serial.begin(115200);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.3);
  waveform1.begin(WAVEFORM_SINE);
  delay(1000);
  button0.update();
  button1.update();
  button2.update();
  a1history = analogRead(A1);
  a2history = analogRead(A2);
  a3history = analogRead(A3);
}




void loop() {
  Serial.print("Beep #");
  Serial.println(count);
  count = count + 1;
  waveform1.frequency(440);
  waveform1.amplitude(0.9);
  wait(250);
  waveform1.amplitude(0);
  wait(1750);
}

void wait(unsigned int milliseconds)
{
  elapsedMillis msec=0;

  while (msec <= milliseconds) {
    button0.update();
    button1.update();
    button2.update();
    if (button0.fallingEdge()) Serial.println("Button (pin 0) Press");
    if (button1.fallingEdge()) Serial.println("Button (pin 1) Press");
    if (button2.fallingEdge()) Serial.println("Button (pin 2) Press");
    if (button0.risingEdge()) Serial.println("Button (pin 0) Release");
    if (button1.risingEdge()) Serial.println("Button (pin 1) Release");
    if (button2.risingEdge()) Serial.println("Button (pin 2) Release");
    int a1 = analogRead(A1);
    int a2 = analogRead(A2);
    int a3 = analogRead(A3);
    if (a1 > a1history + 50 || a1 < a1history - 50) {
      Serial.print("Knob (pin A1) = ");
      Serial.println(a1);
      a1history = a1;
    }
    if (a2 > a2history + 50 || a2 < a2history - 50) {
      Serial.print("Knob (pin A2) = ");
      Serial.println(a2);
      a2history = a2;
    }
    if (a3 > a3history + 50 || a3 < a3history - 50) {
      Serial.print("Knob (pin A3) = ");
      Serial.println(a3);
      a3history = a3;
    }
  }
}


