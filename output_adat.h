/* ADAT for Teensy 3.X
 * Copyright (c) 2017, Ernstjan Freriks,
 * Thanks to Frank Bösing & KPC & Paul Stoffregen!
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

#ifndef output_ADAT_h_
#define output_ADAT_h_

#include "Arduino.h"
#include "AudioStream.h"
#include "DMAChannel.h"

class AudioOutputADAT : public AudioStream
{
public:
	AudioOutputADAT(void) : AudioStream(8, inputQueueArray) { begin(); }
	virtual void update(void);
	void begin(void);
	static void mute_PCM(const bool mute);
protected:
	AudioOutputADAT(int dummy): AudioStream(8, inputQueueArray) {}
	static void config_ADAT(void);
	static audio_block_t *block_ch1_1st;
	static audio_block_t *block_ch2_1st;
	static audio_block_t *block_ch3_1st;
	static audio_block_t *block_ch4_1st;
	static audio_block_t *block_ch5_1st;
	static audio_block_t *block_ch6_1st;
	static audio_block_t *block_ch7_1st;
	static audio_block_t *block_ch8_1st;
	static bool update_responsibility;
	static DMAChannel dma;
	static void isr(void);
	static void setI2SFreq(int freq);
private:
	//static uint32_t vucp;
	static audio_block_t *block_ch1_2nd;
	static audio_block_t *block_ch2_2nd;
	static audio_block_t *block_ch3_2nd;
	static audio_block_t *block_ch4_2nd;
	static audio_block_t *block_ch5_2nd;
	static audio_block_t *block_ch6_2nd;
	static audio_block_t *block_ch7_2nd;
	static audio_block_t *block_ch8_2nd;
	static uint16_t ch1_offset;
	static uint16_t ch2_offset;
	static uint16_t ch3_offset;
	static uint16_t ch4_offset;
	static uint16_t ch5_offset;
	static uint16_t ch6_offset;
	static uint16_t ch7_offset;
	static uint16_t ch8_offset;
	audio_block_t *inputQueueArray[8];
};


#endif
