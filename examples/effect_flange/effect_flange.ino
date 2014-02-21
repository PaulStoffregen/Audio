/*
Change the chorus code to produce a flange effect
140219
  e

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
#define FLANGE_DELAY_LENGTH (6*AUDIO_BLOCK_SAMPLES)
// Allocate the delay line for left and right channels
// The delayline will hold left and right samples so it
// should be declared to be twice as long as the desired
// number of samples in one channel
#define FLANGE_DELAYLINE (FLANGE_DELAY_LENGTH*2)
// The delay line for left and right channels
short delayline[FLANGE_DELAYLINE];

// If this pin is grounded the effect is turned off,
// which makes it just pass through the audio
// Don't use any of the pins listed above
#define PASSTHRU_PIN 1

Bounce b_passthru = Bounce(PASSTHRU_PIN,15);

//const int myInput = AUDIO_INPUT_MIC;
const int myInput = AUDIO_INPUT_LINEIN;

AudioInputI2S       audioInput;         // audio shield: mic or line-in
AudioEffectFlange   myEffect;
AudioOutputI2S      audioOutput;        // audio shield: headphones & line-out

// Create Audio connections between the components
// Both channels of the audio input go to the flange effect
AudioConnection c1(audioInput, 0, myEffect, 0);
AudioConnection c2(audioInput, 1, myEffect, 1);
// both channels from the flange effect go to the audio output
AudioConnection c3(myEffect, 0, audioOutput, 0);
AudioConnection c4(myEffect, 1, audioOutput, 1);

AudioControlSGTL5000 audioShield;

/* 
int s_idx = FLANGE_DELAY_LENGTH/2;
int s_depth = FLANGE_DELAY_LENGTH/16;
double s_freq = 1;

// <<<<<<<<<<<<<<>>>>>>>>>>>>>>>>
// 12
int s_idx = FLANGE_DELAY_LENGTH/2;
int s_depth = FLANGE_DELAY_LENGTH/8;
double s_freq = .125;
// with .125 the ticking is about 1Hz with music
// but with the noise sample it is a bit slower than that
// <<<<<<<<<<<<<<>>>>>>>>>>>>>>>>
*/
/*
// <<<<<<<<<<<<<<>>>>>>>>>>>>>>>>
// 12
int s_idx = FLANGE_DELAY_LENGTH/2;
int s_depth = FLANGE_DELAY_LENGTH/12;
double s_freq = .125;
// with .125 the ticking is about 1Hz with music
// but with the noise sample it is a bit slower than that
// <<<<<<<<<<<<<<>>>>>>>>>>>>>>>>
*/
/*
//12
int s_idx = 15*FLANGE_DELAY_LENGTH/16;
int s_depth = 15*FLANGE_DELAY_LENGTH/16;
double s_freq = 0;
*/
/*
//12
int s_idx = 2*FLANGE_DELAY_LENGTH/4;
int s_depth = FLANGE_DELAY_LENGTH/8;
double s_freq = .0625;
*/

/*
//12 - good with Eric Clapton Unplugged
int s_idx = 3*FLANGE_DELAY_LENGTH/4;
int s_depth = FLANGE_DELAY_LENGTH/8;
double s_freq = .0625;
*/

/*
// Real flange effect! delay line is 2*
int s_idx = 2*FLANGE_DELAY_LENGTH/4;
int s_depth = FLANGE_DELAY_LENGTH/4;
double s_freq = 2;
*/

/* 2 - 
int s_idx = 2*FLANGE_DELAY_LENGTH/4;
int s_depth = FLANGE_DELAY_LENGTH/8;
double s_freq = 4;
*/
/*
// 4
int s_idx = FLANGE_DELAY_LENGTH/4;
int s_depth = FLANGE_DELAY_LENGTH/4;
double s_freq = .25;
*/
// 4
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
  audioShield.volume(50);
  
  // Warn that the passthru pin is grounded
  if(!digitalRead(PASSTHRU_PIN)) {
    Serial.print("PASSTHRU_PIN (");
    Serial.print(PASSTHRU_PIN);
    Serial.println(") is grounded");
  }

  // Set up the flange effect
  // - address of delayline
  // - total number of samples (left AND right) in the delay line
  // - Index (in samples) into the delay line for the added voice
  // - Depth of the flange effect
  // - frequency of the flange effect
  myEffect.begin(delayline,FLANGE_DELAYLINE,s_idx,s_depth,s_freq);

  // I want output on the line out too
  audioShield.unmuteLineout();
  
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
 
  // If the passthru button is pushed, save the current
  // filter index and then switch the effect to passthru
  if(b_passthru.fallingEdge()) {
    myEffect.modify(DELAY_PASSTHRU,0,0);
  }
  
  // If passthru button is released, restore the effect
  if(b_passthru.risingEdge()) {
    myEffect.modify(s_idx,s_depth,s_freq);
  }

}
