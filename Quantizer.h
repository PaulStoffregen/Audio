/* Audio Library for Teensy 3.X
 * Copyright (c) 2019, Paul Stoffregen, paul@pjrc.com
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
/*
 by Alexander Walch
 */
#ifndef quantizer_h_
#define quantizer_h_

#include "Arduino.h"

//#define DEBUG_QUANTIZER

#define NOISE_SHAPE_F_LENGTH 9  //order of filter is 10, but the first coefficient equals 1 and doesn't need to be stored

class Quantizer {
public:
    ///@param audio_sample_rate currently only 44.1kHz and 48kHz are supported
    Quantizer(float audio_sample_rate);
    void configure(bool noiseShaping, bool dither, float factor);
    void quantize(float* input, int16_t* output, uint16_t length);
    //attention outputInterleaved must have length 2*length
    void quantize(float* input0, float* input1, int32_t* outputInterleaved, uint16_t length);
    void reset();
        
private:

bool _noiseShaping=true;
bool _dither=true;
float _fOutputLastIt0=0.f;
float _fOutputLastIt1=0.f;
float _buffer0[NOISE_SHAPE_F_LENGTH];
float _buffer1[NOISE_SHAPE_F_LENGTH];
float* _bPtr0=_buffer0;
float* _bufferEnd0;
float* _bPtr1=_buffer1;
float _noiseSFilter[NOISE_SHAPE_F_LENGTH ];
float _factor;

};

#endif