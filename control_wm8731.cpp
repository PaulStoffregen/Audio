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

#include "control_wm8731.h"
#include "Wire.h"

#if defined(ARDUINO_ARCH_SAMD)
#include <Arduino.h>
#endif

#define WM8731_I2C_ADDR 0x1A
//#define WM8731_I2C_ADDR 0x1B

#define WM8731_REG_LLINEIN	0
#define WM8731_REG_RLINEIN	1
#define WM8731_REG_LHEADOUT	2
#define WM8731_REG_RHEADOUT	3
#define WM8731_REG_ANALOG	4
#define WM8731_REG_DIGITAL	5
#define WM8731_REG_POWERDOWN	6
#define WM8731_REG_INTERFACE	7
#define WM8731_REG_SAMPLING	8
#define WM8731_REG_ACTIVE	9
#define WM8731_REG_RESET	15

bool AudioControlWM8731::enable(void)
{
	Wire.begin();
	delay(5);
	//write(WM8731_REG_RESET, 0);

	write(WM8731_REG_INTERFACE, 0x02); // I2S, 16 bit, MCLK slave
	write(WM8731_REG_SAMPLING, 0x20);  // 256*Fs, 44.1 kHz, MCLK/1

	// In order to prevent pops, the DAC should first be soft-muted (DACMU),
	// the output should then be de-selected from the line and headphone output
	// (DACSEL), then the DAC powered down (DACPD).

	write(WM8731_REG_DIGITAL, 0x08);   // DAC soft mute
	write(WM8731_REG_ANALOG, 0x00);    // disable all

	write(WM8731_REG_POWERDOWN, 0x00); // codec powerdown

	write(WM8731_REG_LHEADOUT, 0x80);      // volume off
	write(WM8731_REG_RHEADOUT, 0x80);

	delay(100); // how long to power up?

	write(WM8731_REG_ACTIVE, 1);
	delay(5);
	write(WM8731_REG_DIGITAL, 0x00);   // DAC unmuted
	write(WM8731_REG_ANALOG, 0x10);    // DAC selected

	return true;
}


bool AudioControlWM8731::write(unsigned int reg, unsigned int val)
{
	Wire.beginTransmission(WM8731_I2C_ADDR);
	Wire.write((reg << 1) | ((val >> 8) & 1));
	Wire.write(val & 0xFF);
	Wire.endTransmission();
	return true;
}

bool AudioControlWM8731::volumeInteger(unsigned int n)
{
	// n = 127 for max volume (+6 dB)
	// n = 48 for min volume (-73 dB)
	// n = 0 to 47 for mute
	if (n > 127) n = 127;
	 //Serial.print("volumeInteger, n = ");
	 //Serial.println(n);
	write(WM8731_REG_LHEADOUT, n | 0x180);
	write(WM8731_REG_RHEADOUT, n | 0x80);
	return true;
}

bool AudioControlWM8731::inputLevel(float n)
{
	// range is 0x00 (min) - 0x1F (max)

	int _level = int(n * 31.f); 

	_level = _level > 0x1F ? 0x1F : _level;
	write(WM8731_REG_LLINEIN, _level);
	write(WM8731_REG_RLINEIN, _level);
	return true;
}

/******************************************************************/


bool AudioControlWM8731master::enable(void)
{
	Wire.begin();
	delay(5);
	//write(WM8731_REG_RESET, 0);

	write(WM8731_REG_INTERFACE, 0x42); // I2S, 16 bit, MCLK master
	write(WM8731_REG_SAMPLING, 0x20);  // 256*Fs, 44.1 kHz, MCLK/1

	// In order to prevent pops, the DAC should first be soft-muted (DACMU),
	// the output should then be de-selected from the line and headphone output
	// (DACSEL), then the DAC powered down (DACPD).

	write(WM8731_REG_DIGITAL, 0x08);   // DAC soft mute
	write(WM8731_REG_ANALOG, 0x00);    // disable all
	
	write(WM8731_REG_POWERDOWN, 0x00); // codec powerdown

	write(WM8731_REG_LHEADOUT, 0x80);      // volume off
	write(WM8731_REG_RHEADOUT, 0x80);

	delay(100); // how long to power up?

	write(WM8731_REG_ACTIVE, 1);
	delay(5);
	write(WM8731_REG_DIGITAL, 0x00);   // DAC unmuted
	write(WM8731_REG_ANALOG, 0x10);    // DAC selected

	return true;
}



