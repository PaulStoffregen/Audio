// Waveform Modulation Example - Create waveforms with 
// modulated frequency
//
// This example is meant to be used with 3 buttons (pin 0,
// 1, 2) and 2 knobs (pins 16/A2, 17/A3), which are present
// on the audio tutorial kit.
//   https://www.pjrc.com/store/audio_tutorial_kit.html
//
// Use an oscilloscope to view the 2 waveforms.
//
// Button0 changes the waveform shape
//
// Knob A2 changes the amount of frequency modulation
//
// Knob A3 varies the shape (only for Pulse & Variable Triangle)
//
// This example code is in the public domain.

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>

AudioSynthWaveformSine   sine1;          //xy=131,97
AudioSynthWaveformSine   sine2;          //xy=152,170
AudioSynthWaveformModulated waveformMod1;   //xy=354,69
AudioOutputAnalogStereo  dacs1;          //xy=490,209
AudioOutputI2S           i2s1;           //xy=532,140
AudioConnection          patchCord1(sine1, 0, i2s1, 1);
AudioConnection          patchCord2(sine1, 0, dacs1, 1);
AudioConnection          patchCord3(sine1, 0, waveformMod1, 0);
AudioConnection          patchCord4(sine2, 0, waveformMod1, 1);
AudioConnection          patchCord5(waveformMod1, 0, i2s1, 0);
AudioConnection          patchCord6(waveformMod1, 0, dacs1, 0);
AudioControlSGTL5000     sgtl5000_1;     //xy=286,240

Bounce button0 = Bounce(0, 15);
Bounce button1 = Bounce(1, 15);
Bounce button2 = Bounce(2, 15);

int current_waveform=0;

extern const int16_t myWaveform[256];  // defined in myWaveform.ino

void setup() {
  Serial.begin(9600);
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);

  delay(300);
  Serial.println("Waveform Modulation Test");
  
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(12);

  // Comment these out if not using the audio adaptor board.
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.8); // caution: very loud - use oscilloscope only!

  // Confirgure both to use "myWaveform" for WAVEFORM_ARBITRARY
  waveformMod1.arbitraryWaveform(myWaveform, 172.0);

  // Configure for middle C note without modulation
  waveformMod1.frequency(261.63);
  waveformMod1.amplitude(1.0);
  sine1.frequency(20.3); // Sine waves are low frequency oscillators (LFO)
  sine2.frequency(1.2);

  current_waveform = WAVEFORM_TRIANGLE_VARIABLE;
  waveformMod1.begin(current_waveform);

  // uncomment to try modulating phase instead of frequency
  //waveformMod1.phaseModulation(720.0);
}

void loop() {
  // Read the buttons and knobs, scale knobs to 0-1.0
  button0.update();
  button1.update();
  button2.update();
  float knob_A2 = (float)analogRead(A2) / 1023.0;
  float knob_A3 = (float)analogRead(A3) / 1023.0;

  // use Knobsto adjust the amount of modulation
  sine1.amplitude(knob_A2);
  sine2.amplitude(knob_A3);

  // Button 0 or 2 changes the waveform type
  if (button0.fallingEdge() || button2.fallingEdge()) {
    switch (current_waveform) {
      case WAVEFORM_SINE:
        current_waveform = WAVEFORM_SAWTOOTH;
        Serial.println("Sawtooth");
        break;
      case WAVEFORM_SAWTOOTH:
        current_waveform = WAVEFORM_SAWTOOTH_REVERSE;
        Serial.println("Reverse Sawtooth");
        break;
      case WAVEFORM_SAWTOOTH_REVERSE:
        current_waveform = WAVEFORM_SQUARE;
        Serial.println("Square");
        break;
      case WAVEFORM_SQUARE:
        current_waveform = WAVEFORM_TRIANGLE;
        Serial.println("Triangle");
        break;
      case WAVEFORM_TRIANGLE:
        current_waveform = WAVEFORM_TRIANGLE_VARIABLE;
        Serial.println("Variable Triangle");
        break;
      case WAVEFORM_TRIANGLE_VARIABLE:
        current_waveform = WAVEFORM_ARBITRARY;
        Serial.println("Arbitary Waveform");
        break;
      case WAVEFORM_ARBITRARY:
        current_waveform = WAVEFORM_PULSE;
        Serial.println("Pulse");
        break;
      case WAVEFORM_PULSE:
        current_waveform = WAVEFORM_SAMPLE_HOLD;
        Serial.println("Sample & Hold");
        break;
      case WAVEFORM_SAMPLE_HOLD:
        current_waveform = WAVEFORM_SINE;
        Serial.println("Sine");
        break;
    }
    waveformMod1.begin(current_waveform);
  }
  
}

