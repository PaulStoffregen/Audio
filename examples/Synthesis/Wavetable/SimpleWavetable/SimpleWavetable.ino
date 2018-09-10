#include <Bounce.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include "BasicFlute1_samples.h"

AudioSynthWavetable wavetable;
AudioOutputI2S i2s1;
AudioMixer4 mixer;
AudioConnection patchCord1(wavetable, 0, mixer, 0);
AudioConnection          patchCord2(mixer, 0, i2s1, 0);
AudioConnection          patchCord3(mixer, 0, i2s1, 1);
AudioControlSGTL5000 sgtl5000_1;

// Bounce objects to read pushbuttons 
Bounce button0 = Bounce(0, 15);
Bounce button1 = Bounce(1, 15);  // 15 ms debounce time
Bounce button2 = Bounce(2, 15);

void setup() { 
  Serial.begin(115200);
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  AudioMemory(20);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.8);
  mixer.gain(0, 0.7);
  
  wavetable.setInstrument(BasicFlute1);
  wavetable.amplitude(1);
}

bool playing = false;

void loop() {
  // Update all the button objects
  button0.update();
  button1.update();
  button2.update();
  //Read knob values
  int knob1 = analogRead(A3);
  int knob2 = analogRead(A2);
  //Get frequency and gain from knobs
  float freq = (float)knob1/5.0;
  float gain = (float)knob2/1023.0;
  //Set a low-limit to the gain
  if (gain < .05) gain = .05;

  if (button1.fallingEdge()) {
    if (playing) {
      playing = false;
      wavetable.stop();
    }
    else {
      playing = true;
      wavetable.playFrequency(freq);
      wavetable.amplitude(gain);
    }    
  }
  wavetable.amplitude(gain);
  wavetable.setFrequency(freq);
}
