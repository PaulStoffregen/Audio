/* HP balance example: Will influence only HP output.

This example code is in the public domain
*/

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

const int myInput = AUDIO_INPUT_LINEIN;
// const int myInput = AUDIO_INPUT_MIC;

// Create the Audio components.  These should be created in the
// order data flows, inputs/sources -> processing -> outputs
//

AudioInputI2S       audioInput;         // audio shield: mic or line-in
AudioOutputI2S      audioOutput;        // audio shield: headphones & line-out

// Create Audio connections between the components

// Just connecting in to out
AudioConnection c1(audioInput, 0, audioOutput, 0); // left connection
AudioConnection c2(audioInput, 1, audioOutput, 1); // right connection

// Create an object to control the audio shield.
// 
AudioControlSGTL5000 audioShield;

void setup() {
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(4);
  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(0.5);
}

elapsedMillis chgMsec=0;

float lastBal=1024;
float vol1=0.75;

void loop() {
  // every 10 ms, check for adjustment
  if (chgMsec > 10) {
    float bal1=analogRead(15);
    bal1=((bal1-512)/512);
    bal1=(int)bal1;
    if(lastBal!=bal1)
    {
      if(bal1<0)
      {
        audioShield.volume(vol1,vol1*(1+bal1));
      } else {
        audioShield.volume(vol1*(1-bal1),vol1);
      }
      lastBal=bal1;
    }
    chgMsec = 0;
  }
}

