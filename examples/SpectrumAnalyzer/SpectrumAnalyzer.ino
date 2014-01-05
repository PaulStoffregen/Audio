#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <LiquidCrystal.h>

//const int myInput = AUDIO_INPUT_LINEIN;
const int myInput = AUDIO_INPUT_MIC;

// Create the Audio components.  These should be created in the
// order data flows, inputs/sources -> processing -> outputs
//
AudioInputI2S       audioInput;         // audio shield: mic or line-in
AudioAnalyzeFFT256  myFFT(11);
AudioOutputI2S      audioOutput;        // audio shield: headphones & line-out

// Create Audio connections between the components
//
AudioConnection c1(audioInput, 0, audioOutput, 0);
AudioConnection c2(audioInput, 0, myFFT, 0);
AudioConnection c3(audioInput, 1, audioOutput, 1);

// Create an object to control the audio shield.
// 
AudioControlSGTL5000 audioShield;

// Use the LiquidCrystal library to display the spectrum
//
LiquidCrystal lcd(0, 1, 2, 3, 4, 5);
byte bar1[8] = {0,0,0,0,0,0,0,255};
byte bar2[8] = {0,0,0,0,0,0,255,255};
byte bar3[8] = {0,0,0,0,0,255,255,255};
byte bar4[8] = {0,0,0,0,255,255,255,255};
byte bar5[8] = {0,0,0,255,255,255,255,255};
byte bar6[8] = {0,0,255,255,255,255,255,255};
byte bar7[8] = {0,255,255,255,255,255,255,255};
byte bar8[8] = {255,255,255,255,255,255,255,255};

void setup() {
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(12);

  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(60);
  
  lcd.begin(16, 2);
  lcd.print("Audio Spectrum");
  lcd.createChar(0, bar1);
  lcd.createChar(1, bar2);
  lcd.createChar(2, bar3);
  lcd.createChar(3, bar4);
  lcd.createChar(4, bar5);
  lcd.createChar(5, bar6);
  lcd.createChar(6, bar7);
  lcd.createChar(7, bar8);

  // pin 21 will select rapid vs animated display
  pinMode(21, INPUT_PULLUP);
}

int count=0;

const int nsum[16] = {1, 1, 2, 2, 3, 4, 5, 6, 6, 8, 12, 14, 16, 20, 28, 24};

int maximum[16];

void loop() {
  if (myFFT.available()) {
    // convert the 128 FFT frequency bins
    // to only 16 sums, for a 16 character LCD
    int sum[16];
    int i;
    for (i=0; i<16; i++) {
      sum[i] = 0;
    }
    int n=0;
    int count=0;
    for (i=0; i<128; i++) {
      sum[n] = sum[n] + myFFT.output[i];
      count = count + 1;
      if (count >= nsum[n]) {
        Serial.print(count);
        Serial.print(" ");
        n = n + 1;
        if (n >= 16) break;
        count = 0;
      }
    }

    // The range is set by the audio shield's
    // knob, which connects to analog pin A1.
    int scale;
    scale = 2 + (1023 - analogRead(A1)) / 7;
    Serial.print(" - ");
    Serial.print(scale);
    Serial.print("  ");
    lcd.setCursor(0, 1);
 
    for (int i=0; i<16; i++) {
      // Reduce the range to 0-8
      int val = sum[i] / scale;
      if (val > 8) val = 8;

      // Compute an animated maximum, where increases
      // show instantly, but if the number is less that
      // the last displayed value, decrease it by 1 for
      // a slow decay (looks pretty)
      if (val >= maximum[i]) {
        maximum[i] = val;
      } else {
        if (maximum[i] > 0) maximum[i] = maximum[i] - 1;
      }

      // a switch on pin 22 select whether we show the
      // slower animation or the direct/fast data
      if (digitalRead(21) == HIGH) {
        val = maximum[i];
      }

      // print each custom digit
      if (val == 0) {
        lcd.write(' ');
      } else {
        lcd.write(val - 1);
      }
      
      Serial.print(sum[i]);
      Serial.print("=");
      Serial.print(val);
      Serial.print(",");
    }
    Serial.println();
    count = 0;
  }
}


