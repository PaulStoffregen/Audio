#include <Audio.h>
#include <Wire.h>
#include <SD.h>


// Create the Audio components.  These should be created in the
// order data flows, inputs/sources -> processing -> outputs
//
AudioInputAnalog analogPinInput(A2); // analog A2 (pin 16)
AudioOutputI2S   audioOutput;        // audio shield: headphones & line-out
AudioOutputPWM   pwmOutput;          // audio output with PWM on pins 3 & 4

// Create Audio connections between the components
//
AudioConnection c1(analogPinInput, 0, audioOutput, 0);
AudioConnection c2(analogPinInput, 0, audioOutput, 1);
AudioConnection c3(analogPinInput, 0, pwmOutput, 0);

// Create an object to control the audio shield.
// 
AudioControlSGTL5000 audioShield;


void setup() {
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(12);

  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.volume(0.6);
}

void loop() {
  // Do nothing here.  The Audio flows automatically

  // When AudioInputAnalog is running, analogRead() must NOT be used.
}


