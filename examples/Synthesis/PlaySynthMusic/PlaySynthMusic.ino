// Implement a 16 note polyphonic midi player  :-)
//
// Music data is read from memory.  The "Miditones" program is used to
// convert from a MIDI file to this compact format.
//
// This example code is in the public domain.
 
#include <Audio.h>
#include <Wire.h>
#ifndef __SAMD51__
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>
#endif

#include "PlaySynthMusic.h"

unsigned char *sp = score;

#define AMPLITUDE (0.2)

// Create 16 waveforms, one for each MIDI channel
AudioSynthWaveform sine0, sine1, sine2, sine3;
AudioSynthWaveform sine4, sine5, sine6, sine7;
AudioSynthWaveform sine8, sine9, sine10, sine11;
AudioSynthWaveform sine12, sine13, sine14, sine15;
AudioSynthWaveform *waves[16] = {
  &sine0, &sine1, &sine2, &sine3,
  &sine4, &sine5, &sine6, &sine7,
  &sine8, &sine9, &sine10, &sine11,
  &sine12, &sine13, &sine14, &sine15
};

// allocate a wave type to each channel.
// The types used and their order is purely arbitrary.
short wave_type[16] = {
  WAVEFORM_SINE,
  WAVEFORM_SQUARE,
  WAVEFORM_SAWTOOTH,
  WAVEFORM_TRIANGLE,
  WAVEFORM_SINE,
  WAVEFORM_SQUARE,
  WAVEFORM_SAWTOOTH,
  WAVEFORM_TRIANGLE,
  WAVEFORM_SINE,
  WAVEFORM_SQUARE,
  WAVEFORM_SAWTOOTH,
  WAVEFORM_TRIANGLE,
  WAVEFORM_SINE,
  WAVEFORM_SQUARE,
  WAVEFORM_SAWTOOTH,
  WAVEFORM_TRIANGLE
};

// Each waveform will be shaped by an envelope
AudioEffectEnvelope env0, env1, env2, env3;
AudioEffectEnvelope env4, env5, env6, env7;
AudioEffectEnvelope env8, env9, env10, env11;
AudioEffectEnvelope env12, env13, env14, env15;
AudioEffectEnvelope *envs[16] = {
  &env0, &env1, &env2, &env3,
  &env4, &env5, &env6, &env7,
  &env8, &env9, &env10, &env11,
  &env12, &env13, &env14, &env15
};

// Route each waveform through its own envelope effect
AudioConnection patchCord01(sine0, env0);
AudioConnection patchCord02(sine1, env1);
AudioConnection patchCord03(sine2, env2);
AudioConnection patchCord04(sine3, env3);
AudioConnection patchCord05(sine4, env4);
AudioConnection patchCord06(sine5, env5);
AudioConnection patchCord07(sine6, env6);
AudioConnection patchCord08(sine7, env7);
AudioConnection patchCord09(sine8, env8);
AudioConnection patchCord10(sine9, env9);
AudioConnection patchCord11(sine10, env10);
AudioConnection patchCord12(sine11, env11);
AudioConnection patchCord13(sine12, env12);
AudioConnection patchCord14(sine13, env13);
AudioConnection patchCord15(sine14, env14);
AudioConnection patchCord16(sine15, env15);

// Four mixers are needed to handle 16 channels of music
AudioMixer4     mixer1;
AudioMixer4     mixer2;
AudioMixer4     mixer3;
AudioMixer4     mixer4;

// Mix the 16 channels down to 4 audio streams
AudioConnection patchCord17(env0, 0, mixer1, 0);
AudioConnection patchCord18(env1, 0, mixer1, 1);
AudioConnection patchCord19(env2, 0, mixer1, 2);
AudioConnection patchCord20(env3, 0, mixer1, 3);
AudioConnection patchCord21(env4, 0, mixer2, 0);
AudioConnection patchCord22(env5, 0, mixer2, 1);
AudioConnection patchCord23(env6, 0, mixer2, 2);
AudioConnection patchCord24(env7, 0, mixer2, 3);
AudioConnection patchCord25(env8, 0, mixer3, 0);
AudioConnection patchCord26(env9, 0, mixer3, 1);
AudioConnection patchCord27(env10, 0, mixer3, 2);
AudioConnection patchCord28(env11, 0, mixer3, 3);
AudioConnection patchCord29(env12, 0, mixer4, 0);
AudioConnection patchCord30(env13, 0, mixer4, 1);
AudioConnection patchCord31(env14, 0, mixer4, 2);
AudioConnection patchCord32(env15, 0, mixer4, 3);

// Now create 2 mixers for the main output
AudioMixer4     mixerLeft;
AudioMixer4     mixerRight;
#ifndef __SAMD51__
AudioOutputI2S  audioOut;
#else
AudioOutputAnalogStereo  audioOut;
#endif

// Mix all channels to both the outputs
AudioConnection patchCord33(mixer1, 0, mixerLeft, 0);
AudioConnection patchCord34(mixer2, 0, mixerLeft, 1);
AudioConnection patchCord35(mixer3, 0, mixerLeft, 2);
AudioConnection patchCord36(mixer4, 0, mixerLeft, 3);
AudioConnection patchCord37(mixer1, 0, mixerRight, 0);
AudioConnection patchCord38(mixer2, 0, mixerRight, 1);
AudioConnection patchCord39(mixer3, 0, mixerRight, 2);
AudioConnection patchCord40(mixer4, 0, mixerRight, 3);
AudioConnection patchCord41(mixerLeft, 0, audioOut, 0);
AudioConnection patchCord42(mixerRight, 0, audioOut, 1);

#ifndef __SAMD51__
AudioControlSGTL5000 codec;
#endif

// Initial value of the volume control
int volume = 50;

void setup()
{
  Serial.begin(115200);
  //while (!Serial) ; // wait for Arduino Serial Monitor
  delay(200);
  
// http://gcc.gnu.org/onlinedocs/cpp/Standard-Predefined-Macros.html
  Serial.print("Begin ");
  Serial.println(__FILE__);
  
  // Proc = 12 (13),  Mem = 2 (8)
  // Audio connections require memory to work.
  // The memory usage code indicates that 10 is the maximum
  // so give it 12 just to be sure.
  AudioMemory(18);
  
#ifndef __SAMD51__
  codec.enable();
  codec.volume(0.45);
#endif

  // reduce the gain on some channels, so half of the channels
  // are "positioned" to the left, half to the right, but all
  // are heard at least partially on both ears
  mixerLeft.gain(1, 0.36);
  mixerLeft.gain(3, 0.36);
  mixerRight.gain(0, 0.36);
  mixerRight.gain(2, 0.36);

  // set envelope parameters, for pleasing sound :-)
  for (int i=0; i<16; i++) {
    envs[i]->attack(9.2);
    envs[i]->hold(2.1);
    envs[i]->decay(31.4);
    envs[i]->sustain(0.6);
    envs[i]->release(84.5);
    // uncomment these to hear without envelope effects
    //envs[i]->attack(0.0);
    //envs[i]->hold(0.0);
    //envs[i]->decay(0.0);
    //envs[i]->release(0.0);
  }

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
  
// Change this to if(1) for measurement output every 5 seconds
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

  // Volume control
  //  uncomment if you have a volume pot soldered to your audio shield
  /*
  int n = analogRead(15);
  if (n != volume) {
    volume = n;
    codec.volume((float)n / 1023);
  }
  */
  
  // read the next note from the table
  c = *sp++;
  opcode = c & 0xF0;
  chan = c & 0x0F;

  if(c < 0x80) {
    // Delay
    d_time = (c << 8) | *sp++;
    delay(d_time);
    return;
  }
  if(*sp == CMD_STOP) {
    for (chan=0; chan<10; chan++) {
      envs[chan]->noteOff();
      waves[chan]->amplitude(0);
    }
    Serial.println("DONE");
    while(1);
  }

  // It is a command
  
  // Stop the note on 'chan'
  if(opcode == CMD_STOPNOTE) {
    envs[chan]->noteOff();
    return;
  }
  
  // Play the note on 'chan'
  if(opcode == CMD_PLAYNOTE) {
    unsigned char note = *sp++;
    unsigned char velocity = *sp++;
    AudioNoInterrupts();
    waves[chan]->begin(AMPLITUDE * velocity2amplitude[velocity-1],
                       tune_frequencies2_PGM[note],
                       wave_type[chan]);
    envs[chan]->noteOn();
    AudioInterrupts();
    return;
  }

  // replay the tune
  if(opcode == CMD_RESTART) {
    sp = score;
    return;
  }
}
