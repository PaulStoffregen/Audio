// Demonstrate a resonant state-variable filter whose filter frequency
// is controlled by an LFO and envelope. A second encelope is used
// to control the dynamics of each note. This example is a simple,
// monophonic synth.
//
// Accepts MIDI in over USB. Set USB type to MIDI in the Tools menu.
//
// This example code is in the public domain.
 

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

// GUItool: begin automatically generated code
AudioSynthWaveformSine   sine1;          //xy=86,359
AudioSynthWaveform       waveform2;      //xy=89,302
AudioSynthWaveformSine   sine2;          //xy=90,419
AudioEffectMultiply      multiply1;      //xy=229,389
AudioEffectEnvelope      envelope1;      //xy=379,388
AudioFilterStateVariable filter1;        //xy=383,309
AudioEffectEnvelope      envelope2;      //xy=547,295
AudioOutputI2S           i2s1;           //xy=727,300
AudioConnection          patchCord1(sine1, 0, multiply1, 0);
AudioConnection          patchCord2(waveform2, 0, filter1, 0);
AudioConnection          patchCord3(sine2, 0, multiply1, 1);
AudioConnection          patchCord4(multiply1, envelope1);
AudioConnection          patchCord5(envelope1, 0, filter1, 1);
AudioConnection          patchCord6(filter1, 0, envelope2, 0);
AudioConnection          patchCord7(envelope2, 0, i2s1, 0);
AudioConnection          patchCord8(envelope2, 0, i2s1, 1);
AudioControlSGTL5000     audioShield;     //xy=720,223
// GUItool: end automatically generated code

byte currentNote = 255;  // MIDI note number of currently playing note, 0 - 127 or 255 for no note

void setup(void)
{
  //set up a basic subtractive synth patch
  AudioMemory(8);
  audioShield.enable();
  audioShield.volume(0.45);  // headphone volume (line-out muted)
  // modulation
  sine1.frequency(5);        // 5Hz LFO
  sine1.amplitude(0.7);     // controls modulation depth
  sine2.frequency(23);        // 23Hz second LFO
  sine1.amplitude(0.9);     // controls modulation depth
  envelope1.attack(140);  // fairly slow attack and decay
  envelope1.hold(180);
  envelope1.decay(140);
  envelope1.release(40);
  // saw oscillator
  waveform2.begin(0.2, 220, WAVEFORM_SAWTOOTH);  // 220Hz saw wave oscillator
                                                                                  // quiet level of 0.4 as resonant filter adds gain
  // resonant filter, low-pass mode (output 0 of filter is LP)
  filter1.resonance(18);
  filter1.octaveControl(1.5);  // modulation signal shifts resonant frequency by +/- 1.5 octaves
  filter1.frequency(200);    // start below the resonant peak
  //envelope for note -on and -off dynamics
  envelope2.attack(10);
  envelope2.decay(20);
  envelope2.release(80);
  
  // now respond to MIDI over USB. In this example, only note-on and note-off used and velocity ignored
  usbMIDI.setHandleNoteOff(OnNoteOff);
  usbMIDI.setHandleNoteOn(OnNoteOn);
}
  
 void loop() {
  usbMIDI.read(); // USB MIDI receive
}

void OnNoteOn(byte channel, byte note, byte velocity) {
  // accept input on any channel, and ignore velocity
  
  // check for a currently held note and brutally kill it
  // this sounds bad so only play one note at once, retro style
  if (currentNote <= 127) {
    envelope2.release(2);
    envelope2.noteOff();
    delay(3);  // briefest period for killed note to stop sounding
    envelope2.release(80);
  }
  currentNote = note;
   // MIDI note 69 == A4 == 440Hz
  float freq = ((float)note - 69) / 12.0;  // semitones above or below A4
  freq = 440.0 * powf(2.0, freq);
  sine1.phase(0);          // reset LFOs phases
  sine2.phase(0);    
  envelope1.noteOn();  // start modulation
  waveform2.frequency(freq);
  waveform2.phase(0);
  filter1.frequency(1.2 * freq);  // resonance above played note
  envelope2.noteOn();
}
  
void OnNoteOff(byte channel, byte note, byte velocity) {
  // accept any channel, ignore off-velocity as most things do
  // we only care about the currently playing note
  if (note == currentNote) {
    envelope2.noteOff();
    envelope1.noteOff();
    currentNote = 255;
  }
}

  
  
  
  
  
  
  
  

