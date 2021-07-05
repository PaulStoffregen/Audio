// Demonstrate use of AudioAnalyzePrint
//
// Generates a square wave shaped by an envelope, and alternately
// prints the resulting wave and just the envelope. Note that the
// actual audio output is silenced, we don't need it and the "raw"
// envelope might well be Bad for Stuff.
//
// Intended to be triggered by the user entering a decimation 
// level, but as this isn't working in Arduino 1.8.13 Serial
// Plotter, it also sends output every couple of seconds: you can
// flip to Serial Monitor and enter a new value, then back to the 
// Serial Plotter
//
// This example code is in the public domain.

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
//#include <SD.h>
//#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthWaveformDc     dc1;            //xy=97,441
AudioSynthWaveform       waveform1;      //xy=111,401
AudioMixer4              mixer1;         //xy=259,428
AudioEffectEnvelope      envelope1;      //xy=394,426
AudioAmplifier           amp1;           //xy=538,425
AudioAnalyzePrint        print1;         //xy=540,366
AudioOutputI2S           i2s1;           //xy=699,422
AudioConnection          patchCord1(dc1, 0, mixer1, 1);
AudioConnection          patchCord2(waveform1, 0, mixer1, 0);
AudioConnection          patchCord3(mixer1, envelope1);
AudioConnection          patchCord4(envelope1, amp1);
AudioConnection          patchCord5(envelope1, print1);
AudioConnection          patchCord6(amp1, 0, i2s1, 1);
AudioConnection          patchCord7(amp1, 0, i2s1, 0);
AudioControlSGTL5000     sgtl5000_1;     //xy=553,532
// GUItool: end automatically generated code


void setup() {
  Serial.begin(115200);
  while (!Serial) 
    ; // wait for Arduino Serial Monitor

  AudioMemory(5);

  pinMode(LED_BUILTIN,OUTPUT);
    
  // not sure what these do...
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.45);

  // Set up print analyzer
  print1.name("440Hz"); // not really needed
  print1.decimate(44);  // decimate by enough that we get to see the whole area of interest
  
  waveform1.begin(1.0f,440.0f,WAVEFORM_SQUARE); // square makes it easier to see at high decimation values
  dc1.amplitude(1.0f);  // set to maximum
  mixer1.gain(0,0.0f);  // everything switched off
  mixer1.gain(1,0.0f);  // to start with
  amp1.gain(0.0f);      // don't really want the audio output
  envelope1.release(40);// fast release so logs aren't too long
}

enum {Idle,doIt,doEnv1, doEnv2, doRelease,stopIt,} state = doIt,last=doIt;
uint32_t next = 100;
int8_t again = 10;

void loop() {
  long num;
  
  if (millis() > next)
  {
    next = millis() + 100;
    switch (state)
    {
      default:
        again--;
        if (Serial.available() || 0 == again) // user entry or every so often
        {
          again = 20;
          num = Serial.parseInt();
          if (num > 0)
            print1.decimate(num);
          state = last == doIt
                      ?doEnv2
                      :doIt;
          last = state;
          digitalWrite(LED_BUILTIN,HIGH);
        }
        break;

      case doIt:
        AudioNoInterrupts();
        mixer1.gain(0,1.0f);
        envelope1.noteOn();
        print1.delay(4);
        print1.length(250);
        print1.trigger();
        AudioInterrupts();
        state = doRelease;
        //next = millis() + 200;
        break;

      case doRelease:
        envelope1.noteOff();
        state = stopIt;
        break;
        
      case doEnv2:
        Serial.println("raw env");
        mixer1.gain(0,0.0f);
        mixer1.gain(1,1000.0f);
        print1.trigger();
        envelope1.noteOn();
        state = doRelease;
        //next = millis() + 200;
        break;
        
      case stopIt:
        mixer1.gain(0,0.0f);
        mixer1.gain(1,0.0f);
        state = Idle;
        while (Serial.available())
          Serial.read();
        digitalWrite(LED_BUILTIN,LOW);
        //AudioNoInterrupts();
        break;
        
    }
  }

}
