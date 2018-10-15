/*
c
  - released
b
- Use FIR filters with fast_fft option

Press key 1 on the trellis to engage the filter
Press key 2 to change the filter type
*/

//#include <arm_math.h>
#include <Audio.h>
#include "filters.h"
#include <Adafruit_Keypad.h>

#define NEO_PIN 10
#define NUM_KEYS 32

const byte ROWS = 4; // four rows
const byte COLS = 8; // eight columns
//define the symbols on the buttons of the keypads
byte trellisKeys[ROWS][COLS] = {
  {1,  2,  3,  4,  5,  6,  7,  8},
  {9,  10, 11, 12, 13, 14, 15, 16},
  {17, 18, 19, 20, 21, 22, 23, 24},
  {25, 26, 27, 28, 29, 30, 31, 32}
};
byte rowPins[ROWS] = {14, 15, 16, 17}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {2, 3, 4, 5, 6, 7, 8, 9}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Adafruit_Keypad customKeypad = Adafruit_Keypad( makeKeymap(trellisKeys), rowPins, colPins, ROWS, COLS);

// If this key is pressed FIR filter is turned off
// which just passes the audio sraight through
#define PASSTHRU_KEY 1
// If this key is pressedthe next FIR filter in the list
// is switched in.
#define FILTER_KEY 2

AudioInputAnalogStereo  audioInput(PIN_MIC, 0);
AudioFilterFIR      myFilterL;
AudioFilterFIR      myFilterR;
AudioOutputAnalogStereo  audioOutput; 

// Create Audio connections between the components
// Route audio into the left and right filters
AudioConnection c1(audioInput, 0, myFilterL, 0);
AudioConnection c2(audioInput, 1, myFilterR, 0);
// Route the output of the filters to their respective channels
AudioConnection c3(myFilterL, 0, audioOutput, 0);
AudioConnection c4(myFilterR, 0, audioOutput, 1);


struct fir_filter {
  short *coeffs;
  short num_coeffs;    // num_coeffs must be an even number, 4 or higher
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

  customKeypad.begin();

  // allocate memory for the audio library
  AudioMemory(8);
  
  // Initialize the filter
  myFilterL.begin(fir_list[0].coeffs, fir_list[0].num_coeffs);
  myFilterR.begin(fir_list[0].coeffs, fir_list[0].num_coeffs);
  Serial.println("setup done");
}

// index of current filter when passthrough is selected
int old_idx = -1;

void loop()
{
  customKeypad.tick();
  
  while(customKeypad.available())
  {
    keypadEvent e = customKeypad.read();
    if(e.bit.KEY == PASSTHRU_KEY){
      if(e.bit.EVENT == KEY_JUST_PRESSED){
        // If the passthru button is pushed, save the current
        // filter index and then switch the filter to passthru
        old_idx = fir_idx;
        myFilterL.begin(FIR_PASSTHRU, 0);
        myFilterR.begin(FIR_PASSTHRU, 0);
      }
      else if(e.bit.EVENT == KEY_JUST_RELEASED){
        // If passthru button is released, restore previous filter
        if(old_idx != -1) {
          myFilterL.begin(fir_list[fir_idx].coeffs, fir_list[fir_idx].num_coeffs);
          myFilterR.begin(fir_list[fir_idx].coeffs, fir_list[fir_idx].num_coeffs);
        }
        old_idx = -1;
      }
    }
    else if(e.bit.KEY == FILTER_KEY){
      if(e.bit.EVENT == KEY_JUST_PRESSED){
        fir_idx++;
        if (fir_list[fir_idx].num_coeffs == 0) fir_idx = 0;
        myFilterL.begin(fir_list[fir_idx].coeffs, fir_list[fir_idx].num_coeffs);
        myFilterR.begin(fir_list[fir_idx].coeffs, fir_list[fir_idx].num_coeffs);
      }
    }
  }
  delay(10);
}


