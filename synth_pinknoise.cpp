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

// http://stenzel.waldorfmusic.de/post/pink/
// https://github.com/Stenzel/newshadeofpink
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// New Shade of Pink
// (c) 2014 Stefan Stenzel
// stefan at waldorfmusic.de
//  
// Terms of use:
// Use for any purpose. If used in a commercial product, you should give me one.  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#include "synth_pinknoise.h"

int16_t AudioSynthNoisePink::instance_cnt = 0;

// Let preprocessor and compiler calculate two lookup tables for 12-tap FIR Filter
// with these coefficients: 1.190566, 0.162580, 0.002208, 0.025475, -0.001522,
// 0.007322, 0.001774, 0.004529, -0.001561, 0.000776, -0.000486, 0.002017
#define Fn(cf,m,shift)   (2048*cf*(2*((m)>>shift&1)-1))
#define FA(n)	(int32_t)(Fn(1.190566,n,0)+Fn(0.162580,n,1)+Fn(0.002208,n,2)+\
		Fn(0.025475,n,3)+Fn(-0.001522,n,4)+Fn(0.007322,n,5))
#define FB(n)   (int32_t)(Fn(0.001774,n,0)+Fn(0.004529,n,1)+Fn(-0.001561,n,2)+\
		Fn(0.000776,n,3)+Fn(-0.000486,n,4)+Fn(0.002017,n,5))
#define FA8(n)  FA(n),FA(n+1),FA(n+2),FA(n+3),FA(n+4),FA(n+5),FA(n+6),FA(n+7)
#define FB8(n)  FB(n),FB(n+1),FB(n+2),FB(n+3),FB(n+4),FB(n+5),FB(n+6),FB(n+7)
const int32_t AudioSynthNoisePink::pfira[64] = // 1st FIR lookup table
	{FA8(0),FA8(8),FA8(16),FA8(24),FA8(32),FA8(40),FA8(48),FA8(56)};
const int32_t AudioSynthNoisePink::pfirb[64] = // 2nd FIR lookup table
	{FB8(0),FB8(8),FB8(16),FB8(24),FB8(32),FB8(40),FB8(48),FB8(56)};

// bitreversed lookup table
#define PM16(n) n,0x80,0x40,0x80,0x20,0x80,0x40,0x80,0x10,0x80,0x40,0x80,0x20,0x80,0x40,0x80
const uint8_t AudioSynthNoisePink::pnmask[256] = {
    PM16(0),PM16(8),PM16(4),PM16(8),PM16(2),PM16(8),PM16(4),PM16(8),
    PM16(1),PM16(8),PM16(4),PM16(8),PM16(2),PM16(8),PM16(4),PM16(8)
};

#define PINT(bitmask, out)           /* macro for processing:           */\
	bit = lfsr >> 31;            /* spill random to all bits        */\
	dec &= ~bitmask;             /* blank old decrement bit         */\
	lfsr <<= 1;                  /* shift lfsr                      */\
	dec |= inc & bitmask;        /* copy increment to decrement bit */\
	inc ^= bit & bitmask;        /* new random bit                  */\
	accu += inc - dec;           /* integrate                       */\
	lfsr ^= bit & taps;          /* update lfsr                     */\
	out = accu +                 /* save output                     */\
	  pfira[lfsr & 0x3F] +       /* add 1st half precalculated FIR  */\
	  pfirb[lfsr >> 6 & 0x3F]    /* add 2nd half, also correts bias */

void AudioSynthNoisePink::update(void)
{
	audio_block_t *block;
	uint32_t *p, *end;
	int32_t n1, n2;
	int32_t gain;
	int32_t inc, dec, accu, bit, lfsr;
	int32_t taps;

	gain = level;
	if (gain == 0) return;
	block = allocate();
	if (!block) return;
	p = (uint32_t *)(block->data);
	end = p + AUDIO_BLOCK_SAMPLES/2;
	taps = 0x46000001;
	inc = pinc;
	dec = pdec;
	accu = paccu;
	lfsr = plfsr;
	do {
		int32_t mask = pnmask[pncnt++];
		PINT(mask, n1);
		n1 = signed_multiply_32x16b(gain, n1);
		PINT(0x0800, n2);
		n2 = signed_multiply_32x16b(gain, n2);
		*p++ = pack_16b_16b(n2, n1);
		PINT(0x0400, n1);
		n1 = signed_multiply_32x16b(gain, n1);
		PINT(0x0800, n2);
		n2 = signed_multiply_32x16b(gain, n2);
		*p++ = pack_16b_16b(n2, n1);
		PINT(0x0200, n1);
		n1 = signed_multiply_32x16b(gain, n1);
		PINT(0x0800, n2);
		n2 = signed_multiply_32x16b(gain, n2);
		*p++ = pack_16b_16b(n2, n1);
		PINT(0x0400, n1);
		n1 = signed_multiply_32x16b(gain, n1);
		PINT(0x0800, n2);
		n2 = signed_multiply_32x16b(gain, n2);
		*p++ = pack_16b_16b(n2, n1);
		PINT(0x0100, n1);
		n1 = signed_multiply_32x16b(gain, n1);
		PINT(0x0800, n2);
		n2 = signed_multiply_32x16b(gain, n2);
		*p++ = pack_16b_16b(n2, n1);
		PINT(0x0400, n1);
		n1 = signed_multiply_32x16b(gain, n1);
		PINT(0x0800, n2);
		n2 = signed_multiply_32x16b(gain, n2);
		*p++ = pack_16b_16b(n2, n1);
		PINT(0x0200, n1);
		n1 = signed_multiply_32x16b(gain, n1);
		PINT(0x0800, n2);
		n2 = signed_multiply_32x16b(gain, n2);
		*p++ = pack_16b_16b(n2, n1);
		PINT(0x0400, n1);
		n1 = signed_multiply_32x16b(gain, n1);
		PINT(0x0800, n2);
		n2 = signed_multiply_32x16b(gain, n2);
		*p++ = pack_16b_16b(n2, n1);
	} while (p < end);
	pinc = inc;
	pdec = dec;
	paccu = accu;
	plfsr = lfsr;
	transmit(block);
	release(block);
}



