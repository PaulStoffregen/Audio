/* Play a flute sound when a button is pressed.

   Connect a pushbutton to pin 1 and pots to pins A2 & A3.
   The audio tutorial kit is the intended hardware:
     https://www.pjrc.com/store/audio_tutorial_kit.html

   Without pots connected, this program will play a very
   strange sound due to rapid random fluctuation of the
   pitch and volume!

   Requires Teensy 3.2 or higher.
   Requires Audio Shield: https://www.pjrc.com/store/teensy3_audio.html
*/

#include <Bounce.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include "Flute_100kbyte_samples.h"

AudioSynthWavetable wavetable;
AudioOutputI2S i2s1;
AudioMixer4 mixer;
AudioConnection patchCord1(wavetable, 0, mixer, 0);
AudioConnection          patchCord2(mixer, 0, i2s1, 0);
AudioConnection          patchCord3(mixer, 0, i2s1, 1);
AudioControlSGTL5000 sgtl5000_1;

// Bounce objects to read pushbuttons 
Bounce button1 = Bounce(1, 15);  // 15 ms debounce time

void setup() { 
  Serial.begin(115200);
  pinMode(1, INPUT_PULLUP);
  AudioMemory(20);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.8);
  mixer.gain(0, 0.7);
  
  wavetable.setInstrument(Flute_100kbyte);
  wavetable.amplitude(1);
}

bool playing = false;

void loop() {
  // Update all the button objects
  button1.update();
  //Read knob values
  int knob1 = analogRead(A3);
  int knob2 = analogRead(A2);
  //Get frequency and gain from knobs (Flute range is 261 to 2100 Hz)
  float freq = 261.0 + (float)knob1/1023.0 * (2100.0 - 261.0);
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
