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

#ifndef dspinst_h_
#define dspinst_h_

#include <stdint.h>

// computes limit((val >> rshift), 2**bits)
static inline int32_t signed_saturate_rshift(int32_t val, int bits, int rshift) __attribute__((always_inline, unused));
static inline int32_t signed_saturate_rshift(int32_t val, int bits, int rshift)
{
#if defined(KINETISK) || defined(__SAMD51__)
	int32_t out;
	asm volatile("ssat %0, %1, %2, asr %3" : "=r" (out) : "I" (bits), "r" (val), "I" (rshift));
	return out;
#elif defined(KINETISL)
	int32_t out, max;
	out = val >> rshift;
	max = 1 << (bits - 1);
	if (out >= 0) {
		if (out > max - 1) out = max - 1;
	} else {
		if (out < -max) out = -max;
	}
	return out;
#endif
}

// computes limit(val, 2**bits)
static inline int16_t saturate16(int32_t val) __attribute__((always_inline, unused));
static inline int16_t saturate16(int32_t val)
{
#if defined(KINETISK) || defined(__SAMD51__)
	int16_t out;
	int32_t tmp;
	asm volatile("ssat %0, %1, %2" : "=r" (tmp) : "I" (16), "r" (val) );
	out = (int16_t) (tmp & 0xffff); // not sure if the & 0xffff is necessary. test.
	return out;
#elif defined(KINETISL)
	return 0; // TODO....
#endif
}

// computes ((a[31:0] * b[15:0]) >> 16)
static inline int32_t signed_multiply_32x16b(int32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t signed_multiply_32x16b(int32_t a, uint32_t b)
{
#if defined(KINETISK) || defined(__SAMD51__)
	int32_t out;
	asm volatile("smulwb %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
#elif defined(KINETISL)
	return ((int64_t)a * (int16_t)(b & 0xFFFF)) >> 16;
#endif
}

// computes ((a[31:0] * b[31:16]) >> 16)
static inline int32_t signed_multiply_32x16t(int32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t signed_multiply_32x16t(int32_t a, uint32_t b)
{
#if defined(KINETISK) || defined(__SAMD51__)
	int32_t out;
	asm volatile("smulwt %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
#elif defined(KINETISL)
	return ((int64_t)a * (int16_t)(b >> 16)) >> 16;
#endif
}

// computes (((int64_t)a[31:0] * (int64_t)b[31:0]) >> 32)
static inline int32_t multiply_32x32_rshift32(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_32x32_rshift32(int32_t a, int32_t b)
{
#if defined(KINETISK) || defined(__SAMD51__)
	int32_t out;
	asm volatile("smmul %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
#elif defined(KINETISL)
	return 0; // TODO....
#endif
}

// computes (((int64_t)a[31:0] * (int64_t)b[31:0] + 0x8000000) >> 32)
static inline int32_t multiply_32x32_rshift32_rounded(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_32x32_rshift32_rounded(int32_t a, int32_t b)
{
#if defined(KINETISK) || defined(__SAMD51__)
	int32_t out;
	asm volatile("smmulr %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
#elif defined(KINETISL)
	return 0; // TODO....
#endif
}

// computes sum + (((int64_t)a[31:0] * (int64_t)b[31:0] + 0x8000000) >> 32)
static inline int32_t multiply_accumulate_32x32_rshift32_rounded(int32_t sum, int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_accumulate_32x32_rshift32_rounded(int32_t sum, int32_t a, int32_t b)
{
#if defined(KINETISK) || defined(__SAMD51__)
	int32_t out;
	asm volatile("smmlar %0, %2, %3, %1" : "=r" (out) : "r" (sum), "r" (a), "r" (b));
	return out;
#elif defined(KINETISL)
	return 0; // TODO....
#endif
}

// computes sum - (((int64_t)a[31:0] * (int64_t)b[31:0] + 0x8000000) >> 32)
static inline int32_t multiply_subtract_32x32_rshift32_rounded(int32_t sum, int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_subtract_32x32_rshift32_rounded(int32_t sum, int32_t a, int32_t b)
{
#if defined(KINETISK) || defined(__SAMD51__)
	int32_t out;
	asm volatile("smmlsr %0, %2, %3, %1" : "=r" (out) : "r" (sum), "r" (a), "r" (b));
	return out;
#elif defined(KINETISL)
	return 0; // TODO....
#endif
}


// computes (a[31:16] | (b[31:16] >> 16))
static inline uint32_t pack_16t_16t(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline uint32_t pack_16t_16t(int32_t a, int32_t b)
{
#if defined(KINETISK) || defined(__SAMD51__)
	int32_t out;
	asm volatile("pkhtb %0, %1, %2, asr #16" : "=r" (out) : "r" (a), "r" (b));
	return out;
#elif defined(KINETISL)
	return (a & 0xFFFF0000) | ((uint32_t)b >> 16);
#endif
}

// computes (a[31:16] | b[15:0])
static inline uint32_t pack_16t_16b(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline uint32_t pack_16t_16b(int32_t a, int32_t b)
{
#if defined(KINETISK) || defined(__SAMD51__)
	int32_t out;
	asm volatile("pkhtb %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
#elif defined(KINETISL)
	return (a & 0xFFFF0000) | (b & 0x0000FFFF);
#endif
}

// computes ((a[15:0] << 16) | b[15:0])
static inline uint32_t pack_16b_16b(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline uint32_t pack_16b_16b(int32_t a, int32_t b)
{
#if defined(KINETISK) || defined(__SAMD51__)
	int32_t out;
	asm volatile("pkhbt %0, %1, %2, lsl #16" : "=r" (out) : "r" (b), "r" (a));
	return out;
#elif defined(KINETISL)
	return (a << 16) | (b & 0x0000FFFF);
#endif
}

// computes ((a[15:0] << 16) | b[15:0])
/*
static inline uint32_t pack_16x16(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline uint32_t pack_16x16(int32_t a, int32_t b)
{
	int32_t out;
	asm volatile("pkhbt %0, %1, %2, lsl #16" : "=r" (out) : "r" (b), "r" (a));
	return out;
}
*/

// computes (((a[31:16] + b[31:16]) << 16) | (a[15:0 + b[15:0]))  (saturates)
static inline uint32_t signed_add_16_and_16(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline uint32_t signed_add_16_and_16(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("qadd16 %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes (((a[31:16] - b[31:16]) << 16) | (a[15:0 - b[15:0]))  (saturates)
static inline int32_t signed_subtract_16_and_16(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline int32_t signed_subtract_16_and_16(int32_t a, int32_t b)
{
	int32_t out;
	asm volatile("qsub16 %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes out = (((a[31:16]+b[31:16])/2) <<16) | ((a[15:0]+b[15:0])/2)
static inline int32_t signed_halving_add_16_and_16(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline int32_t signed_halving_add_16_and_16(int32_t a, int32_t b)
{
	int32_t out;
	asm volatile("shadd16 %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes out = (((a[31:16]-b[31:16])/2) <<16) | ((a[15:0]-b[15:0])/2)
static inline int32_t signed_halving_subtract_16_and_16(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline int32_t signed_halving_subtract_16_and_16(int32_t a, int32_t b)
{
	int32_t out;
	asm volatile("shsub16 %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes (sum + ((a[31:0] * b[15:0]) >> 16))
static inline int32_t signed_multiply_accumulate_32x16b(int32_t sum, int32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t signed_multiply_accumulate_32x16b(int32_t sum, int32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smlawb %0, %2, %3, %1" : "=r" (out) : "r" (sum), "r" (a), "r" (b));
	return out;
}

// computes (sum + ((a[31:0] * b[31:16]) >> 16))
static inline int32_t signed_multiply_accumulate_32x16t(int32_t sum, int32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t signed_multiply_accumulate_32x16t(int32_t sum, int32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smlawt %0, %2, %3, %1" : "=r" (out) : "r" (sum), "r" (a), "r" (b));
	return out;
}

// computes logical and, forces compiler to allocate register and use single cycle instruction
static inline uint32_t logical_and(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline uint32_t logical_and(uint32_t a, uint32_t b)
{
	asm volatile("and %0, %1" : "+r" (a) : "r" (b));
	return a;
}

// computes ((a[15:0] * b[15:0]) + (a[31:16] * b[31:16]))
static inline int32_t multiply_16tx16t_add_16bx16b(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_16tx16t_add_16bx16b(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smuad %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes ((a[15:0] * b[31:16]) + (a[31:16] * b[15:0]))
static inline int32_t multiply_16tx16b_add_16bx16t(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_16tx16b_add_16bx16t(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smuadx %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// // computes sum += ((a[15:0] * b[15:0]) + (a[31:16] * b[31:16]))
static inline int64_t multiply_accumulate_16tx16t_add_16bx16b(int64_t sum, uint32_t a, uint32_t b)
{
	asm volatile("smlald %Q0, %R0, %1, %2" : "+r" (sum) : "r" (a), "r" (b));
	return sum;
}

// // computes sum += ((a[15:0] * b[31:16]) + (a[31:16] * b[15:0]))
static inline int64_t multiply_accumulate_16tx16b_add_16bx16t(int64_t sum, uint32_t a, uint32_t b)
{
	asm volatile("smlaldx %Q0, %R0, %1, %2" : "+r" (sum) : "r" (a), "r" (b));
	return sum;
}

// computes ((a[15:0] * b[15:0])
static inline int32_t multiply_16bx16b(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_16bx16b(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smulbb %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes ((a[15:0] * b[31:16])
static inline int32_t multiply_16bx16t(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_16bx16t(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smulbt %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes ((a[31:16] * b[15:0])
static inline int32_t multiply_16tx16b(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_16tx16b(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smultb %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes ((a[31:16] * b[31:16])
static inline int32_t multiply_16tx16t(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_16tx16t(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smultt %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes (a - b), result saturated to 32 bit integer range
static inline int32_t substract_32_saturate(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t substract_32_saturate(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("qsub %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

//get Q from PSR
static inline uint32_t get_q_psr(void) __attribute__((always_inline, unused));
static inline uint32_t get_q_psr(void)
{
  uint32_t out;
  asm ("mrs %0, APSR" : "=r" (out));
  return (out & 0x8000000)>>27;
}

//clear Q BIT in PSR
static inline void clr_q_psr(void) __attribute__((always_inline, unused));
static inline void clr_q_psr(void)
{
  uint32_t t;
  asm ("mov %[t],#0\n"
       "msr APSR_nzcvq,%0\n" : [t] "=&r" (t)::"cc");
}

// Multiply two S.31 fractional integers, and return the 32 most significant
// bits after a shift left by the constant z.
// This comes from rockbox.org

static inline int32_t FRACMUL_SHL(int32_t x, int32_t y, int z)
{
    int32_t t, t2;
    asm ("smull    %[t], %[t2], %[a], %[b]\n\t"
         "mov      %[t2], %[t2], asl %[c]\n\t"
         "orr      %[t], %[t2], %[t], lsr %[d]\n\t"
         : [t] "=&r" (t), [t2] "=&r" (t2)
         : [a] "r" (x), [b] "r" (y),
           [c] "Mr" ((z) + 1), [d] "Mr" (31 - (z)));
    return t;
}

#endif
