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

FMI:
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


AudioProcessorUsage()
AudioProcessorUsageMax()
AudioProcessorUsageMaxReset()
AudioMemoryUsage()
AudioMemoryUsageMax()
AudioMemoryUsageMaxReset()

The CPU usage is an integer from 0 to 100, and the memory is from 0 to however
many blocks you provided with AudioMemory().


*/

#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <Bounce.h>

// Number of samples in each delay line
#define FLANGE_DELAY_LENGTH (6*AUDIO_BLOCK_SAMPLES)
// Allocate the delay lines for left and right channels
short l_delayline[FLANGE_DELAY_LENGTH];
short r_delayline[FLANGE_DELAY_LENGTH];

// Default is to just pass the audio through. Grounding this pin
// applies the flange effect
// Don't use any of the pins listed above
#define PASSTHRU_PIN 1

Bounce b_passthru = Bounce(PASSTHRU_PIN,15);

//const int myInput = AUDIO_INPUT_MIC;
const int myInput = AUDIO_INPUT_LINEIN;

AudioInputI2S       audioInput;         // audio shield: mic or line-in
AudioEffectFlange   l_myEffect;
AudioEffectFlange   r_myEffect;
AudioOutputI2S      audioOutput;        // audio shield: headphones & line-out

// Create Audio connections between the components
// Both channels of the audio input go to the flange effect
AudioConnection c1(audioInput, 0, l_myEffect, 0);
AudioConnection c2(audioInput, 1, r_myEffect, 0);
// both channels from the flange effect go to the audio output
AudioConnection c3(l_myEffect, 0, audioOutput, 0);
AudioConnection c4(r_myEffect, 0, audioOutput, 1);

AudioControlSGTL5000 audioShield;


int s_idx = FLANGE_DELAY_LENGTH/4;
int s_depth = FLANGE_DELAY_LENGTH/4;
double s_freq = .5;
void setup() {
  
  Serial.begin(9600);
  while (!Serial) ;
  delay(3000);

  pinMode(PASSTHRU_PIN,INPUT_PULLUP);

  // It doesn't work properly with any less than 8
  // but that was an earlier version. Processor and
  // memory usage are now (ver j)
  // Proc = 24 (24),  Mem = 4 (4)
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
  AudioProcessorUsageMaxReset();
  AudioMemoryUsageMaxReset();
}


// audio volume
int volume = 0;

unsigned long last_time = millis();
void loop()
{
  // Volume control
  int n = analogRead(15);
  if (n != volume) {
    volume = n;
    audioShield.volume((float)n / 10.23);
  }
if(0) {
  if(millis() - last_time >= 5000) {
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
  // update the button
  b_passthru.update();
 
  // If the passthru button is pushed
  // turn the flange effect on
  // filter index and then switch the effect to passthru
  if(b_passthru.fallingEdge()) {
    l_myEffect.voices(s_idx,s_depth,s_freq);
    r_myEffect.voices(s_idx,s_depth,s_freq);
  }
  
  // If passthru button is released restore passthru
  if(b_passthru.risingEdge()) {
    l_myEffect.voices(FLANGE_DELAY_PASSTHRU,0,0);
    r_myEffect.voices(FLANGE_DELAY_PASSTHRU,0,0);
  }

}
