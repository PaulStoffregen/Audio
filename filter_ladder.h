/* Audio Library for Teensy, Ladder Filter
 * Copyright (c) 2021, Richard van Hoesel
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

//-----------------------------------------------------------
// Huovilainen New Moog (HNM) model as per CMJ jun 2006
// Implemented as Teensy Audio Library compatible object
// Richard van Hoesel, v. 1.03, Feb. 14 2021
// v1.5 adds polyphase FIR or Linear interpolation
// v1.4 FC extended to 18.7kHz, max res to 1.8, 4x oversampling,
//      and a minor Q-tuning adjustment
// v.1.03 adds oversampling, extended resonance,
// and exposes parameters input_drive and passband_gain
// v.1.02 now includes both cutoff and resonance "CV" modulation inputs
// please retain this header if you use this code.
//-----------------------------------------------------------

// https://forum.pjrc.com/threads/60488?p=269609&viewfull=1#post269609

#ifndef filter_ladder_h_
#define filter_ladder_h_

#include <Arduino.h>     // github.com/PaulStoffregen/cores/blob/master/teensy4/Arduino.h
#include <AudioStream.h> // github.com/PaulStoffregen/cores/blob/master/teensy4/AudioStream.h
#include <arm_math.h>    // github.com/PaulStoffregen/cores/blob/master/teensy4/arm_math.h

enum AudioFilterLadderInterpolation {
	LADDER_FILTER_INTERPOLATION_LINEAR,
	LADDER_FILTER_INTERPOLATION_FIR_POLY
};


class AudioFilterLadder: public AudioStream
{
public:
	AudioFilterLadder() : AudioStream(3, inputQueueArray) { initpoly(); };
	void frequency(float FC);
	void resonance(float reson);
	void octaveControl(float octaves);
	void passbandGain(float passbandgain);
	void inputDrive(float drv);
	void interpolationMethod(AudioFilterLadderInterpolation im);
	virtual void update(void);
private:
	static const int INTERPOLATION = 4;
	static const int interpolation_taps = 36;
	float interpolation_state[(AUDIO_BLOCK_SAMPLES-1) + interpolation_taps / INTERPOLATION];
	arm_fir_interpolate_instance_f32 interpolation;
	float decimation_state[(AUDIO_BLOCK_SAMPLES*INTERPOLATION-1) + interpolation_taps];
	arm_fir_decimate_instance_f32 decimation;
	static float interpolation_coeffs[interpolation_taps];
	float LPF(float s, int i);
	void compute_coeffs(float fc);
	void initpoly();
	bool resonating();
	bool  polyCapable = false;
	bool  polyOn = false;		// FIR is default after initpoly()
	float alpha = 1.0;
	float beta[4] = {0.0, 0.0, 0.0, 0.0};
	float z0[4] = {0.0, 0.0, 0.0, 0.0};
	float z1[4] = {0.0, 0.0, 0.0, 0.0};
	float K = 1.0f;
	float Fbase = 1000;
	float Qadjust = 1.0f;
	float octaveScale = 1.0f/32768.0f;
	float pbg = 0.5f;
	float overdrive = 0.5f;
	float host_overdrive = 1.0f;
	float oldinput = 0;
	audio_block_t *inputQueueArray[3];
};

#endif
