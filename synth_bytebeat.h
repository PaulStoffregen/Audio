/* Audio Library for Teensy 3.X
 * Copyright (c) 2017, Oren Leiman
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

/*********************************************************************************
 * NB: The bytebeat equations defined in synth_bytebeat.cpp were adapted
 * from the 'Viznutcracker, Sweet!' mode of the Ornaments and Crimes firmware
 * for the Ornament and Crime eurorack module, developed by Max Stadler, Patrick
 * Dowling, and Tim Churches. These equations were, in turn adapted from various
 * sources, as noted in comments. The original O_C firmware can be found here, as
 * of August 30, 2017:
 * https://github.com/mxmxmx/O_C
*********************************************************************************/

#ifndef synth_bytebeat_h
#define synth_bytebeat_h

#include "Arduino.h"
#include "AudioStream.h"
#include "utility/dspinst.h"
#include <vector>

#define N_EQUATIONS 16

class AudioSynthByteBeat : public AudioStream
{
 public:
    AudioSynthByteBeat() : AudioStream(0, NULL)
    {
	p0_ = 127;
	p1_ = 127;
	p2_ = 127;
	pitch_ = 1;
	bytepitch_ = 1;
	equation_select_ = 1;
    }
    
    inline void equation(uint8_t param)
    {
	equation_select_ = param % N_EQUATIONS;
    }

    inline void p0(uint8_t param)
    {
	p0_ = param;
    }

    inline void p1(uint8_t param)
    {
	p1_ = param;
    }

    inline void p2(uint8_t param)
    {
	p2_ = param;
    }

    inline void pitch(uint8_t param)
    {
	pitch_ = param;
    }

    inline void bytepitch(uint8_t param)
    {
	bytepitch_ = param;
    }

    virtual void update(void);

 private:
    uint32_t t_ = 0;
    uint32_t phase_ = 0;
    uint16_t last_sample_ = 0;
    uint8_t p0_;
    uint8_t p1_;
    uint8_t p2_;
    uint8_t pitch_;
    uint8_t bytepitch_;
    uint8_t equation_select_;
};

#endif
