// Advanced Microcontroller-based Audio Workshop
//
// http://www.pjrc.com/store/audio_tutorial_kit.html
// https://hackaday.io/project/8292-microcontroller-audio-workshop-had-supercon-2015
// 
// Part 3-2: Fourier Transform

#include <Bounce.h>
#include "AudioSampleGuitar.h"
Bounce button0 = Bounce(0, 15);
Bounce button1 = Bounce(1, 15);  // 15 = 15 ms debounce time
Bounce button2 = Bounce(2, 15);



///////////////////////////////////
// copy the Design Tool code here
///////////////////////////////////




// Use these with the Teensy Audio Shield
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14

// Use these with the Teensy 3.5 & 3.6 SD card
//#define SDCARD_CS_PIN    BUILTIN_SDCARD
//#define SDCARD_MOSI_PIN  11  // not actually used
//#define SDCARD_SCK_PIN   13  // not actually used

// Use these for the SD+Wiz820 or other adaptors
//#define SDCARD_CS_PIN    4
//#define SDCARD_MOSI_PIN  11
//#define SDCARD_SCK_PIN   13

void setup() {
  Serial.begin(9600);
  AudioMemory(10);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  mixer1.gain(0, 0.5);
  mixer1.gain(1, 0.5);
  mixer1.gain(2, 0.0);
  mixer1.gain(3, 0.0);
  // Uncomment one these to try other window functions
  // fft1024_1.windowFunction(NULL);
  // fft1024_1.windowFunction(AudioWindowBartlett1024);
  // fft1024_1.windowFunction(AudioWindowFlattop1024);
  delay(1000);
  playSdWav1.play("SDTEST1.WAV");

}


int fileNumber = 0;
const char * filenames[4] = {
  "SDTEST1.WAV", "SDTEST2.WAV", "SDTEST3.WAV", "SDTEST4.WAV"
};

int noteNumber = 0;
const float noteFrequency[12] = {
  220.00,  // A3
  233.08,  // A#3
  246.94,  // B3
  261.63,  // C4
  277.18,  // C#4
  293.66,  // D4
  311.13,  // D#4
  329.63,  // E4
  349.23,  // F4
  369.99,  // F#4
  392.00,  // G4
  415.30   // G#4
};



void loop() {

  // print Fourier Transform data to the Arduino Serial Monitor
  if (fft1024_1.available()) {
    
    Serial.print("FFT: ");
    for (int i=0; i<30; i++) {  // 0-25  -->  DC to 1.25 kHz
      float n = fft1024_1.read(i);
      printNumber(n);
    }
    Serial.println();
  }
  
  button0.update();
  button1.update();
  button2.update();

  // Left button starts playing a new song
  if (button0.fallingEdge()) {
    mixer1.gain(2, 0.0);
    mixer1.gain(3, 0.0);
    fileNumber = fileNumber + 1;
    if (fileNumber >= 4) fileNumber = 0;
    playMem1.stop();
    playSdWav1.play(filenames[fileNumber]);
    mixer1.gain(0, 0.5);
    mixer1.gain(1, 0.5);
  }

  // Middle button plays Guitar sample
  if (button1.fallingEdge()) {
    mixer1.gain(0, 0.0);
    mixer1.gain(1, 0.0);
    mixer1.gain(3, 0.0);
    playSdWav1.stop();
    playMem1.play(AudioSampleGuitar);
    mixer1.gain(2, 1.0);
  }

  // Right button plays a pure sine wave tone
  if (button2.fallingEdge()) {
    mixer1.gain(0, 0.0);
    mixer1.gain(1, 0.0);
    mixer1.gain(2, 0.0);
    playSdWav1.stop();
    playMem1.stop();
    waveform1.begin(1.0, noteFrequency[noteNumber], WAVEFORM_SINE);
    noteNumber = noteNumber + 1;
    if (noteNumber >= 12) noteNumber = 0;
    mixer1.gain(3, 1.0);
  }
  if (button2.risingEdge()) {
    waveform1.amplitude(0);
  }
/*
  if (fft1024_1.available()) {
    // each time new FFT data is available
    // print to the Arduino Serial Monitor
    Serial.print("FFT: ");
    printNumber(fft1024_1.read(0));
    printNumber(fft1024_1.read(1));
    printNumber(fft1024_1.read(2,3));
    printNumber(fft1024_1.read(4,6));
    printNumber(fft1024_1.read(7,10));
    printNumber(fft1024_1.read(11,15));
    printNumber(fft1024_1.read(16,22));
    printNumber(fft1024_1.read(23,32));
    printNumber(fft1024_1.read(33,46));
    printNumber(fft1024_1.read(47,66));
    printNumber(fft1024_1.read(67,93));
    printNumber(fft1024_1.read(94,131));
    printNumber(fft1024_1.read(132,184));
    printNumber(fft1024_1.read(185,257));
    printNumber(fft1024_1.read(258,359));
    printNumber(fft1024_1.read(360,511));
    Serial.println();
  }
*/

}


void printNumber(float n) {
  
  if (n >= 0.004) {
    Serial.print(n, 3);
    Serial.print(" ");
  } else {
    Serial.print("   -  "); // don't print "0.00"
  }
  
  /*
  if (n > 0.25) {
    Serial.print("***** ");
  } else if (n > 0.18) {
    Serial.print(" ***  ");
  } else if (n > 0.06) {
    Serial.print("  *   ");
  } else if (n > 0.005) {
    Serial.print("  .   ");
  }
  */
}
