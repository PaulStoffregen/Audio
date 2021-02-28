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

#define LINEAR 0
#define FIR_POLY 1
#include "Arduino.h"
#include "AudioStream.h"
#include "arm_math.h"
#define LINEAR 0
#define FIR_POLY 1
#define INTERPOLATION  4
#define INTERPOLATIONscaler  4
#define I_SAMPLERATE  AUDIO_SAMPLE_RATE_EXACT * INTERPOLATION
#define NUM_SAMPLES  128
#define I_NUM_SAMPLES  NUM_SAMPLES * INTERPOLATION

/*
#define interpolation_taps 32
static float interpolation_coeffs[interpolation_taps] = {
444.8708144065367380E-6, 0.002132195798620151, 0.004238318820060649, 0.005517336264360800, 0.004277228394758379,-780.0288538232418890E-6,-0.009468419595216885,-0.019306296307004593,
-0.025555801376824804,-0.022415559173490765,-0.005152700802329096, 0.027575475722578378, 0.072230543044204509, 0.120566259375849666, 0.161676773663769008, 0.185322994251944373,
 0.185322994251944373, 0.161676773663769008, 0.120566259375849666, 0.072230543044204509, 0.027575475722578378,-0.005152700802329096,-0.022415559173490765,-0.025555801376824804,
-0.019306296307004593,-0.009468419595216885,-780.0288538232418890E-6, 0.004277228394758379, 0.005517336264360800, 0.004238318820060649, 0.002132195798620151, 444.8708144065367380E-6
};
*/

#define interpolation_taps 36
static float interpolation_coeffs[interpolation_taps] = {
-14.30851541590154240E-6,  0.001348560352009071, 0.004029285548698377, 0.007644563345368599, 0.010936856250494802, 0.011982063548666887, 0.008882946305001046, 826.6598116471556070E-6,
-0.011008071930708746,-0.023014151355548934,-0.029736402750934567,-0.025405787911977455,-0.006012006772274640, 0.028729626071574525, 0.074466890595619062, 0.122757573409695370,
 0.163145421379242955, 0.186152844567746417, 0.186152844567746417, 0.163145421379242955, 0.122757573409695370, 0.074466890595619062, 0.028729626071574525,-0.006012006772274640,
-0.025405787911977455,-0.029736402750934567,-0.023014151355548934,-0.011008071930708746, 826.6598116471556070E-6, 0.008882946305001046, 0.011982063548666887, 0.010936856250494802,
 0.007644563345368599, 0.004029285548698377, 0.001348560352009071,-14.30851541590154240E-6
};

class AudioFilterLadder: public AudioStream
{
public:
	AudioFilterLadder() : AudioStream(3, inputQueueArray) {};
	void frequency(float FC);
	void resonance(float reson);
	void octaveControl(float octaves);
	void passband_gain(float passbandgain);
	void input_drive(float drv);
	void interpMethod(int im);
	void initpoly();
	virtual void update(void);
private:
	float interpolation_state[(NUM_SAMPLES-1) + interpolation_taps / INTERPOLATION];
	arm_fir_interpolate_instance_f32 interpolation;
	float decimation_state[(I_NUM_SAMPLES-1) + interpolation_taps];
	arm_fir_decimate_instance_f32 decimation;
	float LPF(float s, int i);
	void compute_coeffs(float fc);
	bool resonating();
	bool  firstpoly=true;
	bool  polyOn = true;		// linear default
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
