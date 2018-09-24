/* Audio Library for Teensy 3.X
 * Copyright (c) 2018, Paul Stoffregen, paul@pjrc.com
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

#ifndef effect_freeverb_h_
#define effect_freeverb_h_
#include <Arduino.h>
#include "AudioStream.h"

class AudioEffectFreeverb : public AudioStream
{
public:
	AudioEffectFreeverb();
	virtual void update();
	void roomsize(float n) {
		if (n > 1.0f) n = 1.0f;
		else if (n < 0.0) n = 0.0f;
		combfeeback = (int)(n * 9175.04f) + 22937;
	}
	void damping(float n) {
		if (n > 1.0f) n = 1.0f;
		else if (n < 0.0) n = 0.0f;
		int x1 = (int)(n * 13107.2f);
		int x2 = 32768 - x1;
		__disable_irq();
		combdamp1 = x1;
		combdamp2 = x2;
		__enable_irq();
	}
private:
	audio_block_t *inputQueueArray[1];
	int16_t comb1buf[1116];
	int16_t comb2buf[1188];
	int16_t comb3buf[1277];
	int16_t comb4buf[1356];
	int16_t comb5buf[1422];
	int16_t comb6buf[1491];
	int16_t comb7buf[1557];
	int16_t comb8buf[1617];
	uint16_t comb1index;
	uint16_t comb2index;
	uint16_t comb3index;
	uint16_t comb4index;
	uint16_t comb5index;
	uint16_t comb6index;
	uint16_t comb7index;
	uint16_t comb8index;
	int16_t comb1filter;
	int16_t comb2filter;
	int16_t comb3filter;
	int16_t comb4filter;
	int16_t comb5filter;
	int16_t comb6filter;
	int16_t comb7filter;
	int16_t comb8filter;
	int16_t combdamp1;
	int16_t combdamp2;
	int16_t combfeeback;
	int16_t allpass1buf[556];
	int16_t allpass2buf[441];
	int16_t allpass3buf[341];
	int16_t allpass4buf[225];
	uint16_t allpass1index;
	uint16_t allpass2index;
	uint16_t allpass3index;
	uint16_t allpass4index;
};


class AudioEffectFreeverbStereo : public AudioStream
{
public:
	AudioEffectFreeverbStereo();
	virtual void update();
	void roomsize(float n) {
		if (n > 1.0f) n = 1.0f;
		else if (n < 0.0) n = 0.0f;
		combfeeback = (int)(n * 9175.04f) + 22937;
	}
	void damping(float n) {
		if (n > 1.0f) n = 1.0f;
		else if (n < 0.0) n = 0.0f;
		int x1 = (int)(n * 13107.2f);
		int x2 = 32768 - x1;
		__disable_irq();
		combdamp1 = x1;
		combdamp2 = x2;
		__enable_irq();
	}
private:
	audio_block_t *inputQueueArray[1];
	int16_t comb1bufL[1116];
	int16_t comb2bufL[1188];
	int16_t comb3bufL[1277];
	int16_t comb4bufL[1356];
	int16_t comb5bufL[1422];
	int16_t comb6bufL[1491];
	int16_t comb7bufL[1557];
	int16_t comb8bufL[1617];
	uint16_t comb1indexL;
	uint16_t comb2indexL;
	uint16_t comb3indexL;
	uint16_t comb4indexL;
	uint16_t comb5indexL;
	uint16_t comb6indexL;
	uint16_t comb7indexL;
	uint16_t comb8indexL;
	int16_t comb1filterL;
	int16_t comb2filterL;
	int16_t comb3filterL;
	int16_t comb4filterL;
	int16_t comb5filterL;
	int16_t comb6filterL;
	int16_t comb7filterL;
	int16_t comb8filterL;
	int16_t comb1bufR[1139];
	int16_t comb2bufR[1211];
	int16_t comb3bufR[1300];
	int16_t comb4bufR[1379];
	int16_t comb5bufR[1445];
	int16_t comb6bufR[1514];
	int16_t comb7bufR[1580];
	int16_t comb8bufR[1640];
	uint16_t comb1indexR;
	uint16_t comb2indexR;
	uint16_t comb3indexR;
	uint16_t comb4indexR;
	uint16_t comb5indexR;
	uint16_t comb6indexR;
	uint16_t comb7indexR;
	uint16_t comb8indexR;
	int16_t comb1filterR;
	int16_t comb2filterR;
	int16_t comb3filterR;
	int16_t comb4filterR;
	int16_t comb5filterR;
	int16_t comb6filterR;
	int16_t comb7filterR;
	int16_t comb8filterR;
	int16_t combdamp1;
	int16_t combdamp2;
	int16_t combfeeback;
	int16_t allpass1bufL[556];
	int16_t allpass2bufL[441];
	int16_t allpass3bufL[341];
	int16_t allpass4bufL[225];
	uint16_t allpass1indexL;
	uint16_t allpass2indexL;
	uint16_t allpass3indexL;
	uint16_t allpass4indexL;
	int16_t allpass1bufR[579];
	int16_t allpass2bufR[464];
	int16_t allpass3bufR[364];
	int16_t allpass4bufR[248];
	uint16_t allpass1indexR;
	uint16_t allpass2indexR;
	uint16_t allpass3indexR;
	uint16_t allpass4indexR;
};


#endif

