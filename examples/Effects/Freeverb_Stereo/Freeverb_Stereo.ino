// Freeverb - High quality reverb effect
//
//  Teensy 3.5 or higher is required to run this example
//
// The SD card may connect to different pins, depending on the
// hardware you are using.  Uncomment or configure the SD card
// pins to match your hardware.
//
// Data files to put on your SD card can be downloaded here:
//   http://www.pjrc.com/teensy/td_libs_AudioDataFiles.html
//
// This example code is in the public domain.

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioPlaySdWav           playSdWav1;     //xy=163,135
AudioMixer4              mixer1;         //xy=332,167
AudioEffectFreeverbStereo freeverbs1;     //xy=490,92
AudioMixer4              mixer3;         //xy=653,231
AudioMixer4              mixer2;         //xy=677,152
AudioOutputI2S           i2s1;           //xy=815,198
AudioConnection          patchCord1(playSdWav1, 0, mixer1, 0);
AudioConnection          patchCord2(playSdWav1, 1, mixer1, 1);
AudioConnection          patchCord3(mixer1, 0, mixer2, 1);
AudioConnection          patchCord4(mixer1, freeverbs1);
AudioConnection          patchCord5(mixer1, 0, mixer3, 1);
AudioConnection          patchCord6(freeverbs1, 0, mixer2, 0);
AudioConnection          patchCord7(freeverbs1, 1, mixer3, 0);
AudioConnection          patchCord8(mixer3, 0, i2s1, 1);
AudioConnection          patchCord9(mixer2, 0, i2s1, 0);
AudioControlSGTL5000     sgtl5000_1;     //xy=236,248
// GUItool: end automatically generated code


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

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(10);

  // Comment these out if not using the audio adaptor board.
  // This may wait forever if the SDA & SCL pins lack
  // pullup resistors
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);

  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    // stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
  mixer1.gain(0, 0.5);
  mixer1.gain(1, 0.5);
  mixer2.gain(0, 0.9); // hear 90% "wet"
  mixer2.gain(1, 0.1); // and  10% "dry"
  mixer3.gain(0, 0.9);
  mixer3.gain(1, 0.1);
}

void playFile(const char *filename)
{
  Serial.print("Playing file: ");
  Serial.println(filename);

  // Start playing the file.  This sketch continues to
  // run while the file plays.
  playSdWav1.play(filename);

  // A brief delay for the library read WAV info
  delay(5);

  elapsedMillis msec;

  // Simply wait for the file to finish playing.
  while (playSdWav1.isPlaying()) {

    // while the music plays, adjust parameters and print info
    if (msec > 250) {
      msec = 0;
      float knob_A1 = 0.9;
      float knob_A2 = 0.5;
      float knob_A3 = 0.5;
      
// Uncomment these lines to adjust parameters with analog inputs
      //knob_A1 = (float)analogRead(A1) / 1023.0;
      //knob_A2 = (float)analogRead(A2) / 1023.0;
      //knob_A3 = (float)analogRead(A3) / 1023.0;

      mixer2.gain(0, knob_A1);
      mixer2.gain(1, 1.0 - knob_A1);
      mixer3.gain(0, knob_A1);
      mixer3.gain(1, 1.0 - knob_A1);
      freeverbs1.roomsize(knob_A2);
      freeverbs1.damping(knob_A3);
      
      Serial.print("Reverb: mix=");
      Serial.print(knob_A1 * 100.0);
      Serial.print("%, roomsize=");
      Serial.print(knob_A2 * 100.0);
      Serial.print("%, damping=");
      Serial.print(knob_A3 * 100.0);
      Serial.print("%, CPU Usage=");
      Serial.print(freeverbs1.processorUsage());
      Serial.println("%");
    }
  }
}


void loop() {
  playFile("SDTEST1.WAV");  // filenames are always uppercase 8.3 format
  delay(500);
  playFile("SDTEST2.WAV");
  delay(500);
  playFile("SDTEST3.WAV");
  delay(500);
  playFile("SDTEST4.WAV");
  delay(1500);
}

