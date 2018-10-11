// Talk into the mic and hear it through your headphones

#include <Audio.h>

AudioInputAnalogStereo  audioIn(PIN_MIC, 0);
AudioOutputAnalogStereo  audioOut;

AudioConnection patchCord41(audioIn, 0, audioOut, 0);
AudioConnection patchCord42(audioIn, 0, audioOut, 1);

void setup() {
  AudioMemory(12);
}

void loop() {

}
