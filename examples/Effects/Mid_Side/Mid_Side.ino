#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioInputI2S            i2s1;           //xy=109,157
AudioEffectMidSide       ms_enc1;        //xy=243,157
AudioFilterBiquad        biquad1;        //xy=265,246
AudioMixer4              mixer1;         //xy=355,73
AudioMixer4              mixer2;         //xy=400,268
AudioEffectMidSide       ms_dec1;        //xy=487,154
AudioOutputI2S           i2s2;           //xy=637,154
AudioConnection          patchCord1(i2s1, 0, ms_enc1, 0);
AudioConnection          patchCord2(i2s1, 1, ms_enc1, 1);
AudioConnection          patchCord3(ms_enc1, 0, mixer1, 0);
AudioConnection          patchCord4(ms_enc1, 1, biquad1, 0);
AudioConnection          patchCord5(biquad1, 0, mixer2, 0);
AudioConnection          patchCord6(mixer1, 0, ms_dec1, 0);
AudioConnection          patchCord7(mixer2, 0, ms_dec1, 1);
AudioConnection          patchCord8(ms_dec1, 0, i2s2, 0);
AudioConnection          patchCord9(ms_dec1, 1, i2s2, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=119,90
// GUItool: end automatically generated code


const int myInput = AUDIO_INPUT_LINEIN;
// const int myInput = AUDIO_INPUT_MIC;

void setup() {
  // The stereo line input is encoded into mid and side components.
  // The mid component will be attenuated (mixer1), which leaves some 
  // headroom for the side component to be increased in volume (mixer2).
  // Furthermore, the side component is high-passed (biquad1).
  AudioMemory(6);
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(myInput);
  sgtl5000_1.volume(1.0); // output volume

  ms_enc1.encode();
  ms_dec1.decode();

  // We attenuate the MID channel a little to prevent saturation when increasing the SIDE channel gain
  mixer1.gain(0, 0.9);
  // We increase the gain of the SIDE channel to increase stereo width
  mixer2.gain(0, 2.0);
  // But, we remove low frequencies from the side channel. Better for sub and doesn't get your ears twinkling with out-of-phase basses using a headphone
  biquad1.setHighpass(0, 200, 0.7);
  
  Serial.begin(9600);
  while (!Serial) ;
  delay(3000);
  Serial.println("Initialized mid-side example");
}

void loop() {
}

