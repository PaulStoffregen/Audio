// Advanced Microcontroller-based Audio Workshop
//
// http://www.pjrc.com/store/audio_tutorial_kit.html
// https://hackaday.io/project/8292-microcontroller-audio-workshop-had-supercon-2015
//
// This reference file has all the design tool output.  If you are
// learning, please follow the PDF manual and draw these in the design
// tool.  This reference material is mainly interested for software
// testing and ongoing maintenance as Arduino and the audio library
// are updated with new features.

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

/*
// Part 2-1: First Design Tool Use
// GUItool: begin automatically generated code
AudioPlaySdWav           playSdWav1;     //xy=88,55
AudioOutputI2S           i2s1;           //xy=280,79
AudioConnection          patchCord1(playSdWav1, 0, i2s1, 0);
AudioConnection          patchCord2(playSdWav1, 1, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=209,140
// GUItool: end automatically generated code
*/

/*
// Part 2-2: Mixers & Playing Multiple Sounds
// GUItool: begin automatically generated code
AudioPlaySdWav           playSdWav1;     //xy=88,55
AudioPlaySdWav           playSdWav2;     //xy=109,123
AudioMixer4              mixer1;         //xy=284,44
AudioMixer4              mixer2;         //xy=286,123
AudioOutputI2S           i2s1;           //xy=456,104
AudioConnection          patchCord1(playSdWav1, 0, mixer1, 0);
AudioConnection          patchCord2(playSdWav1, 1, mixer2, 0);
AudioConnection          patchCord3(playSdWav2, 0, mixer1, 1);
AudioConnection          patchCord4(playSdWav2, 1, mixer2, 1);
AudioConnection          patchCord5(mixer1, 0, i2s1, 0);
AudioConnection          patchCord6(mixer2, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=382,209
// GUItool: end automatically generated code
*/

/*
// Part 2-3: Playing Samples (Short Sound Clips)
// GUItool: begin automatically generated code
AudioPlayMemory          playMem1;       //xy=92,48
AudioPlayMemory          playMem2;       //xy=96,104
AudioPlayMemory          playMem3;       //xy=99,160
AudioPlayMemory          playMem4;       //xy=100,222
AudioMixer4              mixer1;         //xy=287,125
AudioOutputI2S           i2s1;           //xy=454,99
AudioConnection          patchCord1(playMem1, 0, mixer1, 0);
AudioConnection          patchCord2(playMem2, 0, mixer1, 1);
AudioConnection          patchCord3(playMem3, 0, mixer1, 2);
AudioConnection          patchCord4(playMem4, 0, mixer1, 3);
AudioConnection          patchCord5(mixer1, 0, i2s1, 0);
AudioConnection          patchCord6(mixer1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=397,205
// GUItool: end automatically generated code
*/

/*
// Part 2-4: Using the Microphone
// GUItool: begin automatically generated code
AudioInputI2S            i2s2;           //xy=95,64
AudioOutputI2S           i2s1;           //xy=286,88
AudioConnection          patchCord1(i2s2, 0, i2s1, 0);
AudioConnection          patchCord2(i2s2, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=226,155
// GUItool: end automatically generated code
*/

/*
// Part 2-5: Simple Delay

// GUItool: begin automatically generated code
AudioInputI2S            i2s2;           //xy=95,64
AudioEffectDelay         delay1;         //xy=230,145
AudioMixer4              mixer1;         //xy=367,97
AudioMixer4              mixer2;         //xy=402,188
AudioMixer4              mixer3;         //xy=526,59
AudioOutputI2S           i2s1;           //xy=654,112
AudioConnection          patchCord1(i2s2, 0, delay1, 0);
AudioConnection          patchCord2(i2s2, 0, mixer3, 0);
AudioConnection          patchCord3(delay1, 0, mixer1, 0);
AudioConnection          patchCord4(delay1, 1, mixer1, 1);
AudioConnection          patchCord5(delay1, 2, mixer1, 2);
AudioConnection          patchCord6(delay1, 3, mixer1, 3);
AudioConnection          patchCord7(delay1, 4, mixer2, 0);
AudioConnection          patchCord8(delay1, 5, mixer2, 1);
AudioConnection          patchCord9(delay1, 6, mixer2, 2);
AudioConnection          patchCord10(delay1, 7, mixer2, 3);
AudioConnection          patchCord11(mixer1, 0, mixer3, 1);
AudioConnection          patchCord12(mixer2, 0, mixer3, 2);
AudioConnection          patchCord13(mixer3, 0, i2s1, 0);
AudioConnection          patchCord14(mixer3, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=583,208
// GUItool: end automatically generated code
*/

/*
// Part 2-6: Feedback (Echo) Delay

// GUItool: begin automatically generated code
AudioInputI2S            i2s2;           //xy=95,73
AudioMixer4              mixer1;         //xy=278,62
AudioEffectDelay         delay1;         //xy=361,227
AudioOutputI2S           i2s1;           //xy=536,221
AudioConnection          patchCord1(i2s2, 0, mixer1, 0);
AudioConnection          patchCord2(mixer1, delay1);
AudioConnection          patchCord3(delay1, 0, mixer1, 3);
AudioConnection          patchCord4(delay1, 0, i2s1, 0);
AudioConnection          patchCord5(delay1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=161,241
// GUItool: end automatically generated code
*/

/*
// Part 2-7: Filters

// GUItool: begin automatically generated code
AudioPlaySdWav           playSdWav1;     //xy=84,67
AudioFilterStateVariable filter1;        //xy=259,83
AudioFilterStateVariable filter2;        //xy=270,168
AudioMixer4              mixer1;         //xy=424,80
AudioMixer4              mixer2;         //xy=433,168
AudioOutputI2S           i2s1;           //xy=580,117
AudioConnection          patchCord1(playSdWav1, 0, filter1, 0);
AudioConnection          patchCord2(playSdWav1, 1, filter2, 0);
AudioConnection          patchCord3(filter1, 0, mixer1, 0);
AudioConnection          patchCord4(filter1, 1, mixer1, 1);
AudioConnection          patchCord5(filter1, 2, mixer1, 2);
AudioConnection          patchCord6(filter2, 0, mixer2, 0);
AudioConnection          patchCord7(filter2, 1, mixer2, 1);
AudioConnection          patchCord8(filter2, 2, mixer2, 2);
AudioConnection          patchCord9(mixer1, 0, i2s1, 0);
AudioConnection          patchCord10(mixer2, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=594,179
// GUItool: end automatically generated code
*/

/*
// Part 2-8: Oscillators and Envelope

// GUItool: begin automatically generated code
AudioSynthWaveform       waveform1;      //xy=77,30
AudioSynthWaveformSine   sine1;          //xy=87,159
AudioSynthNoisePink      pink1;          //xy=110,221
AudioSynthWaveformSineModulated sine_fm1;       //xy=145,102
AudioMixer4              mixer1;         //xy=313,111
AudioEffectEnvelope      envelope1;      //xy=454,165
AudioMixer4              mixer2;         //xy=589,87
AudioOutputI2S           i2s1;           //xy=725,149
AudioConnection          patchCord1(waveform1, sine_fm1);
AudioConnection          patchCord2(waveform1, 0, mixer1, 0);
AudioConnection          patchCord3(sine1, 0, mixer1, 2);
AudioConnection          patchCord4(pink1, 0, mixer1, 3);
AudioConnection          patchCord5(sine_fm1, 0, mixer1, 1);
AudioConnection          patchCord6(mixer1, envelope1);
AudioConnection          patchCord7(mixer1, 0, mixer2, 0);
AudioConnection          patchCord8(envelope1, 0, mixer2, 1);
AudioConnection          patchCord9(mixer2, 0, i2s1, 0);
AudioConnection          patchCord10(mixer2, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=317,208
// GUItool: end automatically generated code
*/

/*
// Part 3-1: Peak Detection

// GUItool: begin automatically generated code
AudioPlaySdWav           playSdWav1;     //xy=136,65
AudioAnalyzePeak         peak2;          //xy=348,219
AudioAnalyzePeak         peak1;          //xy=358,171
AudioOutputI2S           i2s1;           //xy=380,92
AudioConnection          patchCord1(playSdWav1, 0, i2s1, 0);
AudioConnection          patchCord2(playSdWav1, 0, peak1, 0);
AudioConnection          patchCord3(playSdWav1, 1, i2s1, 1);
AudioConnection          patchCord4(playSdWav1, 1, peak2, 0);
AudioControlSGTL5000     sgtl5000_1;     //xy=155,192
// GUItool: end automatically generated code
*/

/*
// Part 3-2: Fourier Transform

// GUItool: begin automatically generated code
AudioPlaySdWav           playSdWav1;     //xy=90,44
AudioPlayMemory          playMem1;       //xy=94,113
AudioSynthWaveform       waveform1;      //xy=104,170
AudioMixer4              mixer1;         //xy=290,91
AudioAnalyzeFFT1024      fft1024_1;      //xy=461,132
AudioOutputI2S           i2s1;           //xy=465,58
AudioConnection          patchCord1(playSdWav1, 0, mixer1, 0);
AudioConnection          patchCord2(playSdWav1, 1, mixer1, 1);
AudioConnection          patchCord3(playMem1, 0, mixer1, 2);
AudioConnection          patchCord4(waveform1, 0, mixer1, 3);
AudioConnection          patchCord5(mixer1, 0, i2s1, 0);
AudioConnection          patchCord6(mixer1, 0, i2s1, 1);
AudioConnection          patchCord7(mixer1, fft1024_1);
AudioControlSGTL5000     sgtl5000_1;     //xy=352,195
// GUItool: end automatically generated code
*/

/*
// Part 3-3: Add a TFT Display

// GUItool: begin automatically generated code
AudioPlaySdWav           playSdWav1;     //xy=136,65
AudioAnalyzePeak         peak2;          //xy=348,219
AudioAnalyzePeak         peak1;          //xy=358,171
AudioOutputI2S           i2s1;           //xy=380,92
AudioConnection          patchCord1(playSdWav1, 0, i2s1, 0);
AudioConnection          patchCord2(playSdWav1, 0, peak1, 0);
AudioConnection          patchCord3(playSdWav1, 1, i2s1, 1);
AudioConnection          patchCord4(playSdWav1, 1, peak2, 0);
AudioControlSGTL5000     sgtl5000_1;     //xy=155,192
// GUItool: end automatically generated code
*/

void loop() {
}

void setup() {
}
