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

#ifndef OUTPUT_I2S_CLOCKS_H_
#define OUTPUT_I2S_CLOCKS_H_

/**
 * Our output sample rate is 44.100 kHz which means we need an
 * MCLK frequency of 256 * 44.100 kHz = 11.2896 MHz to drive our audio codec.
 *
 * MCLK is derived from the CPU clock and can be calculated using
 * the following formula:
 *
 * MCLK = (CPU clock frequency * MCLK_MULT) / MCLK_DIV
 *
 * Where:
 * MCLK_MULT is a positive integer with range 1-256 (8 bits)
 * MCLK_DIV is a positive integer with range 1-4096 (12 bits)
 * MCLK_MULT must be <= MCLK_DIV
 *
 */
#if F_CPU == 96000000 || F_CPU == 48000000 || F_CPU == 24000000
  // PLL is at 96 MHz in these modes
  #define MCLK_MULT	147
  #define MCLK_DIV  1250
#elif F_CPU == 72000000
  #define MCLK_MULT 98
  #define MCLK_DIV  625
#elif F_CPU == 120000000
  #define MCLK_MULT 205
  #define MCLK_DIV  2179 // MCLK = 11289582 Hz (off by 18Hz)
#elif F_CPU == 144000000
  #define MCLK_MULT 49
  #define MCLK_DIV  625
#elif F_CPU == 168000000
  #define MCLK_MULT 42
  #define MCLK_DIV  625
#elif F_CPU == 180000000
  #define MCLK_MULT 196
  #define MCLK_DIV  3125
  #define MCLK_SRC  0
#elif F_CPU == 192000000
  #define MCLK_MULT 147
  #define MCLK_DIV  2500
#elif F_CPU == 216000000
  #define MCLK_MULT 98
  #define MCLK_DIV  1875
  #define MCLK_SRC  0
#elif F_CPU == 240000000
  #define MCLK_MULT 147
  #define MCLK_DIV  3125
#elif F_CPU == 16000000
  #define MCLK_MULT 151
  #define MCLK_DIV  214 // MCLK = 11289719 Hz (off by 119Hz)
#else
  #error "This CPU Clock Speed is not supported by the Audio library";
#endif

#ifndef MCLK_SRC
#if F_CPU >= 20000000
  #define MCLK_SRC  3  // the PLL
#else
  #define MCLK_SRC  0  // system clock
#endif
#endif

#endif /* OUTPUT_I2S_CLOCKS_H_ */
