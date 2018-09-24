#include <synth_simple_drum.h>

#include <Audio.h>
#include <Wire.h>
#ifndef __SAMD51__
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#endif

// GUItool: begin automatically generated code
AudioSynthSimpleDrum     drum2;          //xy=399,244
AudioSynthSimpleDrum     drum3;          //xy=424,310
AudioSynthSimpleDrum     drum1;          //xy=431,197
AudioSynthSimpleDrum     drum4;          //xy=464,374
AudioMixer4              mixer1;         //xy=737,265
#ifndef __SAMD51__
AudioOutputI2S           audioOut;       
#else
AudioOutputAnalogStereo  audioOut;
#endif 
AudioConnection          patchCord1(drum2, 0, mixer1, 1);
AudioConnection          patchCord2(drum3, 0, mixer1, 2);
AudioConnection          patchCord3(drum1, 0, mixer1, 0);
AudioConnection          patchCord4(drum4, 0, mixer1, 3);
AudioConnection          patchCord5(mixer1, 0, audioOut, 0);
AudioConnection          patchCord6(mixer1, 0, audioOut, 1);
#ifndef __SAMD51__
AudioControlSGTL5000     sgtl5000_1;     //xy=930,518
#endif
// GUItool: end automatically generated code

static uint32_t next;

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);

  // audio library init
  AudioMemory(15);

  next = millis() + 1000;

  AudioNoInterrupts();

  drum1.frequency(60);
  drum1.length(1500);
  drum1.secondMix(0.0);
  drum1.pitchMod(0.55);
  
  drum2.frequency(60);
  drum2.length(300);
  drum2.secondMix(0.0);
  drum2.pitchMod(1.0);
  
  drum3.frequency(550);
  drum3.length(400);
  drum3.secondMix(1.0);
  drum3.pitchMod(0.5);

  drum4.frequency(1200);
  drum4.length(150);
  drum4.secondMix(0.0);
  drum4.pitchMod(0.0);
  
#ifndef __SAMD51__
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);
#endif

  AudioInterrupts();

}

void loop() {
  // put your main code here, to run repeatedly:

  static uint32_t num = 0;

  if(millis() == next)
  {
    next = millis() + 1000;

    switch(num % 4)
    {
      case 0:
        drum1.noteOn();
        break;
      case 1:
        drum2.noteOn();
        break;
      case 2:
        drum3.noteOn();
        break;
      case 3:
        drum4.noteOn();
        break;
    }
    num++;

    Serial.print("Diagnostics: ");
#ifndef __SAMD51__
    Serial.print(AudioProcessorUsageMax());
    Serial.print(" ");
#endif
    Serial.println(AudioMemoryUsageMax());
    AudioProcessorUsageMaxReset();
  }
}
