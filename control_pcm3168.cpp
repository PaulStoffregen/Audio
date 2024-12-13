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

#include <Arduino.h>
#include "control_pcm3168.h"

uint32_t AudioControlPCM3168::reset(int pin)
{
	if (pin > 0) // assume >0 is valid!
	{
		pinMode(pin,OUTPUT);
		digitalWriteFast(pin, 0); // start the reset pulse
		delayMicroseconds(1); 	  // datasheet says 100ns is enough
		digitalWriteFast(pin, 1); // end the reset pulse
		resetTime = micros(); 	  // store when we did it
		
		if (0 == resetTime) // unlikely. but
			resetTime = 1;
	}
	else
		resetTime = 0; // say we didn't do a reset
	
	return resetTime; // non-zero if we did a reset
}

bool AudioControlPCM3168::enable(void)
{
	wire->begin();
	
	// If class function was used to reset, we can check it was done
	// long enough ago. If not, we just have to assume all is OK.
	if (0 != resetTime &&					// we have done a reset, but...
		micros() - resetTime < RESET_WAIT) 	// ...is it too soon to enable?
		delayMicroseconds(RESET_WAIT - (micros() - resetTime)); // yes, wait
		
	return write(DAC_CONTROL_1, DC1_24_BIT_I2S_TDM)
		&& write(ADC_CONTROL_1, AC1_24_BIT_I2S_TDM)
		&& write(DAC_SOFT_MUTE_CONTROL, 0x00)
		&& write(ADC_SOFT_MUTE_CONTROL, 0x00)
		&& write(DAC_CONTROL_3, 0x80) // BEN UPDATED TO SET All channels control
		;
}


bool AudioControlPCM3168::disable(void)
{
	// not really disabled, but it is muted
	return write(DAC_SOFT_MUTE_CONTROL, 0x00)
		&& write(ADC_SOFT_MUTE_CONTROL, 0x00)
		;
}

bool AudioControlPCM3168::volumeInteger(uint32_t n)
{
	return write(DAC_ATTENUATION_ALL, n);
}


bool AudioControlPCM3168::volumeInteger(int channel, uint32_t n)
{
	bool rv = false;
	
	channel--; // API is 1-8, we want 0-7
	if (channel >= 0 && channel < DAC_CHANNELS)
		rv = write(DAC_ATTENUATION_BASE + channel, n);

	return rv;
}


bool AudioControlPCM3168::inputLevelInteger(int32_t n)
{
	uint8_t levels[ADC_CHANNELS];
	
	for (int i=0; i < ADC_CHANNELS;i++)
		levels[i] = n;
	
	return write(ADC_ATTENUATION_BASE, levels, ADC_CHANNELS);
}


bool AudioControlPCM3168::inputLevelInteger(int channel, int32_t n)
{
	bool rv = false;
	
	channel--; // API is 1-6, we want 0-5
	if (channel >= 0 && channel < ADC_CHANNELS)
		rv = write(ADC_ATTENUATION_BASE + channel, n);

	return rv;
}


bool AudioControlPCM3168::invertDAC(uint32_t data)
{
	return write(DAC_OUTPUT_PHASE, data); // these bits will invert the signal polarity of their respective DAC channels (1-8)
}


bool AudioControlPCM3168::invertADC(uint32_t data)
{
	return write(ADC_INPUT_PHASE, data); // these bits will invert the signal polarity of their respective ADC channels (1-6)
}


bool AudioControlPCM3168::write(uint32_t address, uint32_t data)
{
	wire->beginTransmission(i2c_addr);
	wire->write(address);
	wire->write(data);
	return wire->endTransmission() == 0;
}


bool AudioControlPCM3168::write(uint32_t address, const void *data, uint32_t len)
{
	wire->beginTransmission(i2c_addr);
	wire->write(address);
	const uint8_t *p = (const uint8_t *)data;
	wire->write(p,len);
	
	return wire->endTransmission() == 0;
}


void AudioControlPCM3168::setSingleEndedMode() {
	write(0x53, 0xFF);
}