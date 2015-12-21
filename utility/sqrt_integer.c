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

#include "sqrt_integer.h"

const uint16_t sqrt_integer_guess_table[33] = {
55109,
38968,
27555,
19484,
13778,
 9742,
 6889,
 4871,
 3445,
 2436,
 1723,
 1218,
  862,
  609,
  431,
  305,
  216,
  153,
  108,
   77,
   54,
   39,
   27,
   20,
   14,
   10,
    7,
    5,
    4,
    3,
    2,
    1,
    0
};
/*
#! /usr/bin/perl
use POSIX;
print "const uint16_t sqrt_integer_guess_table[33] = {\n";
for ($i=0; $i <= 32; $i++) {
	printf "%5d", ceil(sqrt((0xFFFFFFFF >> $i) * sqrt(2)/2 ));
	print "," if $i < 32;
	print "\n";
}
print "};\n";
*/
