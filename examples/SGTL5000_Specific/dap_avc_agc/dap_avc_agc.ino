/* DAP AVC example; AVC is SGTL5000 equiv of AGC

This example code is in the public domain
*/

#include <Audio.h>
#include <Wire.h>
#include <SD.h>


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
  audioShield.volume(75);
  audioShield.unmuteLineout();
  // have to enable DAP to use AVC
  audioShield.dap_enable();
  // here are some settings for AVC that have a fairly obvious effect
  audioShield.dap_avc(2,1,0,-5,0.5,0.5); // see comments starting line #699 of control_sgtl5000.cpp in ./libraries/audio/
  // AVC has its own enable/disable bit
  audioShield.dap_avc_enable(); // you can use audioShield.dap_avc_enable(0); to turn off AVC
}

elapsedMillis chgMsec=0;
float lastVol=1024;

void loop() {
  // every 10 ms, check for adjustment
  if (chgMsec > 10) {
    float vol1=analogRead(15)/10.23;
    vol1=(int)vol1;
    if(lastVol!=vol1)
    {
      audioShield.volume(vol1);
      lastVol=vol1;
    }
    chgMsec = 0;
  }
}

