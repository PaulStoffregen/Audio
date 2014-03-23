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

#ifndef control_sgtl5000_h_
#define control_sgtl5000_h_

#include "AudioControl.h"

#define SGTL_ADC 0
#define SGTL_I2S_TEENSY 1
#define SGTL_AUDIO_PROCESSOR 3

class AudioControlSGTL5000 : public AudioControl
{
public:
	bool enable(void);
	bool disable(void) { return false; }
	bool volume(float n) { return volumeInteger(n * 129 + 0.499); }
	bool inputLevel(float n) {return false;}
	bool muteHeadphone(void) { return write(0x0024, ana_ctrl | (1<<4)); }
	bool unmuteHeadphone(void) { return write(0x0024, ana_ctrl & ~(1<<4)); }
	bool muteLineout(void) { return write(0x0024, ana_ctrl | (1<<8)); }
	bool unmuteLineout(void) { return write(0x0024, ana_ctrl & ~(1<<8)); }
	bool inputSelect(int n) {
		if (n == AUDIO_INPUT_LINEIN) {
			return write(0x0024, ana_ctrl | (1<<2));
		} else if (n == AUDIO_INPUT_MIC) {
			//return write(0x002A, 0x0172) && write(0x0024, ana_ctrl & ~(1<<2));
			return write(0x002A, 0x0173) && write(0x0024, ana_ctrl & ~(1<<2)); // +40dB
		} else {
			return false;
		}
	}
	//bool inputLinein(void) { return write(0x0024, ana_ctrl | (1<<2)); }
	//bool inputMic(void) { return write(0x002A, 0x0172) && write(0x0024, ana_ctrl & ~(1<<2)); }

	unsigned short route(uint8_t i2s_out, uint8_t dac, uint8_t dap, uint8_t dap_mix);
	unsigned short route(uint8_t i2s_out, uint8_t dac, uint8_t dap) { return route(i2s_out,dac,dap,0); }
	bool volume(float left, float right);
	unsigned short micGain(unsigned int n) { return modify(0x002A, n&3, 3); }
	unsigned short lineOutLevel(uint8_t n);
	unsigned short lineOutLevel(uint8_t left, uint8_t right);
	unsigned short dacVolume(float n);
	unsigned short dacVolume(float left, float right);
	unsigned short adcHighPassFilterControl(uint8_t bypass, uint8_t freeze);
	unsigned short adcHighPassFilterControl(uint8_t bypass);
	unsigned short audioMixerEnable(uint8_t n);
	unsigned short audioProcessorEnable(uint8_t n);
	unsigned short audioProcessorEnable(void);
	unsigned short eqFilterCount(uint8_t n);
	unsigned short eqSelect(uint8_t n);
	unsigned short eqBand(uint8_t bandNum, float n);
	void eqBands(float bass, float mid_bass, float midrange, float mid_treble, float treble);
	void eqBands(float bass, float treble);
	void eqFilter(uint8_t filterNum, int *filterParameters);
	unsigned short autoVolumeControl(uint8_t maxGain, uint8_t lbiResponse, uint8_t hardLimit, float threshold, float attack, float decay);
	unsigned short autoVolumeEnable(uint8_t n);
	unsigned short autoVolumeEnable(void);
	unsigned short enhanceBass(float lr_lev, float bass_lev);
	unsigned short enhanceBass(float lr_lev, float bass_lev, uint8_t hpf_bypass, uint8_t cutoff);
	unsigned short enhanceBassEnable(uint8_t n);
	unsigned short enhanceBassEnable(void);
	unsigned short surroundSound(uint8_t width);
	unsigned short surroundSound(uint8_t width, uint8_t select);
	unsigned short surroundSoundEnable(uint8_t n);
	unsigned short surroundSoundEnable(void);
	void killAutomation(void) { semi_automated=false; }

protected:
	bool muted;
	bool volumeInteger(unsigned int n); // range: 0x00 to 0x80
	uint16_t ana_ctrl;

	unsigned char calcVol(float n, unsigned char range);

	unsigned int read(unsigned int reg);
	bool write(unsigned int reg, unsigned int val);
	unsigned int modify(unsigned int reg, unsigned int val, unsigned int iMask);
	unsigned short dap_audio_eq_band(uint8_t bandNum, float n);
private:
	bool semi_automated;
	void automate(uint8_t dap, uint8_t eq);
	void automate(uint8_t dap, uint8_t eq, uint8_t filterCount);
};

//For Filter Type: 0 = LPF, 1 = HPF, 2 = BPF, 3 = NOTCH, 4 = PeakingEQ, 5 = LowShelf, 6 = HighShelf
  #define FILTER_LOPASS 0
  #define FILTER_HIPASS 1
  #define FILTER_BANDPASS 2
  #define FILTER_NOTCH 3
  #define FILTER_PARAEQ 4
  #define FILTER_LOSHELF 5
  #define FILTER_HISHELF 6

void calcBiquad(uint8_t filtertype, float fC, float dB_Gain, float Q, uint32_t quantization_unit, uint32_t fS, int *coef);

#endif
