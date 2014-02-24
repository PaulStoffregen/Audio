/* Tone example using AudioFilterBiquad object and calcBiquad filter calculator routine.

This example code is in the public domain
*/

#include <Audio.h>
#include <Wire.h>
#include <SD.h>

const int myInput = AUDIO_INPUT_LINEIN;
// const int myInput = AUDIO_INPUT_MIC;

int BassFilter_L[]={0,0,0,0,0,0,0,0};
int BassFilter_R[]={0,0,0,0,0,0,0,0};
int TrebFilter_L[]={0,0,0,0,0,0,0,0};
int TrebFilter_R[]={0,0,0,0,0,0,0,0};

int updateFilter[5];
 
AudioInputI2S        audioInput;         // audio shield: mic or line-in
AudioFilterBiquad    filterBass_L(BassFilter_L);
AudioFilterBiquad    filterBass_R(BassFilter_R);
AudioFilterBiquad    filterTreb_L(TrebFilter_L);
AudioFilterBiquad    filterTreb_R(TrebFilter_R);
AudioOutputI2S       audioOutput;        // audio shield: headphones & line-out

// Create Audio connections between the components
//
AudioConnection c1(audioInput,0,filterBass_L,0);
AudioConnection c2(audioInput,1,filterBass_R,0);
AudioConnection c3(filterBass_L,0,filterTreb_L,0);
AudioConnection c4(filterBass_R,0,filterTreb_R,0);
AudioConnection c5(filterTreb_L,0,audioOutput,0);
AudioConnection c6(filterTreb_R,0,audioOutput,1);

// Create an object to control the audio shield.
// 
AudioControlSGTL5000 audioShield;

void setup() {
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(12);
  // Enable the audio shield, select the input and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(75);
  audioShield.unmuteLineout();

  calcBiquad(FILTER_PARAEQ,110,0,0.2,2147483648,44100,updateFilter);
  filterBass_L.updateCoefs(updateFilter);
  filterBass_R.updateCoefs(updateFilter);
  calcBiquad(FILTER_PARAEQ,4400,0,0.167,2147483648,44100,updateFilter);
  filterTreb_L.updateCoefs(updateFilter);
  filterTreb_R.updateCoefs(updateFilter);
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
      filterBass_L.updateCoefs(updateFilter);
      filterBass_R.updateCoefs(updateFilter);
      calcBiquad(FILTER_PARAEQ,4400,tone2,0.167,2147483648,44100,updateFilter);
      filterTreb_L.updateCoefs(updateFilter);
      filterTreb_R.updateCoefs(updateFilter);
      tone1=tone2;
    }
    chgMsec = 0;
  }
}

