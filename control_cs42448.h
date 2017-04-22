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

#ifndef control_cs42448_h_
#define control_cs42448_h_

#include "AudioControl.h"
#include <math.h>

class AudioControlCS42448 : public AudioControl
{
public:
	AudioControlCS42448(void) : i2c_addr(0x48), muted(true) { }
	void setAddress(uint8_t addr) {
		i2c_addr = 0x48 | (addr & 3);
	}
	bool enable(void);
	bool disable(void) {
		return false;
	}
	bool volume(float level) {
		return volumeInteger(volumebyte(level));
	}
	bool inputLevel(float level) {
		return inputLevelInteger(inputlevelbyte(level));
	}
	bool inputSelect(int n) {
		return (n == 0) ? true : false;
	}
	bool volume(int channel, float level) {
		if (channel < 1 || channel > 8) return false;
		return volumeInteger(channel, volumebyte(level)); 
	}
	bool inputLevel(int channel, float level) {
		if (channel < 1 || channel > 6) return false;
		return inputLevelInteger(channel, inputlevelbyte(level));
	}
private:
	bool volumeInteger(uint32_t n);
	bool volumeInteger(int channel, uint32_t n);
	bool inputLevelInteger(int32_t n);
	bool inputLevelInteger(int chnnel, int32_t n);
	// convert level to volume byte, section 6.9.1, page 50
	uint32_t volumebyte(float level) {
		if (level >= 1.0) return 0;
		if (level <= 0.0000003981) return 128;
		return roundf(log10f(level) * -20.0);
	}
	// convert level to input gain, section 6.11.1, page 51
	int32_t inputlevelbyte(float level) {
		if (level > 15.8489) return 48;
		if (level < 0.00063095734) return -128;
		return roundf(log10f(level) * 40.0);
	}
	bool write(uint32_t address, uint32_t data);
	bool write(uint32_t address, const void *data, uint32_t len);
	uint8_t i2c_addr;
	bool muted;
};

#endif
