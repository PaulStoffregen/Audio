// MemoryAndCpuUsage
//
// This example demonstrates how to monitor CPU and memory
// usage by the audio library.  You can see the total memory
// used at any moment, and the maximum (worst case) used.
//
// The total CPU usage, and CPU usage for each object can
// be monitored.  Reset functions clear the maximums.
//
// Use the Arduino Serial Monitor to view the usage info
// and control ('F', 'S', and 'R' keys) this program.
//
// This example code is in the public domain.


#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthWaveformSine   sine1;          //xy=125,221
AudioSynthNoisePink      pink1;          //xy=133,121
AudioEffectEnvelope      envelope1;      //xy=298,133
AudioEffectEnvelope      envelope2;      //xy=302,197
AudioAnalyzeFFT256       fft256_1;       //xy=304,272
AudioMixer4              mixer1;         //xy=486,163
AudioOutputI2S           i2s1;           //xy=640,161
AudioConnection          patchCord1(sine1, envelope2);
AudioConnection          patchCord2(sine1, fft256_1);
AudioConnection          patchCord3(pink1, envelope1);
AudioConnection          patchCord4(envelope1, 0, mixer1, 0);
AudioConnection          patchCord5(envelope2, 0, mixer1, 1);
AudioConnection          patchCord6(mixer1, 0, i2s1, 0);
AudioConnection          patchCord7(mixer1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=517,297
// GUItool: end automatically generated code


void setup() {
  // give the audio library some memory.  We'll be able
  // to see how much it actually uses, which can be used
  // to reduce this to the minimum necessary.
  AudioMemory(20);

  // enable the audio shield
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.6);

  // create a simple percussive sound using pink noise
  // and an envelope to shape it.
  pink1.amplitude(0.5);
  envelope1.attack(1.5);
  envelope1.hold(5);
  envelope1.decay(20);
  envelope1.sustain(0);

  // create a simple bass note using a sine wave
  sine1.frequency(120);
  sine1.amplitude(0.6);
  envelope2.attack(6.5);
  envelope2.hold(25);
  envelope2.decay(70);
  envelope2.sustain(0);

  // add both the note together, so we can hear them
  mixer1.gain(0, 0.5);
  mixer1.gain(1, 0.5);
}


int count = 0;
int speed = 60;


void loop() {
  // a simple sequencer, count goes from 0 to 15
  count = count + 1;
  if (count >= 16) count = 0;

  // play percussive sounds every 4th time
  if (count == 0) envelope1.noteOn();
  if (count == 4) envelope1.noteOn();
  if (count == 8) envelope1.noteOn();
  if (count == 12) envelope1.noteOn();

  // play the bass tone every 8th time
  if (count == 4) {
	sine1.amplitude(0.6);
	sine1.frequency(100);
	envelope2.noteOn();
  }
  if (count == 12) {
	sine1.amplitude(0.3);
	sine1.frequency(120);
	envelope2.noteOn();
  }

  // turn off the sine wave, which saves
  // CPU power (especially since the sine goes
  // to a CPU-hungry FFT analysis)
  if (count == 6) {
	sine1.amplitude(0);
  }

  // check for incoming characters from the serial monitor
  if (Serial.available()) {
    char c = Serial.read();
    if ((c == 'r' || c == 'R')) {
      pink1.processorUsageMaxReset();
      fft256_1.processorUsageMaxReset();
      AudioProcessorUsageMaxReset();
      AudioMemoryUsageMaxReset();
      Serial.println("Reset all max numbers");
    }
    if ((c == 'f' || c == 'F') && speed > 16) {
      speed = speed - 2;
    }
    if ((c == 's' || c == 'S') && speed < 250) {
      speed = speed + 2;
    }
  }

  // print a summary of the current & maximum usage
  Serial.print("CPU: ");
  Serial.print("pink=");
  Serial.print(pink1.processorUsage());
  Serial.print(",");
  Serial.print(pink1.processorUsageMax());
  Serial.print("  ");
  Serial.print("fft=");
  Serial.print(fft256_1.processorUsage());
  Serial.print(",");
  Serial.print(fft256_1.processorUsageMax());
  Serial.print("  ");
  Serial.print("all=");
  Serial.print(AudioProcessorUsage());
  Serial.print(",");
  Serial.print(AudioProcessorUsageMax());
  Serial.print("    ");
  Serial.print("Memory: ");
  Serial.print(AudioMemoryUsage());
  Serial.print(",");
  Serial.print(AudioMemoryUsageMax());
  Serial.print("    ");
  Serial.print("Send: (R)eset, (S)lower, (F)aster");
  Serial.println();

  // very simple timing   :-)
  delay(speed);

}

