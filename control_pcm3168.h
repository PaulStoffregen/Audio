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

#ifndef control_pcm3168_h_
#define control_pcm3168_h_

#include "Wire.h"
#include "AudioControl.h"
#include <math.h>


/*
From SBAS452A –SEPTEMBER 2008–REVISED JANUARY 2016

	9.5 Register Maps
	Table 12. Register Map
	ADDRESS  DATA
	DAC HEX   B7     B6     B5     B4     B3     B2     B1     B0
	64  40  MRST   SRST     —      —      —      —    SRDA1  SRDA0
	65  41  PSMDA  MSDA2  MSDA1  MSDA0  FMTDA3 FMTDA2 FMTDA1 FMTDA0
	66  42  OPEDA3 OPEDA2 OPEDA1 OPEDA0 FLT3   FLT2   FLT1   FLT0
	67  43  REVDA8 REVDA7 REVDA6 REVDA5 REVDA4 REVDA3 REVDA2 REVDA1
	68  44  MUTDA8 MUTDA7 MUTDA6 MUTDA5 MUTDA4 MUTDA3 MUTDA2 MUTDA1
	69  45  ZERO8  ZERO7  ZERO6  ZERO5  ZERO4  ZERO3  ZERO2  ZERO1
	70  46  ATMDDA ATSPDA DEMP1  DEMP0  AZRO2  AZRO1  AZRO0  ZREV
	71  47  ATDA07 ATDA06 ATDA05 ATDA04 ATDA03 ATDA02 ATDA01 ATDA00
	72  48  ATDA17 ATDA16 ATDA15 ATDA14 ATDA13 ATDA12 ATDA11 ATDA10
	73  49  ATDA27 ATDA26 ATDA25 ATDA24 ATDA23 ATDA22 ATDA21 ATDA20
	74  4A  ATDA37 ATDA36 ATDA35 ATDA34 ATDA33 ATDA32 ATDA31 ATDA30
	75  4B  ATDA47 ATDA46 ATDA45 ATDA44 ATDA43 ATDA42 ATDA41 ATDA40
	76  4C  ATDA57 ATDA56 ATDA55 ATDA54 ATDA53 ATDA52 ATDA51 ATDA50
	77  4D  ATDA67 ATDA66 ATDA65 ATDA64 ATDA63 ATDA62 ATDA61 ATDA60
	78  4E  ATDA77 ATDA76 ATDA75 ATDA74 ATDA73 ATDA72 ATDA71 ATDA70
	79  4F  ATDA87 ATDA86 ATDA85 ATDA84 ATDA83 ATDA82 ATDA81 ATDA80
	80  50    —      —      —      —      —      —    SRAD1  SRAD0
	81  51    —    MSAD2  MSAD1  MSAD0    —    FMTAD2 FMTAD1 FMTAD0
	82  52    —    PSVAD2 PSVAD1 PSVAD0   —    BYP2   BYP1   BYP0
	83  53    —      —    SEAD6  SEAD5  SEAD4  SEAD3  SEAD2  SEAD1
	84  54    —      —    REVAD6 REVAD5 REVAD4 REVAD3 REVAD2 REVAD1
	85  55    —      —    MUTAD6 MUTAD5 MUTAD4 MUTAD3 MUTAD2 MUTAD1
	86  56    —      —    OVF6   OVF5   OVF4   OVF3   OVF2   OVF1
	87  57  ATMDAD ATSPAD    —      —      —      —      —   OVFP
	88  58  ATAD07 ATAD06 ATAD05 ATAD04 ATAD03 ATAD02 ATAD01 ATAD00
	89  59  ATAD17 ATAD16 ATAD15 ATAD14 ATAD13 ATAD12 ATAD11 ATAD10
	90  5A  ATAD27 ATAD26 ATAD25 ATAD24 ATAD23 ATAD22 ATAD21 ATAD20
	91  5B  ATAD37 ATAD36 ATAD35 ATAD34 ATAD33 ATAD32 ATAD31 ATAD30
	92  5C  ATAD47 ATAD46 ATAD45 ATAD44 ATAD43 ATAD42 ATAD41 ATAD40
	93  5D  ATAD57 ATAD56 ATAD55 ATAD54 ATAD53 ATAD52 ATAD51 ATAD50
	94  5E  ATAD67 ATAD66 ATAD65 ATAD64 ATAD63 ATAD62 ATAD61 ATAD60
	
	
	Table 13. User-Programmable Mode Control Functions
	FUNCTION                                               RESET DEFAULT      REGISTER  LABEL
	Mode control register reset for ADC and DAC operation  Normal operation      64     MRST
	System reset for ADC and DAC operation                 Normal operation      64     SRST
	DAC sampling mode selection                            Auto                  64     SRDA[1:0]
	DAC power-save mode selection                          Power save            65     PSMDA
	DAC master/slave mode selection                        Slave                 65     MSDA[2:0]
	DAC audio interface format selection                   I2S                   65     FMTDA[3:0]
	DAC operation control                                  Normal operation      66     OPEDA[3:0]
	DAC digital filter roll-off control                    Sharp roll-off        66     FLT[3:0]
	DAC output phase selection                             Normal                67     REVDA[8:1]
	DAC soft mute control                                  Mute disabled         68     MUTDA[8:1]
	DAC zero flag                                          Not detected          69     ZERO[8:1]
	DAC digital attenuation mode                           Channel independent   70     ATMDDA
	DAC digital attenuation speed                          N × 2048/fS           70     ATSPDA
	DAC digital de-emphasis function control               Disabled              70     DEMP[1:0]
	DAC zero flag function selection                       Independent           70     AZRO[2:0]
	DAC zero flag polarity selection                       High for detection    70     ZREV
	DAC digital attenuation level shifting                 0 dB, no attenuation 71–79   ATDAx[7:0]
	ADC sampling mode selection                            Auto                  80     SRAD[1:0]
	ADC master/slave mode selection                        Slave                 81     MSAD[2:0]
	ADC audio interface format selection                   I2S                   81     FMTAD[2:0]
	ADC power-save control                                 Normal operation      82     PSVAD[2:0]
	ADC HPF bypass control                           Normal output, HPF enabled  82     BYP[2:0]
	ADC input configuration control                        Differential          83     SEAD[6:1]
	ADC input phase selection                              Normal                84     REVAD[6:1]
	ADC soft mute control                                  Mute disabled         85     MUTAD[6:1]
	ADC overflow flag                                      Not detected          86     OVF[6:1]
	ADC digital attenuation mode                           Channel independent   87     ATMDAD
	ADC digital attenuation speed                          N × 2048/fS           87     ATSPAD
	ADC overflow flag polarity selection                   High for detection    87     OVFP
	ADC digital attenuation level setting          0 dB, no gain or attenuation  88–94  ATADx[7:0]
*/

class AudioControlPCM3168 : public AudioControl
{
	const uint8_t I2C_BASE = 0x44;
	const int ADC_CHANNELS = 6;
	const int DAC_CHANNELS = 8;
	const uint32_t RESET_WAIT = 200UL;
	
	enum PCM3168reg {RESET_CONTROL = 0x40, DAC_CONTROL_1, DAC_CONTROL_2, 
					 DAC_OUTPUT_PHASE, DAC_SOFT_MUTE_CONTROL, DAC_ZERO_FLAG, 
					 DAC_CONTROL_3, DAC_ATTENUATION_ALL, DAC_ATTENUATION_BASE /* 8 registers */,
					 ADC_SAMPLING_MODE = 0x50, ADC_CONTROL_1, ADC_CONTROL_2,
					 ADC_INPUT_CONFIGURATION, ADC_INPUT_PHASE, ADC_SOFT_MUTE_CONTROL,
					 ADC_OVERFLOW_FLAG, ADC_CONTROL_3, 
					 ADC_ATTENUATION_ALL, ADC_ATTENUATION_BASE /* 6 registers */
	};
	const int DC1_24_BIT_I2S_TDM = 0x06; // DAC_CONTROL_1: set 24-bit I²S mode, TDM format
	const int AC1_24_BIT_I2S_TDM = 0x06; // ADC_CONTROL_1: set 24-bit I²S mode, TDM format
					 
public:
	AudioControlPCM3168(void) 
		: wire(&Wire), i2c_addr(I2C_BASE), muted(true), 
		  resetTime(0)
		{ }
	void setAddress(uint8_t addr) { i2c_addr = I2C_BASE | (addr & 3); }
	void setWire(TwoWire& w) { wire = &w; }
	bool enable(void);
	bool disable(void);
	bool volume(float level) 
	{
		return volumeInteger(volumebyte(level));
	}
	
	bool inputLevel(float level) {
		return inputLevelInteger(inputlevelbyte(level));
	}
	
	bool inputSelect(int n) {
		return (n == 0) ? true : false;
	}
	
	bool volume(int channel, float level) {
		if (channel < 1 || channel > DAC_CHANNELS) return false;
		return volumeInteger(channel, volumebyte(level)); 
	}
	
	bool inputLevel(int channel, float level) {
		if (channel < 1 || channel > DAC_CHANNELS) return false;
		return inputLevelInteger(channel, inputlevelbyte(level));
	}
	
	bool invertDAC(uint32_t data);
	bool invertADC(uint32_t data);
	uint32_t reset(int pin);
	
	void setSingleEndedMode();

private:
	bool volumeInteger(uint32_t n);
	bool volumeInteger(int channel, uint32_t n);
	bool inputLevelInteger(int32_t n);
	bool inputLevelInteger(int chnnel, int32_t n);
	
	// convert level to volume byte, Table 21, page 45
	uint32_t volumebyte(float level) {
		if (level >= 1.0f) return 255;
		if (level <= 1.0e-5f) return 54; // below -100dB, mute
		return 255 + roundf(log10f(level) * 40.0f); // 0.5dB steps: 0dB attenuation is 255
	}
	
	// convert level to input gain, Table 20, page 50
	int32_t inputlevelbyte(float level) {
		if (level > 10.0)    return 255;
		if (level < 1.0e-5f) return  15;
		return 215 + roundf(log10f(level) * 40.0f);
	}
	bool write(uint32_t address, uint32_t data);
	bool write(uint32_t address, const void *data, uint32_t len);
	TwoWire* wire;
	uint8_t i2c_addr;
	bool muted;
	uint32_t resetTime; // when we were reset
};

#endif // control_pcm3168_h_
