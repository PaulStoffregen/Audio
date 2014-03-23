/* Tone example using AudioFilterBiquad object and calcBiquad filter calculator routine.

This example code is in the public domain
*/

#include <Audio.h>
#include <Wire.h>
#include <SD.h>

const int myInput = AUDIO_INPUT_LINEIN;
// const int myInput = AUDIO_INPUT_MIC;

int ToneFilter_L[]={0,0,0,0,0,0,0,0x80000000,0,0,0,0,0,0,0,0}; // defines 2 sets of coefficients, not sure max possible in
int ToneFilter_R[]={0,0,0,0,0,0,0,0x80000000,0,0,0,0,0,0,0,0}; // time frame but probably quite a few.

int updateFilter[5];
 
AudioInputI2S        audioInput;         // audio shield: mic or line-in
AudioFilterBiquad    filterTone_L(ToneFilter_L);
AudioFilterBiquad    filterTone_R(ToneFilter_R);
AudioOutputI2S       audioOutput;        // audio shield: headphones & line-out

// Create Audio connections between the components
//                                    
AudioConnection c1(audioInput,0,filterTone_L,0);
AudioConnection c2(audioInput,1,filterTone_R,0);
AudioConnection c3(filterTone_L,0,audioOutput,0);
AudioConnection c4(filterTone_R,0,audioOutput,1);

// Create an object to control the audio shield.
// 
AudioControlSGTL5000 audioShield;

void setup() {
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(6);
  // Enable the audio shield, select the input and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(0.75);
  audioShield.unmuteLineout();

  calcBiquad(FILTER_PARAEQ,110,0,0.2,2147483648,44100,updateFilter);
  filterTone_L.updateCoefs(updateFilter); // default set updateCoefs(0,updateFilter);
  filterTone_R.updateCoefs(updateFilter);
  calcBiquad(FILTER_PARAEQ,4400,0,0.167,2147483648,44100,updateFilter);
  filterTone_L.updateCoefs(1,updateFilter);
  filterTone_R.updateCoefs(1,updateFilter);
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
      calcBiquad(FILTER_PARAEQ,110,-tone2,0.2,2147483648,44100,updateFilter);
      filterTone_L.updateCoefs(updateFilter);
      filterTone_R.updateCoefs(updateFilter);
      calcBiquad(FILTER_PARAEQ,4400,tone2,0.167,2147483648,44100,updateFilter);
      filterTone_L.updateCoefs(1,updateFilter);
      filterTone_R.updateCoefs(1,updateFilter);
      tone1=tone2;
    }
    chgMsec = 0;
  }
}

