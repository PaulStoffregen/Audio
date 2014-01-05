#include <Audio.h>
#include <Wire.h>
#include <SD.h>

const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;

// Create the Audio components.  These should be created in the
// order data flows, inputs/sources -> processing -> outputs
//
//AudioInputAnalog analogPinInput(16); // analog A2 (pin 16)
AudioInputI2S    audioInput;         // audio shield: mic or line-in
AudioOutputI2S   audioOutput;        // audio shield: headphones & line-out
AudioOutputPWM   pwmOutput;          // audio output with PWM on pins 3 & 4

// Create Audio connections between the components
//
AudioConnection c1(audioInput, 0, audioOutput, 0);
AudioConnection c2(audioInput, 1, audioOutput, 1);
AudioConnection c5(audioInput, 0, pwmOutput, 0);

// Create an object to control the audio shield.
// 
AudioControlSGTL5000 audioShield;


void setup() {
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(12);

  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(60);
}

elapsedMillis volmsec=0;

void loop() {
  // every 50 ms, adjust the volume
  if (volmsec > 50) {
    float vol = analogRead(15);
    vol = vol / 10.24;
    audioShield.volume(vol);
    volmsec = 0;
  }
}


