#include <Audio.h>
#include <Wire.h>
#include <SD.h>

const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;

// each filter requires a set up parameters
//  http://forum.pjrc.com/threads/24793-Audio-Library?p=40179&viewfull=1#post40179
//
int myFilterParameters[] = {  // lowpass, Fc=800 Hz, Q=0.707
  3224322, 6448644, 3224322, 1974735214, -913890679, 0, 0, 0};

// Create the Audio components.  These should be created in the
// order data flows, inputs/sources -> processing -> outputs
//
AudioInputI2S       audioInput;         // audio shield: mic or line-in
AudioFilterBiquad   myFilter(myFilterParameters);
AudioOutputI2S      audioOutput;        // audio shield: headphones & line-out

// Create Audio connections between the components
//
AudioConnection c1(audioInput, 0, audioOutput, 0);
AudioConnection c2(audioInput, 0, myFilter, 0);
AudioConnection c3(myFilter, 0, audioOutput, 1);

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


