// Advanced Microcontroller-based Audio Workshop
//
// http://www.pjrc.com/store/audio_tutorial_kit.html
// https://hackaday.io/project/8292-microcontroller-audio-workshop-had-supercon-2015
// 
// Part 2-5: Simple Delay


///////////////////////////////////
// copy the Design Tool code here
///////////////////////////////////





void setup() {
  Serial.begin(9600);
  AudioMemory(160);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.6);
  sgtl5000_1.inputSelect(AUDIO_INPUT_MIC);
  sgtl5000_1.micGain(36);
  mixer1.gain(0, 0.2);
  mixer1.gain(1, 0.2);
  mixer1.gain(2, 0.2);
  mixer1.gain(3, 0.2);
  mixer2.gain(0, 0.2);
  mixer2.gain(1, 0.2);
  mixer2.gain(2, 0.2);
  mixer2.gain(3, 0.2);
  mixer3.gain(0, 0.0); // default = do not listen to direct signal
  mixer3.gain(1, 1.0); // ch1 is output of mixer1
  mixer3.gain(2, 1.0); // ch2 is output of mixer2
  delay1.delay(0, 400);
  delay1.delay(1, 400);
  delay1.delay(2, 400);
  delay1.delay(3, 400);
  delay1.delay(4, 400);
  delay1.delay(5, 400);
  delay1.delay(6, 400);
  delay1.delay(7, 400);
  delay(1000);
}

void loop() {
  // do nothing
}




