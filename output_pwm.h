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

#ifndef output_pwm_h_
#define output_pwm_h_

#include "Arduino.h"
#include "AudioStream.h"
#include "DMAChannel.h"

#if defined(__IMXRT1062__)
struct _pwm_pin_info_struct {
  uint8_t type;    // 0=no pwm, 1=flexpwm, 2=quad
  uint8_t module;  // 0-3, 0-3
  uint8_t channel; // 0=X, 1=A, 2=B
  uint8_t muxval;  //
};
struct _audio_info_flexpwm {
  IMXRT_FLEXPWM_t *flexpwm;
  _pwm_pin_info_struct info;
  uint8_t pin;
};
#endif

class AudioOutputPWM : public AudioStream
{
public:
	AudioOutputPWM(void) : AudioStream(1, inputQueueArray) { begin(); }
	virtual void update(void);
private:
	static bool update_responsibility;
	audio_block_t *inputQueueArray[1];
	static void isr(void);
	void begin(void);
#if defined(KINETISK)
	static audio_block_t *block_1st;
	static audio_block_t *block_2nd;
	static uint32_t block_offset;
	static uint8_t interrupt_count;
	static DMAChannel dma;
#elif defined(__IMXRT1062__)
public:
	AudioOutputPWM(uint8_t pin1, uint8_t pin2) : AudioStream(1, inputQueueArray) { begin(pin1, pin2); }
private: 
	void begin(uint8_t pin1, uint8_t pin2); //FlexPWM pins only
	static audio_block_t *block;
	static DMAChannel dma[2];
	static _audio_info_flexpwm apins[2];
#endif
};

#endif
