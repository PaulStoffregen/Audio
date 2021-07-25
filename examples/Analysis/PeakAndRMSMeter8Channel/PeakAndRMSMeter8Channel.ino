/* Show levels (RMS & Peak) for 8 I2S microphone inputs
 * 
 * Connect 8 INMP411 I2S microphones to Teensy 4.0
 *   Pin 8    SD on mics #1 and #2
 *   Pin 6    SD on mics #3 and #4
 *   Pin 9    SD on mics #5 and #6
 *   Pin 32   SD on mics #7 and #8
 *   Pin 20   WS on all mics
 *   Pin 21   SCK on all mics
 *   
 * Each mic needs GND to Teensy GND, VCC to Teensy 3.3V.
 * Connect L/R to GND on the odd numbered mics
 *   and L/R to 3.3V on the even numbered mics.
 * 
 * Optional - connect a Teensy Audio Shield or other I2S
 * output device, but do not connect it to pin 8, because
 * the INMP411 mics #1 & #2 send their signal to pin 8.
 * 
 * This example code is in the public domain
 */

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// If INMP411 mics are not available, the audio shield mic can
// be used, but it will send a single signal to inputs #1 & #2
// (if connected to pin 8).
//const int myInput = AUDIO_INPUT_LINEIN;
const int myInput = AUDIO_INPUT_MIC;

AudioInputI2SOct     audioInput;   // audio shield: mic or line-in
AudioAmplifier       amp1;
AudioAmplifier       amp2;
AudioAmplifier       amp3;
AudioAmplifier       amp4;
AudioAmplifier       amp5;
AudioAmplifier       amp6;
AudioAmplifier       amp7;
AudioAmplifier       amp8;
AudioAnalyzeRMS      rms1;
AudioAnalyzeRMS      rms2;
AudioAnalyzeRMS      rms3;
AudioAnalyzeRMS      rms4;
AudioAnalyzeRMS      rms5;
AudioAnalyzeRMS      rms6;
AudioAnalyzeRMS      rms7;
AudioAnalyzeRMS      rms8;
AudioAnalyzePeak     peak1;
AudioAnalyzePeak     peak2;
AudioAnalyzePeak     peak3;
AudioAnalyzePeak     peak4;
AudioAnalyzePeak     peak5;
AudioAnalyzePeak     peak6;
AudioAnalyzePeak     peak7;
AudioAnalyzePeak     peak8;
AudioOutputI2S       audioOutput;    // audio shield: headphones & line-out

// Send all microphone signals to amps
AudioConnection r1(audioInput, 0, amp1, 0);
AudioConnection r2(audioInput, 1, amp2, 0);
AudioConnection r3(audioInput, 2, amp3, 0);
AudioConnection r4(audioInput, 3, amp4, 0);
AudioConnection r5(audioInput, 4, amp5, 0);
AudioConnection r6(audioInput, 5, amp6, 0);
AudioConnection r7(audioInput, 6, amp7, 0);
AudioConnection r8(audioInput, 7, amp8, 0);

// Connect the amps to RMS and Peak analyzers
AudioConnection a1(amp1, 0, rms1, 0);
AudioConnection a2(amp2, 0, rms2, 0);
AudioConnection a3(amp3, 0, rms3, 0);
AudioConnection a4(amp4, 0, rms4, 0);
AudioConnection a5(amp5, 0, rms5, 0);
AudioConnection a6(amp6, 0, rms6, 0);
AudioConnection a7(amp7, 0, rms7, 0);
AudioConnection a8(amp8, 0, rms8, 0);
AudioConnection p1(amp1, 0, peak1, 0);
AudioConnection p2(amp2, 0, peak2, 0);
AudioConnection p3(amp3, 0, peak3, 0);
AudioConnection p4(amp4, 0, peak4, 0);
AudioConnection p5(amp5, 0, peak5, 0);
AudioConnection p6(amp6, 0, peak6, 0);
AudioConnection p7(amp7, 0, peak7, 0);
AudioConnection p8(amp8, 0, peak8, 0);

// Also connect 2 of the amps to an I2S output (Pin 8)
// to be able to listen to the sound.
AudioConnection c10(amp1, 0, audioOutput, 0);
AudioConnection c11(amp2, 0, audioOutput, 1);
AudioControlSGTL5000 audioShield;


void setup() {
  AudioMemory(26);
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.micGain(12);
  audioShield.volume(0.5);
  const float microphoneAmplification = 72.0;
  amp1.gain(microphoneAmplification);
  amp2.gain(microphoneAmplification);
  amp3.gain(microphoneAmplification);
  amp4.gain(microphoneAmplification);
  amp5.gain(microphoneAmplification);
  amp6.gain(microphoneAmplification);
  amp7.gain(microphoneAmplification);
  amp8.gain(microphoneAmplification);
  Serial.begin(9600);
}

elapsedMillis fps;

void loop() {
  if (fps > 24) {
    fps = 0;
    const int digits = 8;
    print_bar(1, rms1.read(), peak1.readPeakToPeak()/2, digits);
    print_bar(2, rms2.read(), peak2.readPeakToPeak()/2, digits);
    print_bar(3, rms3.read(), peak3.readPeakToPeak()/2, digits);
    print_bar(4, rms4.read(), peak4.readPeakToPeak()/2, digits);
    print_bar(5, rms5.read(), peak5.readPeakToPeak()/2, digits);
    print_bar(6, rms6.read(), peak6.readPeakToPeak()/2, digits);
    print_bar(7, rms7.read(), peak7.readPeakToPeak()/2, digits);
    print_bar(8, rms8.read(), peak8.readPeakToPeak()/2, digits);
    Serial.println();
  }
}

void print_bar(int in, float rms, float peak, int digits) {  
  Serial.print(in);
  Serial.print(":");
  
  int num_rms = roundf(rms * digits);
  int num_peak = roundf(peak * digits);
  
  for (int i=0; i < digits; i++) {
    if (i < num_rms) {
      Serial.print("*");
    } else if (i < num_peak) {
      Serial.print(">");
    } else {
      Serial.print(" ");
    }
  }
  Serial.print(" ");
}
