/*
VERSION 2 - use modified library which has been changed to handle
            one channel instead of two
            
Proc = 21 (22),  Mem = 4 (6)
140529
  2a
  - default at startup is to have passthru ON and the button
    switches the flange effect in.
  

From: http://www.cs.cf.ac.uk/Dave/CM0268/PDF/10_CM0268_Audio_FX.pdf
about Comb filter effects
Effect      Delay range (ms)    Modulation
Resonator      0 - 20            None
Flanger        0 - 15            Sinusoidal (approx 1Hz)
Chorus        25 - 50            None
Echo            >50              None

The CPU usage is an integer from 0 to 100, and the memory is from 0 to however
many blocks you provided with AudioMemory().

*/

#include <Audio.h>
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

// Number of samples in each delay line
#define FLANGE_DELAY_LENGTH (6*AUDIO_BLOCK_SAMPLES)
// Allocate the delay lines for left and right channels
short l_delayline[FLANGE_DELAY_LENGTH];
short r_delayline[FLANGE_DELAY_LENGTH];

// Default is to just pass the audio through. pressing this button
// applies the flange effect
// Don't use any of the pins listed above
#define PASSTHRU_KEY 1

AudioInputAnalogStereo  audioInput(PIN_MIC, 0);
AudioEffectFlange   l_myEffect;
AudioEffectFlange   r_myEffect;
AudioOutputAnalogStereo  audioOutput; 

// Create Audio connections between the components
// send mono channel to the flange effect
AudioConnection c1(audioInput, 0, l_myEffect, 0);
AudioConnection c2(audioInput, 0, r_myEffect, 0);
// both channels from the flange effect go to the audio output
AudioConnection c3(l_myEffect, 0, audioOutput, 0);
AudioConnection c4(r_myEffect, 0, audioOutput, 1);

int s_idx = FLANGE_DELAY_LENGTH/4;
int s_depth = FLANGE_DELAY_LENGTH/4;
double s_freq = .5;
void setup() {
  
  Serial.begin(9600);
  delay(3000);

  customKeypad.begin();

  // It doesn't work properly with any less than 8
  // but that was an earlier version. Processor and
  // memory usage are now (ver j)
  // Proc = 24 (24),  Mem = 4 (4)
  AudioMemory(8);

  // Set up the flange effect:
  // address of delayline
  // total number of samples in the delay line
  // Index (in samples) into the delay line for the added voice
  // Depth of the flange effect
  // frequency of the flange effect
  l_myEffect.begin(l_delayline,FLANGE_DELAY_LENGTH,s_idx,s_depth,s_freq);
  r_myEffect.begin(r_delayline,FLANGE_DELAY_LENGTH,s_idx,s_depth,s_freq);
  // Initially the effect is off. It is switched on when the
  // PASSTHRU button is pushed.
  l_myEffect.voices(FLANGE_DELAY_PASSTHRU,0,0);
  r_myEffect.voices(FLANGE_DELAY_PASSTHRU,0,0);
  
  Serial.println("setup done");
}

void loop()
{
  customKeypad.tick();
  
  while(customKeypad.available())
  {
    keypadEvent e = customKeypad.read();
    if(e.bit.KEY == PASSTHRU_KEY){
      if(e.bit.EVENT == KEY_JUST_PRESSED){
        // If the passthru button is pushed
        // turn the flange effect on
        // filter index and then switch the effect to passthru
        l_myEffect.voices(s_idx,s_depth,s_freq);
        r_myEffect.voices(s_idx,s_depth,s_freq);
      }
      else if(e.bit.EVENT == KEY_JUST_RELEASED){
        // If passthru button is released restore passthru
        l_myEffect.voices(FLANGE_DELAY_PASSTHRU,0,0);
        r_myEffect.voices(FLANGE_DELAY_PASSTHRU,0,0);
      }
    }
  }
  delay(10);
}
