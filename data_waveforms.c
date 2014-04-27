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

// These audio waveforms have a period of 256 points, plus a 257th
// point that is a duplicate of the first point.  This duplicate
// is needed because the waveform generator uses linear interpolation
// between each point and the next point in the waveform.

const int16_t AudioWaveformSine[257] = {
     0,   804,  1608,  2410,  3212,  4011,  4808,  5602,  6393,  7179,
  7962,  8739,  9512, 10278, 11039, 11793, 12539, 13279, 14010, 14732,
 15446, 16151, 16846, 17530, 18204, 18868, 19519, 20159, 20787, 21403,
 22005, 22594, 23170, 23731, 24279, 24811, 25329, 25832, 26319, 26790,
 27245, 27683, 28105, 28510, 28898, 29268, 29621, 29956, 30273, 30571,
 30852, 31113, 31356, 31580, 31785, 31971, 32137, 32285, 32412, 32521,
 32609, 32678, 32728, 32757, 32767, 32757, 32728, 32678, 32609, 32521,
 32412, 32285, 32137, 31971, 31785, 31580, 31356, 31113, 30852, 30571,
 30273, 29956, 29621, 29268, 28898, 28510, 28105, 27683, 27245, 26790,
 26319, 25832, 25329, 24811, 24279, 23731, 23170, 22594, 22005, 21403,
 20787, 20159, 19519, 18868, 18204, 17530, 16846, 16151, 15446, 14732,
 14010, 13279, 12539, 11793, 11039, 10278,  9512,  8739,  7962,  7179,
  6393,  5602,  4808,  4011,  3212,  2410,  1608,   804,     0,  -804,
 -1608, -2410, -3212, -4011, -4808, -5602, -6393, -7179, -7962, -8739,
 -9512,-10278,-11039,-11793,-12539,-13279,-14010,-14732,-15446,-16151,
-16846,-17530,-18204,-18868,-19519,-20159,-20787,-21403,-22005,-22594,
-23170,-23731,-24279,-24811,-25329,-25832,-26319,-26790,-27245,-27683,
-28105,-28510,-28898,-29268,-29621,-29956,-30273,-30571,-30852,-31113,
-31356,-31580,-31785,-31971,-32137,-32285,-32412,-32521,-32609,-32678,
-32728,-32757,-32767,-32757,-32728,-32678,-32609,-32521,-32412,-32285,
-32137,-31971,-31785,-31580,-31356,-31113,-30852,-30571,-30273,-29956,
-29621,-29268,-28898,-28510,-28105,-27683,-27245,-26790,-26319,-25832,
-25329,-24811,-24279,-23731,-23170,-22594,-22005,-21403,-20787,-20159,
-19519,-18868,-18204,-17530,-16846,-16151,-15446,-14732,-14010,-13279,
-12539,-11793,-11039,-10278, -9512, -8739, -7962, -7179, -6393, -5602,
 -4808, -4011, -3212, -2410, -1608,  -804,     0
};

#if 0
#! /usr/bin/perl
use Math::Trig ':pi';
$len = 256;
print "const int16_t AudioWaveformSine[257] = {\n";
for ($i=0; $i <= $len; $i++) {
        $f = sin($i / $len * 2 * pi);
        $d = sprintf "%.0f", $f * 32767.0;
        #print $d;
        printf "%6d", $d + 0;
        print "," if ($i < $len);
        print "\n" if ($i % 10) == 9;
}
print "\n" unless ($len % 10) == 9;
print "};\n";
#endif


const int16_t fader_table[257] = {
    0,    1,    4,   11,   19,   30,   44,   60,   78,   99,
  123,  149,  177,  208,  241,  276,  314,  355,  398,  443,
  490,  541,  593,  648,  705,  764,  826,  891,  957, 1026,
 1097, 1171, 1247, 1325, 1405, 1488, 1572, 1660, 1749, 1840,
 1934, 2030, 2128, 2228, 2330, 2435, 2541, 2650, 2761, 2873,
 2988, 3105, 3224, 3344, 3467, 3592, 3718, 3847, 3977, 4109,
 4243, 4379, 4517, 4657, 4798, 4941, 5086, 5232, 5380, 5530,
 5682, 5835, 5989, 6145, 6303, 6462, 6623, 6785, 6949, 7114,
 7281, 7448, 7618, 7788, 7960, 8133, 8307, 8483, 8660, 8838,
 9017, 9197, 9378, 9560, 9743, 9928,10113,10299,10486,10674,
10863,11053,11244,11435,11627,11820,12013,12207,12402,12597,
12793,12989,13186,13384,13582,13780,13979,14178,14377,14577,
14777,14977,15177,15378,15579,15780,15981,16182,16383,16584,
16785,16986,17187,17387,17588,17788,17989,18188,18388,18588,
18787,18985,19184,19382,19579,19776,19972,20168,20364,20558,
20752,20946,21139,21331,21522,21712,21902,22091,22279,22466,
22652,22838,23022,23205,23388,23569,23749,23928,24106,24283,
24458,24633,24806,24977,25148,25317,25485,25651,25817,25980,
26142,26303,26463,26620,26776,26931,27084,27236,27385,27534,
27680,27825,27968,28109,28249,28386,28522,28656,28789,28919,
29048,29174,29299,29422,29542,29661,29778,29893,30006,30116,
30225,30331,30436,30538,30638,30736,30832,30926,31017,31107,
31194,31279,31361,31442,31520,31596,31669,31740,31809,31876,
31940,32002,32062,32119,32174,32226,32276,32324,32369,32412,
32452,32490,32526,32559,32590,32618,32644,32667,32688,32707,
32723,32737,32748,32756,32763,32766,32767
};
#if 0
#! /usr/bin/perl
print "const int16_t fader_table[257] = {\n";
$len = 256;
for ($i=0; $i < $len+1; $i++) {
        $a = cos(3.14149 * $i / $len);
        $in = (1 - $a) / 2;
        $d = $in * 32768;
        $d = 32767 if $d >= 32767.5;
        printf "%5d", $d;
        print "," if ($i < $len);
        print "\n" if ($i % 10) == 9;
}
print "\n};\n";
#endif
