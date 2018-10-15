#include <Audio.h>

AudioInputAnalogStereo  audioInput(PIN_MIC, 0);
AudioEffectMidSide       ms_enc1;        //xy=243,157
AudioFilterBiquad        biquad1;        //xy=265,246
AudioMixer4              mixer1;         //xy=355,73
AudioMixer4              mixer2;         //xy=400,268
AudioEffectMidSide       ms_dec1;        //xy=487,154
AudioOutputAnalogStereo  audioOutput; 
AudioConnection          patchCord1(audioInput, 0, ms_enc1, 0);
AudioConnection          patchCord2(audioInput, 0, ms_enc1, 1);
AudioConnection          patchCord3(ms_enc1, 0, mixer1, 0);
AudioConnection          patchCord4(ms_enc1, 1, biquad1, 0);
AudioConnection          patchCord5(biquad1, 0, mixer2, 0);
AudioConnection          patchCord6(mixer1, 0, ms_dec1, 0);
AudioConnection          patchCord7(mixer2, 0, ms_dec1, 1);
AudioConnection          patchCord8(ms_dec1, 0, audioOutput, 0);
AudioConnection          patchCord9(ms_dec1, 1, audioOutput, 1);

void setup() {
  // The stereo line input is encoded into mid and side components.
  // The mid component will be attenuated (mixer1), which leaves some 
  // headroom for the side component to be increased in volume (mixer2).
  // Furthermore, the side component is high-passed (biquad1).
  AudioMemory(6);

  ms_enc1.encode();
  ms_dec1.decode();

  // We attenuate the MID channel a little to prevent saturation when increasing the SIDE channel gain
  mixer1.gain(0, 0.9);
  // We increase the gain of the SIDE channel to increase stereo width
  mixer2.gain(0, 2.0);
  // But, we remove low frequencies from the side channel. Better for sub and doesn't get your ears twinkling with out-of-phase basses using a headphone
  biquad1.setHighpass(0, 200, 0.7);
  
  Serial.begin(9600);
  delay(3000);
  Serial.println("Initialized mid-side example");
}

void loop() {
}

