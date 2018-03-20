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
	//AudioEffectFreeverb() : AudioStream(1, inputQueueArray);
	AudioEffectFreeverb();
	virtual void update();
private:
	audio_block_t *inputQueueArray[2];
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

#endif

