/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
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

#ifndef input_adcs_h_
#define input_adcs_h_

#include "Arduino.h"
#include "AudioStream.h"

#if defined(ARDUINO_ARCH_SAMD)

#include "Adafruit_ZeroDMA.h"

class AudioInputAnalogStereo : public AudioStream
{
	public:
	AudioInputAnalogStereo() : AudioStream(0, NULL) {
		init(A2, A3);
	}
	AudioInputAnalogStereo(uint8_t pin0, uint8_t pin1) : AudioStream(0, NULL) {
		init(pin0, pin1);
	}
	virtual void update(void);
	private:
	static audio_block_t *block_left;
	static audio_block_t *block_right;
	static uint16_t offset_left;
	static uint16_t offset_right;
	static int32_t hpf_y1[2];
	static int32_t hpf_x1[2];

	static bool update_responsibility;
	static Adafruit_ZeroDMA *dma0;
	static Adafruit_ZeroDMA *dma1;
	static DmacDescriptor *desc;
	static void isr0(Adafruit_ZeroDMA *dma);
	static void isr1(Adafruit_ZeroDMA *dma);
	static void init(uint8_t pin0, uint8_t pin1);
};

#else

#include "DMAChannel.h"

class AudioInputAnalogStereo : public AudioStream
{
public:
        AudioInputAnalogStereo() : AudioStream(0, NULL) {
        init(A2, A3);
    }
        AudioInputAnalogStereo(uint8_t pin0, uint8_t pin1) : AudioStream(0, NULL) {
        init(pin0, pin1);
    }
        virtual void update(void);
private:
        static audio_block_t *block_left;
        static audio_block_t *block_right;
        static uint16_t offset_left;
        static uint16_t offset_right;
        static int32_t hpf_y1[2];
        static int32_t hpf_x1[2];

        static bool update_responsibility;
        static DMAChannel dma0;
        static DMAChannel dma1;
        static void isr0(void);
        static void isr1(void);
        static void init(uint8_t pin0, uint8_t pin1);
};

#endif

#endif
