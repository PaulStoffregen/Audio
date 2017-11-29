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

#ifndef audio_spi_interrupt_h_
#define audio_spi_interrupt_h_

#include "Arduino.h"
#include "AudioStream.h"
#include "SPI.h"

static inline void AudioStartUsingSPI(void) __attribute__((always_inline, unused));
static inline void AudioStopUsingSPI(void) __attribute__((always_inline, unused));

#ifdef SPI_HAS_NOTUSINGINTERRUPT

extern unsigned short AudioUsingSPICount;

static inline void AudioStartUsingSPI(void) {
	SPI.usingInterrupt(IRQ_SOFTWARE);
	AudioUsingSPICount++;
}

static inline void AudioStopUsingSPI(void) {
	if (AudioUsingSPICount == 0 || --AudioUsingSPICount == 0)
		SPI.notUsingInterrupt(IRQ_SOFTWARE);
}

#else

static inline void AudioStartUsingSPI(void) {
#if defined(ARDUINO_ARCH_SAMD)
	//TODO: this
#else
	SPI.usingInterrupt(IRQ_SOFTWARE);
#endif
}

static inline void AudioStopUsingSPI(void) {
}

#endif

#endif
