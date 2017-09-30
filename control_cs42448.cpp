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

#include "control_cs42448.h"
#include "Wire.h"


#define CS42448_Chip_ID				0x01
#define CS42448_Power_Control			0x02
#define CS42448_Functional_Mode			0x03
#define CS42448_Interface_Formats		0x04
#define CS42448_ADC_Control_DAC_DeEmphasis	0x05
#define CS42448_Transition_Control		0x06
#define CS42448_DAC_Channel_Mute		0x07
#define CS42448_AOUT1_Volume_Control		0x08
#define CS42448_AOUT2_Volume_Control		0x09
#define CS42448_AOUT3_Volume_Control		0x0A
#define CS42448_AOUT4_Volume_Control		0x0B
#define CS42448_AOUT5_Volume_Control		0x0C
#define CS42448_AOUT6_Volume_Control		0x0D
#define CS42448_AOUT7_Volume_Control		0x0E
#define CS42448_AOUT8_Volume_Control		0x0F
#define CS42448_DAC_Channel_Invert		0x10
#define CS42448_AIN1_Volume_Control		0x11
#define CS42448_AIN2_Volume_Control		0x12
#define CS42448_AIN3_Volume_Control		0x13
#define CS42448_AIN4_Volume_Control		0x14
#define CS42448_AIN5_Volume_Control		0x15
#define CS42448_AIN6_Volume_Control		0x16
#define CS42448_ADC_Channel_Invert		0x17
#define CS42448_Status_Control			0x18
#define CS42448_Status				0x19
#define CS42448_Status_Mask			0x1A
#define CS42448_MUTEC_Pin_Control		0x1B

// 4.9 Recommended Power-Up Sequence
//    1. Hold RST low until the power supply and clocks are stable. In this state,
//       the control port is reset to its default settings and VQ will remain low.
//    2. Bring RST high. The device will initially be in a low power state with VQ
//       low. All features will default as described in the "Register Quick Reference"
//       on page 40.
//    3. Perform a write operation to the Power Control register ("Power Control
//       (Address 02h)" on page 43) to set bit 0 to a '1'b.  This will place the
//       device in a power down state.
//    4. Load the desired register settings while keeping the PDN bit set to '1'b.
//    5. Mute all DACs. Muting the DACs suppresses any noise associated with the
//       CODEC's first initialization after power is applied.
//    6. Set the PDN bit in the power control register to '0'b. VQ will ramp to
//       approximately VA/2 according to the Popguard specification in section
//       "Popguard" on page 29.
//    7. Following approximately 2000 LRCK cycles, the device is initialized and
//       ready for normal operation.
//    8. After the CODEC is initialized, wait ~90 LRCK cycles (~1.9 ms @48 kHz) and
//       then un-mute the DACs.
//    9. Normal operation begins.

static const uint8_t default_config[] = {
	0xF4, // CS42448_Functional_Mode = slave mode, MCLK 25.6 MHz max
	0x76, // CS42448_Interface_Formats = TDM mode
	0x1C, // CS42448_ADC_Control_DAC_DeEmphasis = single ended ADC
	0x63, // CS42448_Transition_Control = soft vol control
	0xFF  // CS42448_DAC_Channel_Mute = all outputs mute
};

bool AudioControlCS42448::enable(void)
{
	Wire.begin();
	// TODO: wait for reset signal high??
	if (!write(CS42448_Power_Control, 0xFF)) return false; // power down
	if (!write(CS42448_Functional_Mode, default_config, sizeof(default_config))) return false;
	if (!write(CS42448_Power_Control, 0)) return false; // power up
	return true;
}

bool AudioControlCS42448::volumeInteger(uint32_t n)
{
	uint8_t data[9];
	data[0] = 0;
	for (int i=1; i < 9; i++) {
		data[i] = n;
	}
	return write(CS42448_DAC_Channel_Mute, data, 9);
}

bool AudioControlCS42448::volumeInteger(int channel, uint32_t n)
{

	return true;
}

bool AudioControlCS42448::inputLevelInteger(int32_t n)
{

	return true;
}

bool AudioControlCS42448::inputLevelInteger(int chnnel, int32_t n)
{

	return true;
}

bool AudioControlCS42448::write(uint32_t address, uint32_t data)
{
	Wire.beginTransmission(i2c_addr);
	Wire.write(address);
	Wire.write(data);
	if (Wire.endTransmission() == 0) return true;
	return false;
}

bool AudioControlCS42448::write(uint32_t address, const void *data, uint32_t len)
{
	Wire.beginTransmission(i2c_addr);
	Wire.write(address | 0x80);
	const uint8_t *p = (const uint8_t *)data;
	const uint8_t *end = p + len;
	while (p < end) {
		Wire.write(*p++);
	}
	if (Wire.endTransmission() == 0) return true;
	return false;
}


