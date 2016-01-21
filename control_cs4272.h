/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * https://hackaday.io/project/5912-teensy-super-audio-board
 * https://github.com/whollender/Audio/tree/SuperAudioBoard
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

#ifndef control_cs4272_h_
#define control_cs4272_h_

#include "AudioControl.h"

class AudioControlCS4272 : public AudioControl
{
public:
	bool enable(void);
	bool disable(void) { return false; }
	bool volume(float n) { return volumeInteger(n * 127 + 0.499); }
	bool inputLevel(float n) { return false; }
	bool inputSelect(int n) { return false; }

	bool volume(float left, float right);
	bool dacVolume(float n) { return volumeInteger(n * 127 + 0.499); }
	bool dacVolume(float left, float right);

	bool muteOutput(void);
	bool unmuteOutput(void);

	bool muteInput(void);
	bool unmuteInput(void);

	bool enableDither(void);
	bool disableDither(void);

protected:
	bool write(unsigned int reg, unsigned int val);
	bool volumeInteger(unsigned int n); // range: 0x0 to 0x7F
	
	uint8_t regLocal[8];

	void initLocalRegs(void);
};

// For sample rate ratio select (only single speed tested)
#define CS4272_RATIO_SINGLE 0
#define CS4272_RATIO_DOUBLE 2
#define CS4272_RATIO_QUAD 3

#endif
