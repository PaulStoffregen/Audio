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

#ifndef output_dacs_h_
#define output_dacs_h_

#include "Arduino.h"
#include "AudioStream.h"

#if defined(ARDUINO_ARCH_SAMD)

#include "Adafruit_ZeroDMA.h"

class AudioOutputAnalogStereo : public AudioStream
{
	public:
	AudioOutputAnalogStereo(void) : AudioStream(2, inputQueueArray) { begin(); }
	virtual void update(void);
	void begin(void);
	void analogReference(int ref);
	private:
	static audio_block_t *block_left_1st;
	static audio_block_t *block_left_2nd;
	static audio_block_t *block_right_1st;
	static audio_block_t *block_right_2nd;
	static audio_block_t block_silent;
	static bool update_responsibility;
	audio_block_t *inputQueueArray[2];
	static Adafruit_ZeroDMA *dma0;
	static DmacDescriptor *desc;
	static void isr(Adafruit_ZeroDMA *dma);
};

#else //teensy

#include "DMAChannel.h"

class AudioOutputAnalogStereo : public AudioStream
{
public:
	AudioOutputAnalogStereo(void) : AudioStream(2, inputQueueArray) { begin(); }
	virtual void update(void);
	void begin(void);
	void analogReference(int ref);
private:
	static audio_block_t *block_left_1st;
	static audio_block_t *block_left_2nd;
	static audio_block_t *block_right_1st;
	static audio_block_t *block_right_2nd;
	static audio_block_t block_silent;
	static bool update_responsibility;
	audio_block_t *inputQueueArray[2];
	static DMAChannel dma;
	static void isr(void);
};

#endif

#endif
