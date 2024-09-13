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

#ifndef output_i2s_h_
#define output_i2s_h_

#include <Arduino.h>     // github.com/PaulStoffregen/cores/blob/master/teensy4/Arduino.h
#include <AudioStream.h> // github.com/PaulStoffregen/cores/blob/master/teensy4/AudioStream.h
#include <DMAChannel.h>  // github.com/PaulStoffregen/cores/blob/master/teensy4/DMAChannel.h


#if !defined(KINETISL)

class AudioOutputI2S : public AudioStream
{
public:
	AudioOutputI2S(void) : AudioStream(2, inputQueueArray) { begin(); }
	virtual void update(void);
	void begin(void);
	friend class AudioInputI2S;
	friend class AudioInputPDM;
#if defined(__IMXRT1062__)
	friend class AudioOutputI2SQuad;
	friend class AudioInputI2SQuad;
	friend class AudioOutputI2SHex;
	friend class AudioInputI2SHex;
	friend class AudioOutputI2SOct;
	friend class AudioInputI2SOct;
#endif
protected:
	AudioOutputI2S(int dummy): AudioStream(2, inputQueueArray) {} // to be used only inside AudioOutputI2Sslave !!
	static void config_i2s(bool only_bclk = false);
	static audio_block_t *block_left_1st;
	static audio_block_t *block_right_1st;
	static bool update_responsibility;
	static DMAChannel dma;
	static void isr(void);
private:
	static audio_block_t *block_left_2nd;
	static audio_block_t *block_right_2nd;
	static uint16_t block_left_offset;
	static uint16_t block_right_offset;
	audio_block_t *inputQueueArray[2];
};


class AudioOutputI2Sslave : public AudioOutputI2S
{
public:
	AudioOutputI2Sslave(void) : AudioOutputI2S(0) { begin(); } ;
	void begin(void);
	friend class AudioInputI2Sslave;
	friend void dma_ch0_isr(void);
protected:
	static void config_i2s(void);
};

#elif defined(KINETISL)

/**************************************************************************************
*       Teensy LC
***************************************************************************************/

class AudioOutputI2S : public AudioStream
{
public:
	AudioOutputI2S(void) : AudioStream(2, inputQueueArray) { begin(); }
	virtual void update(void);
	void begin(void);
	friend class AudioInputI2S;
protected:
	AudioOutputI2S(int dummy): AudioStream(2, inputQueueArray) {} // to be used only inside AudioOutputI2Sslave !!
	static void config_i2s(void);
	static audio_block_t *block_left;
	static audio_block_t *block_right;
	static bool update_responsibility;
	static DMAChannel dma1;
	static DMAChannel dma2;
	static void isr1(void);
	static void isr2(void);
private:
	audio_block_t *inputQueueArray[2];
};


class AudioOutputI2Sslave : public AudioOutputI2S
{
public:
	AudioOutputI2Sslave(void) : AudioOutputI2S(0) { begin(); } ;
	void begin(void);
	friend class AudioInputI2Sslave;
	friend void dma_ch0_isr(void);
	friend void dma_ch1_isr(void);
protected:
	static void config_i2s(void);
};

#endif

#endif
