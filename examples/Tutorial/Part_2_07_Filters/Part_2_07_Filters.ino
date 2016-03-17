// Advanced Microcontroller-based Audio Workshop
//
// https://github.com/PaulStoffregen/AudioWorkshop2015/raw/master/workshop.pdf
// https://hackaday.io/project/8292-microcontroller-audio-workshop-had-supercon-2015
// 
// Part 2-7: Filters

#include <Bounce.h>


///////////////////////////////////
// copy the Design Tool code here
///////////////////////////////////





// Bounce objects to read pushbuttons 
Bounce button0 = Bounce(0, 15);
Bounce button1 = Bounce(1, 15);  // 15 ms debounce time
Bounce button2 = Bounce(2, 15);

void setup() {
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  Serial.begin(9600);
  AudioMemory(12);
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
  mixer1.gain(0, 0.0);  
  mixer1.gain(1, 1.0);  // default to hearing band-pass signal
  mixer1.gain(2, 0.0);
  mixer1.gain(3, 0.0);
  mixer2.gain(0, 0.0);
  mixer2.gain(1, 1.0);
  mixer2.gain(2, 0.0);
  mixer2.gain(3, 0.0);
  delay(1000);
}

void loop() {
  if (playSdWav1.isPlaying() == false) {
    Serial.println("Start playing");
    playSdWav1.play("SDTEST3.WAV");
    delay(10); // wait for library to parse WAV info
  }

  // Update all the button objects
  button0.update();
  button1.update();
  button2.update();
  
  if (button0.fallingEdge()) {
    Serial.println("Low Pass Signal");
    mixer1.gain(0, 1.0);  // hear low-pass signal
    mixer1.gain(1, 0.0);
    mixer1.gain(2, 0.0);
    mixer2.gain(0, 1.0);
    mixer2.gain(1, 0.0);
    mixer2.gain(2, 0.0);
  }
  if (button1.fallingEdge()) {
    Serial.println("Band Pass Signal");
    mixer1.gain(0, 0.0);
    mixer1.gain(1, 1.0);  // hear low-pass signal
    mixer1.gain(2, 0.0);
    mixer2.gain(0, 0.0);
    mixer2.gain(1, 1.0);
    mixer2.gain(2, 0.0);
  }
  if (button2.fallingEdge()) {
    Serial.println("High Pass Signal");
    mixer1.gain(0, 0.0);
    mixer1.gain(1, 0.0);
    mixer1.gain(2, 1.0);  // hear low-pass signal
    mixer2.gain(0, 0.0);
    mixer2.gain(1, 0.0);
    mixer2.gain(2, 1.0);
  }

  // read the knob and adjust the filter frequency
  int knob = analogRead(A3);
  // quick and dirty equation for exp scale frequency adjust
  float freq =  expf((float)knob / 150.0) * 10.0 + 80.0;
  filter1.frequency(freq);
  filter2.frequency(freq);
  Serial.print("frequency = ");
  Serial.println(freq);
  delay(200);
}




