// Advanced Microcontroller-based Audio Workshop
// 
// Part 2-1: Using the Microphone


///////////////////////////////////
// copy the Design Tool code here
///////////////////////////////////




void setup() {
  Serial.begin(9600);
  AudioMemory(8);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);
  sgtl5000_1.inputSelect(AUDIO_INPUT_MIC);
  sgtl5000_1.micGain(36);
  delay(1000);
}

void loop() {
  // do nothing
}




