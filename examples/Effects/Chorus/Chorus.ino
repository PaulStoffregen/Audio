/*

This example has been modified for the trellis m4

VERSION 2 - use modified library which has been changed to handle
            one channel instead of two
140529
Proc = 7 (7),  Mem = 4 (4)
  2a
  - default at startup is to have passthru ON and the button
    switches the chorus effect in.
  
previous performance measures were PROC/MEM 9/4

From: http://www.cs.cf.ac.uk/Dave/CM0268/PDF/10_CM0268_Audio_FX.pdf
about Comb filter effects
Effect      Delay range (ms)    Modulation
Resonator      0 - 20            None
Flanger        0 - 15            Sinusoidal (approx 1Hz)
Chorus        25 - 50            None
Echo            >50              None

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
#define CHORUS_DELAY_LENGTH (16*AUDIO_BLOCK_SAMPLES)
// Allocate the delay lines for left and right channels
short l_delayline[CHORUS_DELAY_LENGTH];
short r_delayline[CHORUS_DELAY_LENGTH];

// Default is to just pass the audio through.
// applies the chorus effect
#define PASSTHRU_KEY 1

AudioInputAnalogStereo      audioInput(PIN_MIC, 0);;         
AudioEffectChorus   l_myEffect;
AudioEffectChorus   r_myEffect;
AudioOutputAnalogStereo      audioOutput;

// Create Audio connections between the components
// we only have mono input, send it to both channels
AudioConnection c1(audioInput, 0, l_myEffect, 0);
AudioConnection c2(audioInput, 0, r_myEffect, 0);
// both channels chorus effects go to the audio output
AudioConnection c3(l_myEffect, 0, audioOutput, 0);
AudioConnection c4(r_myEffect, 0, audioOutput, 1);

// number of "voices" in the chorus which INCLUDES the original voice
int n_chorus = 2;

// <<<<<<<<<<<<<<>>>>>>>>>>>>>>>>
void setup() {
  
  Serial.begin(9600);
  while (!Serial) ;
  delay(3000);

  customKeypad.begin();

  // Maximum memory usage was reported as 4
  // Proc = 9 (9),  Mem = 4 (4)
  AudioMemory(4);

  // Initialize the effect - left channel
  // address of delayline
  // total number of samples in the delay line
  // number of voices in the chorus INCLUDING the original voice
  if(!l_myEffect.begin(l_delayline,CHORUS_DELAY_LENGTH,n_chorus)) {
    Serial.println("AudioEffectChorus - left channel begin failed");
    while(1);
  }

  // Initialize the effect - right channel
  // address of delayline
  // total number of samples in the delay line
  // number of voices in the chorus INCLUDING the original voice
  if(!r_myEffect.begin(r_delayline,CHORUS_DELAY_LENGTH,n_chorus)) {
    Serial.println("AudioEffectChorus - left channel begin failed");
    while(1);
  }
  // Initially the effect is off. It is switched on when the
  // PASSTHRU button is pushed.
  l_myEffect.voices(0);
  r_myEffect.voices(0);

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
        // switch on the chorus when the button is pressed
        l_myEffect.voices(n_chorus);
        r_myEffect.voices(n_chorus);
      }
      else if(e.bit.EVENT == KEY_JUST_RELEASED){
        // switch off the chorus when button is released
        l_myEffect.voices(0);
        r_myEffect.voices(0);
      }
    }
  }
  delay(10);
}
