#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>

AudioPlaySdWav           playSdWav1;     //xy=163,135
AudioMixer4              mixer1;         //xy=332,167
AudioEffectGranular      granular1;      //xy=504,155
AudioOutputI2S           i2s1;           //xy=664,185
AudioConnection          patchCord1(playSdWav1, 0, mixer1, 0);
AudioConnection          patchCord2(playSdWav1, 1, mixer1, 1);
AudioConnection          patchCord3(mixer1, granular1);
AudioConnection          patchCord4(granular1, 0, i2s1, 0);
AudioConnection          patchCord5(granular1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=236,248

Bounce button0 = Bounce(0, 15);
Bounce button1 = Bounce(1, 15);
Bounce button2 = Bounce(2, 15);

#define GRANULAR_MEMORY_SIZE 6000
int16_t granularMemory[GRANULAR_MEMORY_SIZE];

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

#define NUM_FILES  4
const char *filenames[NUM_FILES]={"SDTEST1.WAV", "SDTEST2.WAV", "SDTEST3.WAV", "SDTEST4.WAV"};
int nextfile=0;

void setup() {
  Serial.begin(9600);
  AudioMemory(10);

  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);

  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);

  mixer1.gain(0, 0.5);
  mixer1.gain(1, 0.5);

  // the Granular effect requires memory to operate
  granular1.begin(granularMemory, GRANULAR_MEMORY_SIZE);

  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    // stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
}

void loop() {
  if (playSdWav1.isPlaying() == false) {
    // start the next song playing
    playSdWav1.play(filenames[nextfile]);
    Serial.print("Playing: ");
    Serial.println(filenames[nextfile]);
    delay(5); // brief delay for the library read WAV info
    nextfile = nextfile + 1;
    if (nextfile >= NUM_FILES) {
      nextfile = 0;
    }
  }

  // read pushbuttons
  button0.update();
  button1.update();
  button2.update();
  // read knobs
  int knobA2 = analogRead(A2);
  int knobA3 = analogRead(A3);

  if (button0.fallingEdge()) {
    granular1.freeze(1, knobA2, knobA3);
  }
  if (button0.risingEdge()) {
    granular1.freeze(0, knobA2, knobA3);
  }

  if (button1.fallingEdge()) {
    granular1.shift(1, knobA2, knobA3);
  }
  if (button1.risingEdge()) {
    granular1.shift(0, knobA2, knobA3);
  }

}
