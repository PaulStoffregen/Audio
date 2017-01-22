// Advanced Microcontroller-based Audio Workshop
//
// http://www.pjrc.com/store/audio_tutorial_kit.html
// https://hackaday.io/project/8292-microcontroller-audio-workshop-had-supercon-2015
// 
// Part 2-8: Oscillators

#include <Bounce.h>

Bounce button0 = Bounce(0, 15);
Bounce button1 = Bounce(1, 15);  // 15 = 15 ms debounce time
Bounce button2 = Bounce(2, 15);


///////////////////////////////////
// copy the Design Tool code here
///////////////////////////////////




void setup() {
  Serial.begin(9600);
  AudioMemory(20);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.32);
  
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  mixer1.gain(0, 0.75);
  mixer1.gain(1, 0.0);
  mixer1.gain(2, 0.0);
  mixer1.gain(3, 0.0);
  mixer2.gain(0, 0.15);
  mixer2.gain(1, 0.0);
  mixer2.gain(2, 0.0);
  mixer2.gain(3, 0.0);
  waveform1.begin(WAVEFORM_SAWTOOTH);
  waveform1.amplitude(0.75);
  waveform1.frequency(50);
  waveform1.pulseWidth(0.15);
  sine_fm1.frequency(440);
  sine_fm1.amplitude(0.75);
  sine1.frequency(200);
  sine1.amplitude(0.75);
  pink1.amplitude(0.75);
  envelope1.attack(10);
  envelope1.hold(10);
  envelope1.decay(25);
  envelope1.sustain(0.4);
  envelope1.release(70);
}

int waveform_type = WAVEFORM_SAWTOOTH;
int mixer1_setting = 0;
int mixer2_setting = 0;
elapsedMillis timeout = 0;
bool mixer2_envelope = false;

void loop() {
  button0.update();
  button1.update();
  button2.update();

  // Left changes the type of control waveform
  if (button0.fallingEdge()) {
    Serial.print("Control waveform: ");
    if (waveform_type == WAVEFORM_SAWTOOTH) {
      waveform_type = WAVEFORM_SINE;
      Serial.println("Sine");
    } else if (waveform_type == WAVEFORM_SINE) {
      waveform_type = WAVEFORM_SQUARE;
      Serial.println("Square");
    } else if (waveform_type == WAVEFORM_SQUARE) {
      waveform_type = WAVEFORM_TRIANGLE;
      Serial.println("Triangle");
    } else if (waveform_type == WAVEFORM_TRIANGLE) {
      waveform_type = WAVEFORM_PULSE;
      Serial.println("Pulse");
    } else if (waveform_type == WAVEFORM_PULSE) {
      waveform_type = WAVEFORM_SAWTOOTH;
      Serial.println("Sawtooth");
    }
    waveform1.begin(waveform_type);
  }

  // middle button switch which source we hear from mixer1
  if (button1.fallingEdge()) {
    if (mixer1_setting == 0) {
      mixer1.gain(0, 0.75);
      mixer1.gain(1, 0.0);
      mixer1.gain(2, 0.0);
      mixer1.gain(3, 0.0);
      Serial.println("Mixer1: Control oscillator");
      mixer1_setting = 1;
    } else if (mixer1_setting == 1) {
      mixer1.gain(0, 0.0);
      mixer1.gain(1, 0.75);
      mixer1.gain(2, 0.0);
      mixer1.gain(3, 0.0);
      Serial.println("Mixer1: Frequency Modulated Oscillator");
      mixer1_setting = 2;
    } else if (mixer1_setting == 2) {
      mixer1.gain(0, 0.0);
      mixer1.gain(1, 0.0);
      mixer1.gain(2, 0.75);
      mixer1.gain(3, 0.0);
      Serial.println("Mixer1: Regular Sine Wave Oscillator");
      mixer1_setting = 3;
    } else if (mixer1_setting == 3) {
      mixer1.gain(0, 0.0);
      mixer1.gain(1, 0.0);
      mixer1.gain(2, 0.0);
      mixer1.gain(3, 0.75);
      Serial.println("Mixer1: Pink Noise");
      mixer1_setting = 0;
    }
  }

  // Right button activates the envelope
  if (button2.fallingEdge()) {
    mixer2.gain(0, 0.0);
    mixer2.gain(1, 1.0);
    mixer2_envelope = true;
    timeout = 0;
    envelope1.noteOn();
  }
  if (button2.risingEdge()) {
    envelope1.noteOff();
    timeout = 0;
  }

  // after 4 seconds of inactivity, go back to
  // steady listening intead of the envelope
  if (mixer2_envelope == true && timeout > 4000) {
    mixer2.gain(0, 0.15);
    mixer2.gain(1, 0.0);
    mixer2_envelope = false;
  }

  // use the knobs to adjust parameters
  //float knob1 = (float)analogRead(A1) / 1023.0;
  float knob2 = (float)analogRead(A2) / 1023.0;
  float knob3 = (float)analogRead(A3) / 1023.0;
  waveform1.frequency(360 * knob2 + 0.25);
  sine_fm1.frequency(knob3 * 1500 + 50);
  sine1.frequency(knob3 * 1500 + 50);
}


