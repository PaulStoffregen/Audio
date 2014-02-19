/*
Change the chorus code to produce a flange effect

PROC/MEM 25/4
140204
  d
  - fixed the problem with user-supplied delay line

140203
  c
  
140203
  b
  - when switching to/from passthru, keep the delay line filled
    BUT to be effective must also fix up begin and add another
    function to allow changing to/from passthru without 
    reinitialing everything.
    
I have mixed up the names "chorus" and flange". The sketches named
chorus have, up to version 'm', actually implemented a flanger.
From version 'o' onwards the sketches named chorus will implement
a real chorus effect and flange will do a flanging effect.

140202
  a
Modify filter_test_f to try out a chorus effect

m + n only changed the effect parameters
    - 
140201
  l - found the problem at last using my_flange_cd_usd_c
YES! Way back when I found I hadn't been using d_depth. Now I
    have just discovered that I haven't been using delay_offset!!!!!
 
140201
  k
  interpolation doesn't remove the ticking
  
140131
  j
>>> The lower the frequency, the less ticking.
  - try interpolation
140201
  - got this restored to the way it was last night
    and then reinstated the changes. I had a couple of
    changes to the right channel that were incorrect or
    weren't carried over from the left channel changes
  
  i
  - don't know why but this version seems to have more "presence"
    than previous versions.
    The presence occurred when "sign = 1" was put in front of the left.
    It essentially makes it a passthrough.
    If both have "sign=1" then it is identical to passthrough
    
    Ticking is still not fixed
    
  h
  - add sign reversal. It seems to make audio much clearer
    BUT it hasn't got rid of the ticking noise
  
  g
  - I wasn't even using delay_depth!!!!
  
140131
  f
  - added code to print the processor and memory usage every 5 seconds
NOW the problem is to try to remove the ticking

  e
FOUND the problem with the right channel. It was also in the left channel
but the placement of the delay line arrays made it more noticeable in the
right channel. I was not calculating idx properly. In particular, the
resuling index could be negative.

I have shortened the delay line to only 2*AUDIO_BLOCK_SAMPLES

  - removed redundancies in the update code. rewrite the block
    instead of getting a new one
Haven't solved the noise in the right channel yet.
Tried duplicating right channel code to left but noise stays on the right

140130
  d
The noise stays in the right channel even if it is calculated first
Switching the L/R inputs doesn't switch the noise to the left channel  
  
  c
>> Now add a sinusoidal modulation to the offset
    There's an awful noise in both channels but it is much louder in
    the right channel. NOPE - it is ONLY in the right channel
    but the audio does sound like it is working (sort of) except that it
    is rather tinny. Maybe it needs to have the interpolation added.

  b
  - this works with clip16_6s.wav.
    The original of this audio file was from http://www.donreiman.com/Chorus/Chorus.htm
    I reworked it with Goldwave to make it a stereo WAV file
    But with Rick Wakewan's Jane Seymour it seems to act more like
    a high-pass filter.
    
  a
  - removed FIR stuff and changed the name to AudioEffectChorus
    it's basically a blank template and compiles.

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

// If this pin is grounded the FIR filter is turned which
// makes just pass through the audio
// Don't use any of the pins listed above
#define PASSTHRU_PIN 1

Bounce b_passthru = Bounce(PASSTHRU_PIN,15);

//const int myInput = AUDIO_INPUT_MIC;
const int myInput = AUDIO_INPUT_LINEIN;

AudioInputI2S       audioInput;         // audio shield: mic or line-in
AudioEffectFlange   myEffect;
AudioOutputI2S      audioOutput;        // audio shield: headphones & line-out

// Create Audio connections between the components
// Both channels of the audio input go to the FIR filter
AudioConnection c1(audioInput, 0, myEffect, 0);
AudioConnection c2(audioInput, 1, myEffect, 1);
// both channels from the FIR filter go to the audio output
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
  // address of delayline
  // total number of samples (left AND right) in the delay line
  // Index (in samples) into the delay line for the added voice
  // Depth of the flange effect
  // frequency of the flange effect
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
if(1) {
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
  // filter index and then switch the filter to passthru
  if(b_passthru.fallingEdge()) {
    myEffect.modify(DELAY_PASSTHRU,0,0);
  }
  
  // If passthru button is released, restore the effect
  if(b_passthru.risingEdge()) {
    myEffect.modify(s_idx,s_depth,s_freq);
  }

}




