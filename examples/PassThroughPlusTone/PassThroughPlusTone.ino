#include <Audio.h>
#include <Bounce.h>
#include <Wire.h>
#include <SD.h>

const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;

// Create the Audio components.  These should be created in the
// order data flows, inputs/sources -> processing -> outputs
//
//AudioInputAnalog analogPinInput(16); // analog A2 (pin 16)
AudioInputI2S       audioInput;         // audio shield: mic or line-in
AudioSynthWaveform  toneLow(AudioWaveformSine);
AudioSynthWaveform  toneHigh(AudioWaveformTriangle);
AudioMixer4         mixerLeft;
AudioMixer4         mixerRight;
AudioOutputI2S      audioOutput;        // audio shield: headphones & line-out
AudioOutputPWM      pwmOutput;          // audio output with PWM on pins 3 & 4

// Create Audio connections between the components
//
AudioConnection c1(audioInput, 0, mixerLeft, 0);
AudioConnection c2(audioInput, 1, mixerRight, 0);
AudioConnection c3(toneHigh, 0, mixerLeft, 1);
AudioConnection c4(toneHigh, 0, mixerRight, 1);
AudioConnection c5(toneLow, 0, mixerLeft, 2);
AudioConnection c6(toneLow, 0, mixerRight, 2);
AudioConnection c7(mixerLeft, 0, audioOutput, 0);
AudioConnection c8(mixerRight, 0, audioOutput, 1);
AudioConnection c9(mixerLeft, 0, pwmOutput, 0);

// Create an object to control the audio shield.
// 
AudioControlSGTL5000 audioShield;


// Bounce objects to read two pushbuttons (pins 0 and 1)
//
Bounce button0 = Bounce(0, 12);
Bounce button1 = Bounce(1, 12);  // 12 ms debounce time


void setup() {
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(12);

  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(60);
}

elapsedMillis volmsec=0;

void loop() {
  // Check each button
  button0.update();
  button1.update();

  // fallingEdge = high (not pressed - voltage from pullup resistor)
  //               to low (pressed - button connects pin to ground)
  if (button0.fallingEdge()) {
    toneLow.frequency(256);
    toneLow.amplitude(0.4);
  }
  if (button1.fallingEdge()) {
    toneHigh.frequency(980);
    toneHigh.amplitude(0.25);
  } 

  // risingEdge = low (pressed - button connects pin to ground)
  //              to high (not pressed - voltage from pullup resistor)
  if (button0.risingEdge()) {
    toneLow.amplitude(0);
  }
  if (button1.risingEdge()) {
    toneHigh.amplitude(0);
  }

  // every 50 ms, adjust the volume
  if (volmsec > 50) {
    float vol = analogRead(15);
    vol = vol / 10.24;
    audioShield.volume(vol);
    volmsec = 0;
  }
}


