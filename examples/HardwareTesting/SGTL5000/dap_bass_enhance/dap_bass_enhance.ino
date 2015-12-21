/* DAP Bass enhance example SGTL5000 only

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
//
AudioConnection c1(audioInput, 0, audioOutput, 0); // left passing through
AudioConnection c2(audioInput, 1, audioOutput, 1); // right passing through

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
  // just enable it to use default settings.
  audioShield.audioPostProcessorEnable();
  audioShield.enhanceBassEnable(); // all we need to do for default bass enhancement settings.
  // audioShield.enhanceBass((float)lr_level,(float)bass_level);
  // audioShield.enhanceBass((float)lr_level,(float)bass_level,(uint8_t)hpf_bypass,(uint8_t)cutoff);
  // please see http://www.pjrc.com/teensy/SGTL5000.pdf page 50 for valid values for BYPASS_HPF and CUTOFF
}

elapsedMillis chgMsec=0;
float lastVol=1024;

void loop() {
  // every 10 ms, check for adjustment
  if (chgMsec > 10) { // more regular updates for actual changes seems better.
    float vol1=analogRead(15)/1023.0;
    vol1=(int)vol1;
    if(lastVol!=vol1)
    {
      audioShield.volume(vol1);
      lastVol=vol1;
    }
    chgMsec = 0;
  }
}

