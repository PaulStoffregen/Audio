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

const int16_t ulaw_decode_table[256] = {
     4,    12,    20,    28,    36,    44,    52,    60,    68,    76,
    84,    92,   100,   108,   116,   124,   136,   152,   168,   184,
   200,   216,   232,   248,   264,   280,   296,   312,   328,   344,
   360,   376,   400,   432,   464,   496,   528,   560,   592,   624,
   656,   688,   720,   752,   784,   816,   848,   880,   928,   992,
  1056,  1120,  1184,  1248,  1312,  1376,  1440,  1504,  1568,  1632,
  1696,  1760,  1824,  1888,  1984,  2112,  2240,  2368,  2496,  2624,
  2752,  2880,  3008,  3136,  3264,  3392,  3520,  3648,  3776,  3904,
  4096,  4352,  4608,  4864,  5120,  5376,  5632,  5888,  6144,  6400,
  6656,  6912,  7168,  7424,  7680,  7936,  8320,  8832,  9344,  9856,
 10368, 10880, 11392, 11904, 12416, 12928, 13440, 13952, 14464, 14976,
 15488, 16000, 16768, 17792, 18816, 19840, 20864, 21888, 22912, 23936,
 24960, 25984, 27008, 28032, 29056, 30080, 31104, 32128,    -4,   -12,
   -20,   -28,   -36,   -44,   -52,   -60,   -68,   -76,   -84,   -92,
  -100,  -108,  -116,  -124,  -136,  -152,  -168,  -184,  -200,  -216,
  -232,  -248,  -264,  -280,  -296,  -312,  -328,  -344,  -360,  -376,
  -400,  -432,  -464,  -496,  -528,  -560,  -592,  -624,  -656,  -688,
  -720,  -752,  -784,  -816,  -848,  -880,  -928,  -992, -1056, -1120,
 -1184, -1248, -1312, -1376, -1440, -1504, -1568, -1632, -1696, -1760,
 -1824, -1888, -1984, -2112, -2240, -2368, -2496, -2624, -2752, -2880,
 -3008, -3136, -3264, -3392, -3520, -3648, -3776, -3904, -4096, -4352,
 -4608, -4864, -5120, -5376, -5632, -5888, -6144, -6400, -6656, -6912,
 -7168, -7424, -7680, -7936, -8320, -8832, -9344, -9856,-10368,-10880,
-11392,-11904,-12416,-12928,-13440,-13952,-14464,-14976,-15488,-16000,
-16768,-17792,-18816,-19840,-20864,-21888,-22912,-23936,-24960,-25984,
-27008,-28032,-29056,-30080,-31104,-32128
};
/*
#! /usr/bin/perl
print "const int16_t ulaw_decode_table[256] = {\n";
for ($i=0; $i < 256; $i++) {
	$r = ($i >> 4) & 7;
	$n = ($i & 0xF) << ($r + 3);
	$n |= 1 << ($r + 7);
	$n |= 1 << ($r + 2);
	$n -= 128;
	$n *= -1 if $i > 127;
        printf "%6d", $n + 0;
        print "," if ($i < 255);
        print "\n" if ($i % 10) == 9;
}
print "\n};\n";
*/



/*
const int16_t ulaw_decode_table[256] = {
     0,     8,    16,    24,    32,    40,    48,    56,    64,    72,
    80,    88,    96,   104,   112,   120,   128,   144,   160,   176,
   192,   208,   224,   240,   256,   272,   288,   304,   320,   336,
   352,   368,   384,   416,   448,   480,   512,   544,   576,   608,
   640,   672,   704,   736,   768,   800,   832,   864,   896,   960,
  1024,  1088,  1152,  1216,  1280,  1344,  1408,  1472,  1536,  1600,
  1664,  1728,  1792,  1856,  1920,  2048,  2176,  2304,  2432,  2560,
  2688,  2816,  2944,  3072,  3200,  3328,  3456,  3584,  3712,  3840,
  3968,  4224,  4480,  4736,  4992,  5248,  5504,  5760,  6016,  6272,
  6528,  6784,  7040,  7296,  7552,  7808,  8064,  8576,  9088,  9600,
 10112, 10624, 11136, 11648, 12160, 12672, 13184, 13696, 14208, 14720,
 15232, 15744, 16256, 17280, 18304, 19328, 20352, 21376, 22400, 23424,
 24448, 25472, 26496, 27520, 28544, 29568, 30592, 31616,     0,    -8,
   -16,   -24,   -32,   -40,   -48,   -56,   -64,   -72,   -80,   -88,
   -96,  -104,  -112,  -120,  -128,  -144,  -160,  -176,  -192,  -208,
  -224,  -240,  -256,  -272,  -288,  -304,  -320,  -336,  -352,  -368,
  -384,  -416,  -448,  -480,  -512,  -544,  -576,  -608,  -640,  -672,
  -704,  -736,  -768,  -800,  -832,  -864,  -896,  -960, -1024, -1088,
 -1152, -1216, -1280, -1344, -1408, -1472, -1536, -1600, -1664, -1728,
 -1792, -1856, -1920, -2048, -2176, -2304, -2432, -2560, -2688, -2816,
 -2944, -3072, -3200, -3328, -3456, -3584, -3712, -3840, -3968, -4224,
 -4480, -4736, -4992, -5248, -5504, -5760, -6016, -6272, -6528, -6784,
 -7040, -7296, -7552, -7808, -8064, -8576, -9088, -9600,-10112,-10624,
-11136,-11648,-12160,-12672,-13184,-13696,-14208,-14720,-15232,-15744,
-16256,-17280,-18304,-19328,-20352,-21376,-22400,-23424,-24448,-25472,
-26496,-27520,-28544,-29568,-30592,-31616
};
*/
/*
#! /usr/bin/perl
print "const int16_t ulaw_decode_table[256] = {\n";
for ($i=0; $i < 256; $i++) {
	$r = ($i >> 4) & 7;
	$n = ($i & 0xF) << ($r + 3);
	$n |= 1 << ($r + 7);
	$n -= 128;
	$n *= -1 if $i > 127;
        printf "%6d", $n + 0;
        print "," if ($i < 255);
        print "\n" if ($i % 10) == 9;
}
print "\n};\n";
*/
