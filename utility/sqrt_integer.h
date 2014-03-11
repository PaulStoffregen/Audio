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

#include <stdint.h>

#ifdef __cplusplus
extern "C" const uint16_t sqrt_integer_guess_table[];
#else
extern const uint16_t sqrt_integer_guess_table[];
#endif

inline uint32_t sqrt_uint32(uint32_t in) __attribute__((always_inline,unused));
inline uint32_t sqrt_uint32(uint32_t in)
{
	uint32_t n = sqrt_integer_guess_table[__builtin_clz(in)];
	n = ((in / n) + n) / 2;
	n = ((in / n) + n) / 2;
	n = ((in / n) + n) / 2;
	return n;
}

inline uint32_t sqrt_uint32_approx(uint32_t in) __attribute__((always_inline,unused));
inline uint32_t sqrt_uint32_approx(uint32_t in)
{
	uint32_t n = sqrt_integer_guess_table[__builtin_clz(in)];
	n = ((in / n) + n) / 2;
	n = ((in / n) + n) / 2;
	return n;
}

