/* Tone example using SGTL5000 DAP PEQ filters and calcBiquad filter calculator routine.

This example code is in the public domain
*/
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

const int myInput = AUDIO_INPUT_LINEIN;
// const int myInput = AUDIO_INPUT_MIC;

int updateFilter[5];
 
AudioInputI2S        audioInput;         // audio shield: mic or line-in
AudioOutputI2S       audioOutput;        // audio shield: headphones & line-out

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
  // Enable the audio shield, select the input and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(0.5);
  audioShield.audioPostProcessorEnable(); // enable the DAP block in SGTL5000
  // audioShield.eqSelect(1); // using PEQ Biquad filters
  // audioShield.eqFilterCount(2); // enable filter 0 & filter 1
  calcBiquad(FILTER_PARAEQ,110,0,0.2,524288,44100,updateFilter); // automation negates the need
  audioShield.eqFilter(0,updateFilter); // for the three lines commented out above.
  calcBiquad(FILTER_PARAEQ,4400,0,0.167,524288,44100,updateFilter);
  audioShield.eqFilter(1,updateFilter);
}

elapsedMillis chgMsec=0;
float tone1=0;

void loop() {
  // every 10 ms, check for adjustment
  if (chgMsec > 10) {
    
    float tone2=analogRead(15);
    tone2=floor(((tone2-512)/512)*70)/10;
    if(tone2!=tone1)
    {
      // calcBiquad(FilterType,FrequencyC,dBgain,Q,QuantizationUnit,SampleRate,int*);
      calcBiquad(FILTER_PARAEQ,110,-tone2,0.2,524288,44100,updateFilter);
      audioShield.eqFilter(0,updateFilter);
      calcBiquad(FILTER_PARAEQ,4400,tone2,0.167,524288,44100,updateFilter);
      audioShield.eqFilter(1,updateFilter);
      tone1=tone2;
    }
    chgMsec = 0;
  }
}

