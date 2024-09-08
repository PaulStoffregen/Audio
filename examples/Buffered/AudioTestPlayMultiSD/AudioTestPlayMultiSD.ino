// Play back sounds from multiple SD cards
// Tested on Teensy 4.1, changes may be needed for 3.x

#include <SD.h>
#include <Bounce.h>
#include <Audio.h>

#define SDCARD_CS_PIN 10 // audio adaptor

// #define sd1 SD // can do this for one card
SDClass sd1;
SDClass sd2;
File frec;

// GUItool: begin automatically generated code
AudioInputI2S            i2s2;           //xy=105,63
AudioAnalyzePeak         peak1;          //xy=278,108
AudioRecordQueue         queue1;         //xy=281,63
AudioPlayWAVstereo           playRaw1;       //xy=302,157
AudioPlayWAVstereo           playRaw2;       //xy=302,157
AudioOutputI2S           i2s1;           //xy=470,120
AudioConnection          patchCord1(i2s2, 0, queue1, 0);
AudioConnection          patchCord2(i2s2, 0, peak1, 0);
AudioConnection          patchCord3(playRaw1, 0, i2s1, 0);
AudioConnection          patchCord4(playRaw2, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=265,212
// GUItool: end automatically generated code

// Bounce objects to easily and reliably read the buttons
Bounce buttonPlay1 =   Bounce(30, 8);
Bounce buttonPlay2 =   Bounce(31, 8);

//==================================================================
void setup() {

  // Configure the pushbutton pins
  pinMode(30, INPUT_PULLUP);
  pinMode(31, INPUT_PULLUP);

  // Audio connections require memory
  AudioMemory(10);

  while (!Serial)
    ;
  Serial.println("Started!");

  // Enable the audio shield, select input, and enable output
  //sgtl5000_1.setAddress(HIGH);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.07);

  playRaw1.createBuffer(2048,AudioBuffer::inHeap);
  playRaw2.createBuffer(2048,AudioBuffer::inHeap);

  // initialize the internal Teensy 4.1 SD card
  if (!sd1.begin(BUILTIN_SDCARD)) {
    Serial.println("error sd1.begin");
  }

  // initialize the Audio Adapter SD card
  if (!sd2.begin(SDCARD_CS_PIN)) {
    Serial.println("error sd2.begin");
  }
}


//==================================================================
void loop() {
  buttonPlay1.update();
  buttonPlay2.update();

  if (buttonPlay1.fallingEdge()) {
    Serial.println("Play 1");
    // playRaw1.play("sine220.wav"); // if sd1 is #defined as SD, this works
    playRaw1.play("sine220.wav", sd1);
  }
  if (buttonPlay2.fallingEdge()) {
    Serial.println("Play 2");
    playRaw2.play("sine330.wav", sd2);
  }
}
