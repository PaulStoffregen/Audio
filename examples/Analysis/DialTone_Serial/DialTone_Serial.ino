// Dial Tone (DTMF) decoding example.
//
// The audio with dial tones is connected to audio shield
// Left Line-In pin.  Dial tone output is produced on the
// Line-Out and headphones.
//
// Use the Arduino Serial Monitor to watch for incoming
// dial tones, and to send digits to be played as dial tones.
//
// This example code is in the public domain.


#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// Create the Audio components.  These should be created in the
// order data flows, inputs/sources -> processing -> outputs
//
AudioInputI2S            audioIn;
AudioAnalyzeToneDetect   row1;     // 7 tone detectors are needed
AudioAnalyzeToneDetect   row2;     // to receive DTMF dial tones
AudioAnalyzeToneDetect   row3;
AudioAnalyzeToneDetect   row4;
AudioAnalyzeToneDetect   column1;
AudioAnalyzeToneDetect   column2;
AudioAnalyzeToneDetect   column3;
AudioSynthWaveformSine   sine1;    // 2 sine wave
AudioSynthWaveformSine   sine2;    // to create DTMF
AudioMixer4              mixer;
AudioOutputI2S           audioOut;

// Create Audio connections between the components
//
AudioConnection patchCord01(audioIn, 0, row1, 0);
AudioConnection patchCord02(audioIn, 0, row2, 0);
AudioConnection patchCord03(audioIn, 0, row3, 0);
AudioConnection patchCord04(audioIn, 0, row4, 0);
AudioConnection patchCord05(audioIn, 0, column1, 0);
AudioConnection patchCord06(audioIn, 0, column2, 0);
AudioConnection patchCord07(audioIn, 0, column3, 0);
AudioConnection patchCord10(sine1, 0, mixer, 0);
AudioConnection patchCord11(sine2, 0, mixer, 1);
AudioConnection patchCord12(mixer, 0, audioOut, 0);
AudioConnection patchCord13(mixer, 0, audioOut, 1);

// Create an object to control the audio shield.
// 
AudioControlSGTL5000 audioShield;


void setup() {
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(12);

  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.volume(0.5);
  
  while (!Serial) ;
  delay(100);
  
  // Configure the tone detectors with the frequency and number
  // of cycles to match.  These numbers were picked for match
  // times of approx 30 ms.  Longer times are more precise.
  row1.frequency(697, 21);
  row2.frequency(770, 23);
  row3.frequency(852, 25);
  row4.frequency(941, 28);
  column1.frequency(1209, 36);
  column2.frequency(1336, 40);
  column3.frequency(1477, 44);
}

const float row_threshold = 0.2;
const float column_threshold = 0.2;

void loop() {
  float r1, r2, r3, r4, c1, c2, c3;
  char digit=0;

  // read all seven tone detectors
  r1 = row1.read();
  r2 = row2.read();
  r3 = row3.read();
  r4 = row4.read();
  c1 = column1.read();
  c2 = column2.read();
  c3 = column3.read();

  // print the raw data, for troubleshooting
  Serial.print("tones: ");
  Serial.print(r1);
  Serial.print(", ");
  Serial.print(r2);
  Serial.print(", ");
  Serial.print(r3);
  Serial.print(", ");
  Serial.print(r4);
  Serial.print(",   ");
  Serial.print(c1);
  Serial.print(", ");
  Serial.print(c2);
  Serial.print(", ");
  Serial.print(c3);

  // check all 12 combinations for key press
  if (r1 >= row_threshold) {
    if (c1 > column_threshold) {
      digit = '1';
    } else if (c2 > column_threshold) {
      digit = '2';
    } else if (c3 > column_threshold) {
      digit = '3';
    }
  } else if (r2 >= row_threshold) { 
    if (c1 > column_threshold) {
      digit = '4';
    } else if (c2 > column_threshold) {
      digit = '5';
    } else if (c3 > column_threshold) {
      digit = '6';
    }
  } else if (r3 >= row_threshold) { 
    if (c1 > column_threshold) {
      digit = '7';
    } else if (c2 > column_threshold) {
      digit = '8';
    } else if (c3 > column_threshold) {
      digit = '9';
    }
  } else if (r4 >= row_threshold) { 
    if (c1 > column_threshold) {
      digit = '*';
    } else if (c2 > column_threshold) {
      digit = '0';
    } else if (c3 > column_threshold) {
      digit = '#';
    }
  }

  // print the key, if any found
  if (digit > 0) {
    Serial.print("  --> Key: ");
    Serial.print(digit);
  }
  Serial.println();

  // uncomment these lines to see how much CPU time
  // the tone detectors and audio library are using
  //Serial.print("CPU=");
  //Serial.print(AudioProcessorUsage());
  //Serial.print("%, max=");
  //Serial.print(AudioProcessorUsageMax());
  //Serial.print("%   ");

  // check if any data has arrived from the serial monitor
  if (Serial.available()) {
    char key = Serial.read();
    int low=0;
    int high=0;
    if (key == '1') {
      low = 697;
      high = 1209;
    } else if (key == '2') {
      low = 697;
      high = 1336;
    } else if (key == '3') {
      low = 697;
      high = 1477;
    } else if (key == '4') {
      low = 770;
      high = 1209;
    } else if (key == '5') {
      low = 770;
      high = 1336;
    } else if (key == '6') {
      low = 770;
      high = 1477;
    } else if (key == '7') {
      low = 852;
      high = 1209;
    } else if (key == '8') {
      low = 852;
      high = 1336;
    } else if (key == '9') {
      low = 852;
      high = 1477;
    } else if (key == '*') {
      low = 941;
      high = 1209;
    } else if (key == '0') {
      low = 941;
      high = 1336;
    } else if (key == '#') {
      low = 941;
      high = 1477;
    }

    // play the DTMF tones, if characters send from the Arduino Serial Monitor
    if (low > 0 && high > 0) {
      Serial.print("Output sound for key ");
      Serial.print(key);
      Serial.print(", low freq=");
      Serial.print(low);
      Serial.print(", high freq=");
      Serial.print(high);
      Serial.println();
      AudioNoInterrupts();  // disable audio library momentarily
      sine1.frequency(low);
      sine1.amplitude(0.4);
      sine2.frequency(high);
      sine2.amplitude(0.45);
      AudioInterrupts();    // enable, both tones will start together
      delay(100);           // let the sound play for 0.1 second
      AudioNoInterrupts();
      sine1.amplitude(0);
      sine2.amplitude(0);
      AudioInterrupts();
      delay(50);            // make sure we have 0.05 second silence after
    }
  }

  delay(25);
}


