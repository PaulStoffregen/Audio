/*
PROC/MEM 9/4

140219
  p

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

#include <arm_math.h>
#include <Audio.h>
#include <Wire.h>
//#include <WM8731.h>
#include <SD.h>
#include <SPI.h>
#include <Bounce.h>



// Number of samples in ONE channel
#define CHORUS_DELAY_LENGTH (16*AUDIO_BLOCK_SAMPLES)
// Allocate the delay line for left and right channels
// The delayline will hold left and right samples so it
// should be declared to be twice as long as the desired
// number of samples in one channel
#define CHORUS_DELAYLINE (CHORUS_DELAY_LENGTH*2)
// The delay line for left and right channels
short delayline[CHORUS_DELAYLINE];

// If this pin is grounded the chorus is turned off
// which makes it just pass through the audio
// Don't use any of the pins listed above
#define PASSTHRU_PIN 1

Bounce b_passthru = Bounce(PASSTHRU_PIN,15);

//const int myInput = AUDIO_INPUT_MIC;
const int myInput = AUDIO_INPUT_LINEIN;

AudioInputI2S       audioInput;         // audio shield: mic or line-in
AudioEffectChorus   myEffect;
AudioOutputI2S      audioOutput;        // audio shield: headphones & line-out

// Create Audio connections between the components
// Both channels of the audio input go to the chorus effect
AudioConnection c1(audioInput, 0, myEffect, 0);
AudioConnection c2(audioInput, 1, myEffect, 1);
// both channels from the chorus effect go to the audio output
AudioConnection c3(myEffect, 0, audioOutput, 0);
AudioConnection c4(myEffect, 1, audioOutput, 1);

AudioControlSGTL5000 audioShield;


// number of "voices" in the chorus which INCLUDES the original voice
int n_chorus = 2;

// <<<<<<<<<<<<<<>>>>>>>>>>>>>>>>
void setup() {
  
  Serial.begin(9600);
  while (!Serial) ;
  delay(3000);

  pinMode(PASSTHRU_PIN,INPUT_PULLUP);

  // Maximum memory usage was reported as 4
  // Proc = 9 (9),  Mem = 4 (4)
  AudioMemory(4);

  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(0.5);
  
  // Warn that the passthru pin is grounded
  if(!digitalRead(PASSTHRU_PIN)) {
    Serial.print("PASSTHRU_PIN (");
    Serial.print(PASSTHRU_PIN);
    Serial.println(") is grounded");
  }

  // Initialize the effect
  // - address of delayline
  // - total number of samples (left AND right) in the delay line
  // - number of voices in the chorus INCLUDING the original voice
  if(!myEffect.begin(delayline,CHORUS_DELAYLINE,n_chorus)) {
    Serial.println("AudioEffectChorus - begin failed");
    while(1);
  }
  
  // I want output on the line out too
  audioShield.unmuteLineout();
//  audioShield.muteHeadphone();
  
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
    audioShield.volume((float)n / 1023);
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
 
  // If the passthru button is pushed, switch the effect to passthru
  if(b_passthru.fallingEdge()) {
    myEffect.modify(0);
  }
  
  // If passthru button is released, restore the previous chorus
  if(b_passthru.risingEdge()) {
    myEffect.modify(n_chorus);
  }

}
