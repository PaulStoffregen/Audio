/*
  Demo of the audio sweep function.
  The user specifies the amplitude,
  start and end frequencies (which can sweep up or down)
  and the length of time of the sweep.
   
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

*/

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>

AudioSynthToneSweep myEffect;
AudioOutputI2S      audioOutput;        // audio shield: headphones & line-out

// The tone sweep goes to left and right channels
AudioConnection c1(myEffect, 0, audioOutput, 0);
AudioConnection c2(myEffect, 0, audioOutput, 1);

AudioControlSGTL5000 audioShield;


float t_ampx = 0.8;
int t_lox = 10;
int t_hix = 22000;
// Length of time for the sweep in seconds
float t_timex = 10;
// <<<<<<<<<<<<<<>>>>>>>>>>>>>>>>
void setup(void)
{
  
  Serial.begin(9600);
  while (!Serial) ;
  delay(3000);

  AudioMemory(2);

  audioShield.enable();
  audioShield.volume(0.5);

  Serial.println("setup done");

  if(!myEffect.play(t_ampx,t_lox,t_hix,t_timex)) {
    Serial.println("AudioSynthToneSweep - begin failed");
    while(1);
  }
  // wait for the sweep to end
  while(myEffect.isPlaying());

  // and now reverse the sweep
  if(!myEffect.play(t_ampx,t_hix,t_lox,t_timex)) {
    Serial.println("AudioSynthToneSweep - begin failed");
    while(1);
  }
  // wait for the sweep to end
  while(myEffect.isPlaying());
  Serial.println("Done");
}

void loop(void)
{
}



