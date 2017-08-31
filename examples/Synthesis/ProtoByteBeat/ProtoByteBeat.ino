/* Audio Library for Teensy 3.X
 * Copyright (c) 2017, Oren Leiman
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**********************************************************************************
 * This example program was written to run on the SparkFun Proto Pedal, which
 * is the only piece of simple, programmable audio hardware I had lying
 * around at the time it was written. Bytebeat synthesis really benefits from
 * having some knobs to twist as different combinations of parameters can
 * generate drastically different patterns and timbres within a single
 * equation.
 *
 * You can find the basic kit and its specification here as of 8/30/2017:
 * https://www.sparkfun.com/products/13124
 *
 * For more information about installing the Teensy associated peripherals:
 * https://learn.sparkfun.com/tutorials/proto-pedal-example-programmable-digital-pedal
***********************************************************************************/

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioInputI2S            i2s2;           //xy=314,485
AudioSynthByteBeat       byte_beat1;
AudioMixer4              mixer1;         //xy=642,332
AudioOutputI2S           i2s1;           //xy=915,329
AudioConnection          patchCord2(byte_beat1, 0, mixer1, 0);
AudioConnection          patchCord3(mixer1, 0, i2s1, 0);
AudioControlSGTL5000     sgtl5000_1;     //xy=915,421
// GUItool: end automatically generated code

#define BUTTON 14
#define NPOTS 5

struct {
    uint16_t vals[NPOTS];
    const int pins[NPOTS] = {A1, A7, A6, A3, A2};
} Pots;

enum {
    BYTEPITCH = 4,
    PITCH = 3,
    P0 = 2,
    P1 = 1,
    P2 = 0
};

void setup() {
    pinMode(14, INPUT_PULLUP);
    digitalWrite(14, HIGH);

    Serial.begin(115200);
    Serial.println("Setup...");

    AudioMemory(140);

    AudioNoInterrupts();

    sgtl5000_1.enable();
    sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
    sgtl5000_1.volume(0.8);

    mixer1.gain(0, 1.0);

    AudioInterrupts();

    Serial.println("...Complete!");
}

void readPots() {
    for (int i = 0; i < NPOTS; i++) {
	Pots.vals[i] = analogRead(Pots.pins[i]);
    }
}

void loop() {
    readPots();
    if (digitalRead(BUTTON)) {
	byte_beat1.equation(map(Pots.vals[BYTEPITCH], 0, 1023, 0, 15));
    } else {
	byte_beat1.bytepitch(map(Pots.vals[BYTEPITCH], 0, 1023, 1, 10));
	byte_beat1.pitch(map(Pots.vals[PITCH], 0, 1023, 1, 24));
	byte_beat1.p0(map(Pots.vals[P0], 0, 1023, 1, 255));
	byte_beat1.p1(map(Pots.vals[P1], 0, 1023, 1, 255));
	byte_beat1.p2(map(Pots.vals[P2], 0, 1023, 1, 255));
    }
}
