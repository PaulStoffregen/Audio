// Dial Tone (DTMF) decoding example.
//
// The audio with dial tones is connected to analog input A2,
// without using the audio shield.  See the "DialTone_DTMF"
// example for using the audio shield.
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
AudioInputAnalog         audioIn;
AudioAnalyzeToneDetect   row1;     // 7 tone detectors are needed
AudioAnalyzeToneDetect   row2;     // to receive DTMF dial tones
AudioAnalyzeToneDetect   row3;
AudioAnalyzeToneDetect   row4;
AudioAnalyzeToneDetect   column1;
AudioAnalyzeToneDetect   column2;
AudioAnalyzeToneDetect   column3;

// Create Audio connections between the components
//
AudioConnection patchCord1(audioIn, 0, row1, 0);
AudioConnection patchCord2(audioIn, 0, row2, 0);
AudioConnection patchCord3(audioIn, 0, row3, 0);
AudioConnection patchCord4(audioIn, 0, row4, 0);
AudioConnection patchCord5(audioIn, 0, column1, 0);
AudioConnection patchCord6(audioIn, 0, column2, 0);
AudioConnection patchCord7(audioIn, 0, column3, 0);

// pins where the 7 segment LEDs are connected
const int sevenseg_a = 17;   //  aaa
const int sevenseg_b = 9;    // f   b
const int sevenseg_c = 11;   // f   b
const int sevenseg_d = 12;   //  ggg
const int sevenseg_e = 14;   // e   c
const int sevenseg_f = 15;   // e   c
const int sevenseg_g = 10;   //  ddd


void setup() {
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(3);
  
  //while (!Serial) ;
  //delay(100);
  
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
  
  // The 7 segment display is "common anode), where the
  // common pin connects to +3.3V.  LOW turns the LED on
  // and HIGH turns the LED off.  If you use a common
  // cathode display, you will need to change all the HIGH
  // to LOW and LOW to HIGH.
  pinMode(sevenseg_a, OUTPUT);
  pinMode(sevenseg_b, OUTPUT);
  pinMode(sevenseg_c, OUTPUT);
  pinMode(sevenseg_d, OUTPUT);
  pinMode(sevenseg_e, OUTPUT);
  pinMode(sevenseg_f, OUTPUT);
  pinMode(sevenseg_g, OUTPUT);
  digitalWrite(sevenseg_a, HIGH);
  digitalWrite(sevenseg_b, HIGH);
  digitalWrite(sevenseg_c, HIGH);
  digitalWrite(sevenseg_d, HIGH);
  digitalWrite(sevenseg_e, HIGH);
  digitalWrite(sevenseg_f, HIGH);
  digitalWrite(sevenseg_g, HIGH);
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
      digitalWrite(sevenseg_a, HIGH);
      digitalWrite(sevenseg_b, LOW);
      digitalWrite(sevenseg_c, LOW);
      digitalWrite(sevenseg_d, HIGH);
      digitalWrite(sevenseg_e, HIGH);
      digitalWrite(sevenseg_f, HIGH);
      digitalWrite(sevenseg_g, HIGH);
    } else if (c2 > column_threshold) {
      digit = '2';
      digitalWrite(sevenseg_a, LOW);
      digitalWrite(sevenseg_b, LOW);
      digitalWrite(sevenseg_c, HIGH);
      digitalWrite(sevenseg_d, LOW);
      digitalWrite(sevenseg_e, LOW);
      digitalWrite(sevenseg_f, HIGH);
      digitalWrite(sevenseg_g, LOW);
    } else if (c3 > column_threshold) {
      digit = '3';
      digitalWrite(sevenseg_a, LOW);
      digitalWrite(sevenseg_b, LOW);
      digitalWrite(sevenseg_c, LOW);
      digitalWrite(sevenseg_d, LOW);
      digitalWrite(sevenseg_e, HIGH);
      digitalWrite(sevenseg_f, HIGH);
      digitalWrite(sevenseg_g, LOW);
    }
  } else if (r2 >= row_threshold) { 
    if (c1 > column_threshold) {
      digit = '4';
      digitalWrite(sevenseg_a, HIGH);
      digitalWrite(sevenseg_b, LOW);
      digitalWrite(sevenseg_c, LOW);
      digitalWrite(sevenseg_d, HIGH);
      digitalWrite(sevenseg_e, HIGH);
      digitalWrite(sevenseg_f, LOW);
      digitalWrite(sevenseg_g, LOW);
    } else if (c2 > column_threshold) {
      digit = '5';
      digitalWrite(sevenseg_a, LOW);
      digitalWrite(sevenseg_b, HIGH);
      digitalWrite(sevenseg_c, LOW);
      digitalWrite(sevenseg_d, LOW);
      digitalWrite(sevenseg_e, HIGH);
      digitalWrite(sevenseg_f, LOW);
      digitalWrite(sevenseg_g, LOW);
    } else if (c3 > column_threshold) {
      digit = '6';
      digitalWrite(sevenseg_a, LOW);
      digitalWrite(sevenseg_b, HIGH);
      digitalWrite(sevenseg_c, LOW);
      digitalWrite(sevenseg_d, LOW);
      digitalWrite(sevenseg_e, LOW);
      digitalWrite(sevenseg_f, LOW);
      digitalWrite(sevenseg_g, LOW);
    }
  } else if (r3 >= row_threshold) { 
    if (c1 > column_threshold) {
      digit = '7';
      digitalWrite(sevenseg_a, LOW);
      digitalWrite(sevenseg_b, LOW);
      digitalWrite(sevenseg_c, LOW);
      digitalWrite(sevenseg_d, HIGH);
      digitalWrite(sevenseg_e, HIGH);
      digitalWrite(sevenseg_f, HIGH);
      digitalWrite(sevenseg_g, HIGH);
    } else if (c2 > column_threshold) {
      digit = '8';
      digitalWrite(sevenseg_a, LOW);
      digitalWrite(sevenseg_b, LOW);
      digitalWrite(sevenseg_c, LOW);
      digitalWrite(sevenseg_d, LOW);
      digitalWrite(sevenseg_e, LOW);
      digitalWrite(sevenseg_f, LOW);
      digitalWrite(sevenseg_g, LOW);
    } else if (c3 > column_threshold) {
      digit = '9';
      digitalWrite(sevenseg_a, LOW);
      digitalWrite(sevenseg_b, LOW);
      digitalWrite(sevenseg_c, LOW);
      digitalWrite(sevenseg_d, LOW);
      digitalWrite(sevenseg_e, HIGH);
      digitalWrite(sevenseg_f, LOW);
      digitalWrite(sevenseg_g, LOW);
    }
  } else if (r4 >= row_threshold) { 
    if (c1 > column_threshold) {
      digit = '*';
      digitalWrite(sevenseg_a, HIGH);
      digitalWrite(sevenseg_b, HIGH);
      digitalWrite(sevenseg_c, HIGH);
      digitalWrite(sevenseg_d, HIGH);
      digitalWrite(sevenseg_e, HIGH);
      digitalWrite(sevenseg_f, HIGH);
      digitalWrite(sevenseg_g, LOW);
    } else if (c2 > column_threshold) {
      digit = '0';
      digitalWrite(sevenseg_a, LOW);
      digitalWrite(sevenseg_b, LOW);
      digitalWrite(sevenseg_c, LOW);
      digitalWrite(sevenseg_d, LOW);
      digitalWrite(sevenseg_e, LOW);
      digitalWrite(sevenseg_f, LOW);
      digitalWrite(sevenseg_g, HIGH);
    } else if (c3 > column_threshold) {
      digit = '#';
      digitalWrite(sevenseg_a, HIGH);
      digitalWrite(sevenseg_b, LOW);
      digitalWrite(sevenseg_c, LOW);
      digitalWrite(sevenseg_d, HIGH);
      digitalWrite(sevenseg_e, LOW);
      digitalWrite(sevenseg_f, LOW);
      digitalWrite(sevenseg_g, LOW);
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

  delay(25);
}


