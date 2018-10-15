#include <Audio.h>

AudioInputAnalogStereo  audioIn(PIN_MIC, 0);
AudioOutputAnalogStereo  audioOut;        
AudioFilterBiquad        biquad1;   
AudioConnection          patchCord1(audioIn, 0, biquad1, 0);
AudioConnection          patchCord2(biquad1, 0, audioOut, 0);
AudioConnection          patchCord3(biquad1, 0, audioOut, 1);

void setup() {
  AudioMemory(12);

  // Butterworth filter, 12 db/octave
  biquad1.setLowpass(0, 800, 0.707);

  // Linkwitz-Riley filter, 48 dB/octave
  //biquad1.setLowpass(0, 800, 0.54);
  //biquad1.setLowpass(1, 800, 1.3);
  //biquad1.setLowpass(2, 800, 0.54);
  //biquad1.setLowpass(3, 800, 1.3);
}


void loop() {
}


