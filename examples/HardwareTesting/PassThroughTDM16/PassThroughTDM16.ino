#include <Audio.h>

// GUItool: begin automatically generated code
AudioInputTDM            tdm1;           //xy=266,244
AudioOutputTDM           tdm2;           //xy=460,251
AudioConnection          patchCord1(tdm1, 0, tdm2, 0);
AudioConnection          patchCord2(tdm1, 1, tdm2, 1);
AudioConnection          patchCord3(tdm1, 2, tdm2, 2);
AudioConnection          patchCord4(tdm1, 3, tdm2, 3);
AudioConnection          patchCord5(tdm1, 4, tdm2, 4);
AudioConnection          patchCord6(tdm1, 5, tdm2, 5);
AudioConnection          patchCord7(tdm1, 6, tdm2, 6);
AudioConnection          patchCord8(tdm1, 7, tdm2, 7);
AudioConnection          patchCord9(tdm1, 8, tdm2, 8);
AudioConnection          patchCord10(tdm1, 9, tdm2, 9);
AudioConnection          patchCord11(tdm1, 10, tdm2, 10);
AudioConnection          patchCord12(tdm1, 11, tdm2, 11);
AudioConnection          patchCord13(tdm1, 12, tdm2, 12);
AudioConnection          patchCord14(tdm1, 13, tdm2, 13);
AudioConnection          patchCord15(tdm1, 14, tdm2, 14);
AudioConnection          patchCord16(tdm1, 15, tdm2, 15);
AudioControlCS42448      cs42448_1;      //xy=414,425
// GUItool: end automatically generated code

void setup() {
  AudioMemory(50);
  Serial.begin(9600);
  Serial.println("TDM Passthrough");
  if (cs42448_1.enable() && cs42448_1.volume(0.75)) {
    Serial.println("configured CS42448");
  } else {
    Serial.println("failed to config CS42448");
  }
}

void loop() {
}
