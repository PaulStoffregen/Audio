// Advanced Microcontroller-based Audio Workshop
//
// https://github.com/PaulStoffregen/AudioWorkshop2015/raw/master/workshop.pdf
// https://hackaday.io/project/8292-microcontroller-audio-workshop-had-supercon-2015
// 
// Part 2-2: Mixers & Playing Multiple Sounds


///////////////////////////////////
// copy the Design Tool code here
///////////////////////////////////




void setup() {
  Serial.begin(9600);
  AudioMemory(8);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);
  SPI.setMOSI(7);
  SPI.setSCK(14);
  if (!(SD.begin(10))) {
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
  pinMode(13, OUTPUT); // LED on pin 13
  mixer1.gain(0, 0.5);
  mixer1.gain(1, 0.5);
  mixer2.gain(0, 0.5);
  mixer2.gain(1, 0.5);
  delay(1000);
}

void loop() {
  if (playSdWav1.isPlaying() == false) {
    Serial.println("Start playing 1");
    playSdWav1.play("SDTEST1.WAV");
    delay(10); // wait for library to parse WAV info
  }
  if (playSdWav2.isPlaying() == false) {
    Serial.println("Start playing 2");
    playSdWav2.play("SDTEST4.WAV");
    delay(10); // wait for library to parse WAV info
  }
  // uncomment this code to allow Knob A3 to pan between songs
  /*
  int knob = analogRead(A3);  // knob = 0 to 1023
  float gain1 = (float)knob / 1023.0;
  float gain2 = 1.0 - gain1;
  mixer1.gain(0, gain1);
  mixer1.gain(1, gain2);
  mixer2.gain(0, gain1);
  mixer2.gain(1, gain2);
  */
}










