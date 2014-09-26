/*
c
  - released
b
- Use FIR filters with fast_fft option

The audio board uses the following pins.
 6 - MEMCS
 7 - MOSI
 9 - BCLK
10 - SDCS
11 - MCLK
12 - MISO
13 - RX
14 - SCLK
15 - VOL
18 - SDA
19 - SCL
22 - TX
23 - LRCLK
*/

//#include <arm_math.h>
#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <Bounce.h>
#include "filters.h"

// If this pin is grounded the FIR filter is turned off
// which just passes the audio sraight through
// Don't use any of the pins listed above
#define PASSTHRU_PIN 1
// If this pin goes low the next FIR filter in the list
// is switched in.
#define FILTER_PIN 0

// debounce the passthru and filter switching pins
Bounce b_passthru = Bounce(PASSTHRU_PIN,15);
Bounce b_filter   = Bounce(FILTER_PIN,15);

//const int myInput = AUDIO_INPUT_MIC;
const int myInput = AUDIO_INPUT_LINEIN;


AudioInputI2S       audioInput;         // audio shield: mic or line-in
AudioFilterFIR      myFilterL;
AudioFilterFIR      myFilterR;
AudioOutputI2S      audioOutput;        // audio shield: headphones & line-out

// Create Audio connections between the components
// Route audio into the left and right filters
AudioConnection c1(audioInput, 0, myFilterL, 0);
AudioConnection c2(audioInput, 1, myFilterR, 0);
// Route the output of the filters to their respective channels
AudioConnection c3(myFilterL, 0, audioOutput, 0);
AudioConnection c4(myFilterR, 0, audioOutput, 1);
AudioControlSGTL5000 audioShield;


struct fir_filter {
  short *coeffs;
  short num_coeffs;
};

// index of current filter. Start with the low pass.
int fir_idx = 0;
struct fir_filter fir_list[] = {
  {low_pass , 100},    // low pass with cutoff at 1kHz and -60dB at 2kHz
  {band_pass, 100},    // bandpass 1200Hz - 1700Hz
  {NULL,      0}
};


void setup() {
  Serial.begin(9600);
  delay(300);

  pinMode(PASSTHRU_PIN, INPUT_PULLUP);
  pinMode(FILTER_PIN, INPUT_PULLUP);

  // allocate memory for the audio library
  AudioMemory(8);

  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(0.5);
  
  // Warn that the passthru pin is grounded
  if(!digitalRead(PASSTHRU_PIN)) {
    Serial.print("PASSTHRU_PIN (");
    Serial.print(PASSTHRU_PIN);
    Serial.println(") is grounded");
  }

  // Warn that the filter pin is grounded
  if(!digitalRead(FILTER_PIN)) {
    Serial.print("FILTER_PIN (");
    Serial.print(FILTER_PIN);
    Serial.println(") is grounded");
  }  
  // Initialize the filter
  myFilterL.begin(fir_list[0].coeffs, fir_list[0].num_coeffs);
  myFilterR.begin(fir_list[0].coeffs, fir_list[0].num_coeffs);
  Serial.println("setup done");
}

// index of current filter when passthrough is selected
int old_idx = -1;

// audio volume
int volume = 0;
unsigned long last_time = millis();

void loop()
{
  // Volume control
  int n = analogRead(15);
  if (n != volume) {
    volume = n;
    //uncomment this line if your audio shield has the volume pot
    //audioShield.volume((float)n / 1023);
  }
  
  // update the two buttons
  b_passthru.update();
  b_filter.update();
  
  // If the passthru button is pushed, save the current
  // filter index and then switch the filter to passthru
  if (b_passthru.fallingEdge()) {
    old_idx = fir_idx;
    myFilterL.begin(FIR_PASSTHRU, 0);
    myFilterR.begin(FIR_PASSTHRU, 0);
  }
  
  // If passthru button is released, restore previous filter
  if (b_passthru.risingEdge()) {
    if(old_idx != -1) {
      myFilterL.begin(fir_list[fir_idx].coeffs, fir_list[fir_idx].num_coeffs);
      myFilterR.begin(fir_list[fir_idx].coeffs, fir_list[fir_idx].num_coeffs);
    }
    old_idx = -1;
  }
  
  // switch to next filter in the list
  if (b_filter.fallingEdge()) {
    fir_idx++;
    if (fir_list[fir_idx].num_coeffs == 0) fir_idx = 0;
    myFilterL.begin(fir_list[fir_idx].coeffs, fir_list[fir_idx].num_coeffs);
    myFilterR.begin(fir_list[fir_idx].coeffs, fir_list[fir_idx].num_coeffs);
  }

  // print information about resource usage
  // Proc = 18 (18),  Mem = 4 (5)
  if (millis() - last_time >= 2500) {
    Serial.print("Proc = ");
    Serial.print(AudioProcessorUsage());
    Serial.print(" (");    
    Serial.print(AudioProcessorUsageMax());
    Serial.print("),  Mem = ");
    Serial.print(AudioMemoryUsage());
    Serial.print(" (");    
    Serial.print(AudioMemoryUsageMax());
    Serial.println(")");
    last_time = millis();
  }


}



