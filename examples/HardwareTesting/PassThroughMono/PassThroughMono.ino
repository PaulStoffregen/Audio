/*
 * A simple hardware test which receives audio on the A2 analog pin
 * and sends it to the PWM (pin 3) output and DAC (A14 pin) output.
 *
 * This example code is in the public domain.
 */

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioInputAnalog         adc1;           //xy=161,80
AudioOutputAnalog        dac1;           //xy=329,47
AudioOutputPWM           pwm1;           //xy=331,125
AudioConnection          patchCord1(adc1, dac1);
AudioConnection          patchCord2(adc1, pwm1);
// GUItool: end automatically generated code

void setup() {
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(12);
}

void loop() {
  // Do nothing here.  The Audio flows automatically

  // When AudioInputAnalog is running, analogRead() must NOT be used.
}


