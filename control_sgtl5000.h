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

class AudioControlSGTL5000 : public AudioControl
{
public:
	AudioControlSGTL5000(void) : i2c_addr(0x0A) { }
	void setAddress(uint8_t level);
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
			return write(0x0020, 0x055) // +7.5dB gain (1.3Vp-p full scale)
			 && write(0x0024, ana_ctrl | (1<<2)); // enable linein
		} else if (n == AUDIO_INPUT_MIC) {
			return write(0x002A, 0x0173) // mic preamp gain = +40dB
			 && write(0x0020, 0x088)     // input gain +12dB (is this enough?)
			 && write(0x0024, ana_ctrl & ~(1<<2)); // enable mic
		} else {
			return false;
		}
	}
	bool volume(float left, float right);
	bool micGain(unsigned int dB);
	bool lineInLevel(uint8_t n) { return lineInLevel(n, n); }
	bool lineInLevel(uint8_t left, uint8_t right);
	unsigned short lineOutLevel(uint8_t n);
	unsigned short lineOutLevel(uint8_t left, uint8_t right);
	unsigned short dacVolume(float n);
	unsigned short dacVolume(float left, float right);
	bool dacVolumeRamp();
	bool dacVolumeRampLinear();
	bool dacVolumeRampDisable();
	unsigned short adcHighPassFilterEnable(void);
	unsigned short adcHighPassFilterFreeze(void);
	unsigned short adcHighPassFilterDisable(void);
	unsigned short audioPreProcessorEnable(void);
	unsigned short audioPostProcessorEnable(void);
	unsigned short audioProcessorDisable(void);
	unsigned short eqFilterCount(uint8_t n);
	unsigned short eqSelect(uint8_t n);
	unsigned short eqBand(uint8_t bandNum, float n);
	void eqBands(float bass, float mid_bass, float midrange, float mid_treble, float treble);
	void eqBands(float bass, float treble);
	void eqFilter(uint8_t filterNum, int *filterParameters);
	unsigned short autoVolumeControl(uint8_t maxGain, uint8_t lbiResponse, uint8_t hardLimit, float threshold, float attack, float decay);
	unsigned short autoVolumeEnable(void);
	unsigned short autoVolumeDisable(void);
	unsigned short enhanceBass(float lr_lev, float bass_lev);
	unsigned short enhanceBass(float lr_lev, float bass_lev, uint8_t hpf_bypass, uint8_t cutoff);
	unsigned short enhanceBassEnable(void);
	unsigned short enhanceBassDisable(void);
	unsigned short surroundSound(uint8_t width);
	unsigned short surroundSound(uint8_t width, uint8_t select);
	unsigned short surroundSoundEnable(void);
	unsigned short surroundSoundDisable(void);
	void killAutomation(void) { semi_automated=false; }

protected:
	bool muted;
	bool volumeInteger(unsigned int n); // range: 0x00 to 0x80
	uint16_t ana_ctrl;
	uint8_t i2c_addr;
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
  
//For frequency adjustment
  #define FLAT_FREQUENCY 0
  #define PARAMETRIC_EQUALIZER 1
  #define TONE_CONTROLS 2
  #define GRAPHIC_EQUALIZER 3


void calcBiquad(uint8_t filtertype, float fC, float dB_Gain, float Q, uint32_t quantization_unit, uint32_t fS, int *coef);

#endif
