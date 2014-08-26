#include <Audio.h>
#include <Wire.h>
//#include <WM8731.h>
#include <SD.h>
#include <SPI.h>

#include "PlayMidiTonesADSR.h"

unsigned char *sp = score;

//AudioInputI2S adc;
//AudioInputAnalog ana(16);
AudioSynthWaveform sine0, sine1, sine2, sine3, sine4,
		   sine5, sine6, sine7;;
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

AudioEffectEnvelope env0, env1, env2, env3, env4, env5, env6, env7, env8;

AudioEffectEnvelope *envs[8] = {
  &env0,
  &env1,
  &env2,
  &env3,
  &env4,
  &env5,
  &env6,
  &env7,
};

AudioMixer4              mixer1;
AudioMixer4              mixer2;
AudioOutputI2S audioOut;

AudioConnection c0(sine0, 0, env0, 0);
AudioConnection c1(sine1, 0, env1, 0);
AudioConnection c2(sine2, 0, env2, 0);
AudioConnection c3(sine3, 0, env3, 0);

AudioConnection c4(sine4, 0, env4, 0);
AudioConnection c5(sine5, 0, env5, 0 );
AudioConnection c6(sine6, 0, env6, 0);
AudioConnection c7(sine7, 0, env7, 0);

AudioConnection c8(env0, 0, mixer1, 0);
AudioConnection c9(env1, 0, mixer1, 1);
AudioConnection c10(env2, 0, mixer1, 2);
AudioConnection c11(env3, 0, mixer1, 3);

AudioConnection c12(env4, 0, mixer2, 0);
AudioConnection c13(env5, 0, mixer2, 1);
AudioConnection c14(env6, 0, mixer2, 2);
AudioConnection c15(env7, 0, mixer2, 3);

AudioConnection c16(mixer1, 0, audioOut, 0);
AudioConnection c17(mixer2, 0, audioOut, 1);

//AudioControl_WM8731 codec;
AudioControlSGTL5000 codec;

int volume = 50;

void setup()
{
  Serial.begin(115200);
  while (!Serial) ;
  delay(3000);

  sine0.begin(0.2, 440.0, TONE_TYPE_SQUARE);
  sine1.begin(0.2, 440.0, TONE_TYPE_SQUARE);
  sine2.begin(0.2, 440.0, TONE_TYPE_SQUARE);
  sine3.begin(0.2, 440.0, TONE_TYPE_SQUARE);
  sine4.begin(0.2, 440.0, TONE_TYPE_SQUARE);
  sine5.begin(0.2, 440.0, TONE_TYPE_SQUARE);
  sine6.begin(0.2, 440.0, TONE_TYPE_SQUARE);
  sine7.begin(0.2, 440.0, TONE_TYPE_SQUARE);

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(8);
  
  codec.enable();
  codec.volume(0.5);
  // I want output on the line out too
  codec.unmuteLineout();

// Comment out this code to hear what it sounds like
// when the tones aren't ramped. (or change 88 to 0)
  // Set the ramp time for each wave
  for(int i = 0; i < 8;i++) {
    waves[i]->set_ramp_length(88);
  }

  
  delay(200);
  Serial.println("Begin PlayMidiTones");
  delay(50);
  Serial.println("setup done");
AudioProcessorUsageMaxReset();
AudioMemoryUsageMaxReset();
}

unsigned long last_time = millis();
void loop()
{
  unsigned char c,opcode,chan;
  unsigned long d_time;
  
if(0) {
// For PlayMidiTones this produces:
// Proc = 6 (12),  Mem = 2 (8)
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
  /* Commented out for testing
  int n = analogRead(15);
  if (n != volume) {
    volume = n;
    codec.volume((float)n / 1023);
  }
  */
  
  c = *sp++;
  opcode = c & 0xf0;
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
  if(opcode == CMD_STOPNOTE) {
    //waves[chan]->amplitude(0);
    envs[chan]->noteOff();
    return;
  }

  if(opcode == CMD_PLAYNOTE) {
    waves[chan]->frequency(tune_frequencies2_PGM[*sp++]);
    envs[chan]->noteOn(); 
    //waves[chan]->amplitude(AMPLITUDE);
    return;
  }

  if(opcode == CMD_RESTART) {
    sp = score;
    return;
  }
}
