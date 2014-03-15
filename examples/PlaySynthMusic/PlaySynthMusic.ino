// Implement the midi player inside the Audio library.
// This uses the new version of the waveform generator code
// See PlayMidiTones for code which uses the old version

 
#include <Audio.h>
#include <Wire.h>
//#include <WM8731.h>
#include <SD.h>
#include <SPI.h>
#include "arm_math.h"

#include "PlaySynthMusic.h"

unsigned char *sp = score;

#define AMPLITUDE (0.4)


// The midi file has more than 8 channels
// Those above 7 will be mapped to 0, 1 etc.
AudioSynthWaveform sine0;
AudioSynthWaveform sine1;
AudioSynthWaveform sine2;
AudioSynthWaveform sine3;
AudioSynthWaveform sine4;
AudioSynthWaveform sine5;
AudioSynthWaveform sine6;
AudioSynthWaveform sine7;
AudioSynthWaveform *waves[8] = {
  &sine0,
  &sine1,
  &sine2,
  &sine3,
  &sine4,
  &sine5,
  &sine6,
  &sine7,
};

AudioMixer4              mixer1;
AudioMixer4              mixer2;
AudioOutputI2S audioOut;

AudioConnection c0(sine0, 0, mixer1, 0);
AudioConnection c1(sine1, 0, mixer1, 1);
AudioConnection c2(sine2, 0, mixer1, 2);
AudioConnection c3(sine3, 0, mixer1, 3);

AudioConnection c4(sine4, 0, mixer2, 0);
AudioConnection c5(sine5, 0, mixer2, 1);
AudioConnection c6(sine6, 0, mixer2, 2);
AudioConnection c7(sine7, 0, mixer2, 3);
AudioConnection c11(mixer1, 0, audioOut, 0);
AudioConnection c12(mixer2, 0, audioOut, 1);

//AudioControl_WM8731 codec;
AudioControlSGTL5000 codec;

int volume = 50;

// allocate a wave type to each channel.
// The types used and their order is purely arbitrary.
short wave_type[8] = {
  TONE_TYPE_SINE,
  TONE_TYPE_SQUARE,
  TONE_TYPE_SAWTOOTH,
  TONE_TYPE_TRIANGLE,
  TONE_TYPE_SINE,
  TONE_TYPE_SQUARE,
  TONE_TYPE_SAWTOOTH,
  TONE_TYPE_TRIANGLE,
};

void setup()
{
  Serial.begin(115200);
  while (!Serial) ;
  delay(3000);

  // Audio connections require memory to work.
  // The memory usage code indicates that 8 is the maximum
  // so give it 10 just to be sure.
  AudioMemory(10);
  
  codec.enable();
  codec.volume(volume);
  // I want output on the line out too
  // Comment this if you don't it
  codec.unmuteLineout();

  
  // Set the ramp time for each wave object
  for(int i = 0; i < 8;i++) {
    waves[i]->set_ramp_length(88);
  }

  Serial.println("Begin PlayMidiTones");

  Serial.println("setup done");
  
  // Initialize processor and memory measurements
  AudioProcessorUsageMaxReset();
  AudioMemoryUsageMaxReset();
}


unsigned long last_time = millis();
void loop()
{
  unsigned char c,opcode,chan;
  unsigned long d_time;
  
// Change this to if(1) for measurement output
if(0) {
/*
  For PlaySynthMusic this produces:
  Proc = 20 (21),  Mem = 2 (8)
*/
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

  // Volume control
  int n = analogRead(15);
  if (n != volume) {
    volume = n;
    codec.volume((float)n / 10.23);
  }
  
  c = *sp++;
  opcode = c & 0xf0;
  // was 0x0f but I'm only handling 8 channels
  // This will map Ch 8->Ch 0, Ch 9->Ch 1, etc.
  chan = c & 0x07;

  if(c < 0x80) {
    // Delay
    d_time = (c << 8) | *sp++;
    delay(d_time);
    return;
  }
  if(*sp == CMD_STOP) {
    Serial.println("DONE");
    while(1);
  }

  // It is a command
  
  // Stop the note on 'chan'
  if(opcode == CMD_STOPNOTE) {
    waves[chan]->amplitude(0);
    return;
  }
  
  // Play the note on 'chan'
  if(opcode == CMD_PLAYNOTE) {
    waves[chan]->begin(AMPLITUDE,tune_frequencies2_PGM[*sp++],
                        wave_type[chan]);
    return;
  }

  // replay the tune
  if(opcode == CMD_RESTART) {
    sp = score;
    return;
  }
}
