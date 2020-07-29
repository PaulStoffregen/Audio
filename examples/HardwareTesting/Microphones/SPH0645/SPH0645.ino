/* SPH0645 MEMS Microphone Test (Adafruit product #3421)
 *
 * Forum thread with connection details and other info:
 * https://forum.pjrc.com/threads/60599?p=238070&viewfull=1#post238070
 */


#include <Audio.h>

// GUItool: begin automatically generated code
AudioInputI2S            i2s1;           //xy=180,111
AudioFilterStateVariable filter1;        //xy=325,101
AudioAmplifier           amp1;           //xy=470,93
AudioAnalyzeFFT1024      fft1024_1;      //xy=616,102
AudioConnection          patchCord1(i2s1, 0, filter1, 0);
AudioConnection          patchCord2(filter1, 2, amp1, 0);
AudioConnection          patchCord3(amp1, fft1024_1);
// GUItool: end automatically generated code

void setup() {
  AudioMemory(50);
  filter1.frequency(30); // filter out DC & extremely low frequencies
  amp1.gain(8.5);        // amplify sign to useful range
}

void loop() {
  if (fft1024_1.available()) {
    // each time new FFT data is available
    // print 20 bins to the Arduino Serial Monitor
    Serial.print("FFT: ");
    for (int i = 0; i < 20; i++) {
      float n = fft1024_1.read(i);
      if (n >= 0.001) {
        Serial.print(n, 3);
        Serial.print(" ");
      } else {
        Serial.print("  --  "); // don't print "0.000"
      }
    }
    Serial.println();
  }
}
