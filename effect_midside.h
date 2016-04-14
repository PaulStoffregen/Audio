/* Audio Library for Teensy 3.X
 * Copyright (c) 2015, Hedde Bosman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef effect_midside_decode_h_
#define effect_midside_decode_h_

#include "Arduino.h"
#include "AudioStream.h"

#include "utility/dspinst.h"


/**
 * This object performs the decoding of mid/side signals into a stereo signal.
 * That is, left = (mid+side), right = (mid-side)
 * After encoding and processing, this object decodes the mid/side components 
 * back into enjoyable stereo audio.
 * Caution: processed mid/side signals may cause digital saturation (clipping).
 ***************************************************************/
class AudioEffectMidSide : public AudioStream
{
public:
	AudioEffectMidSide(void): AudioStream(2,inputQueueArray), encoding(true) { }
	void encode() { encoding = true; }
	void decode() { encoding = false; }
	virtual void update(void);
private:
	bool encoding;
	audio_block_t *inputQueueArray[2];
};

#endif

