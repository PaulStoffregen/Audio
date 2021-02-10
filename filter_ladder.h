/* Audio Library for Teensy, Ladder Filter
 * Copyright (c) 2021, Richard van Hoesel
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

//-----------------------------------------------------------
// Huovilainen New Moog (HNM) model as per CMJ jun 2006
// Implemented as Teensy Audio Library compatible object
// Richard van Hoesel, Feb. 9 2021
// v.1.02 now includes both cutoff and resonance "CV" modulation inputs
// please retain this header if you use this code.
//-----------------------------------------------------------

// https://forum.pjrc.com/threads/60488?p=269609&viewfull=1#post269609

#ifndef filter_ladder_h_
#define filter_ladder_h_

#include "Arduino.h"
#include "AudioStream.h"

class AudioFilterLadder: public AudioStream
{
public:
	AudioFilterLadder() : AudioStream(3, inputQueueArray) {};
	void frequency(float FC);
	void resonance(float reson);
	virtual void update(void);
private:
	float LPF(float s, int i);
	void compute_coeffs(float fc);
	bool resonating();
	float alpha = 1.0;
	float beta[4] = {0.0, 0.0, 0.0, 0.0};
	float z0[4] = {0.0, 0.0, 0.0, 0.0};
	float z1[4] = {0.0, 0.0, 0.0, 0.0};
	float K = 1.0;
	float Fbase = 1000;
	//float Qbase = 0.5;
	//float overdrive = 1.0;
	audio_block_t *inputQueueArray[3];
};

#endif
