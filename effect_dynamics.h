/* Audio Library for Teensy 3.X
 * Dynamics Processor (Gate, Compressor & Limiter)
 * Copyright (c) 2018, Marc Paquette (marc@dacsystemes.com)
 * Based on analyse_rms, effect_envelope & mixer objects by Paul Stoffregen
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

#ifndef effect_dynamics_h_
#define effect_dynamics_h_

#if !defined(KINETISL)

#include "Arduino.h"
#include "AudioStream.h"

#define MIN_DB	-110.0f
#define MAX_DB	0.0f

#define MIN_T	0.03f  //Roughly 1 block
#define MAX_T	4.00f

#define RATIO_OFF		1.0f
#define RATIO_INFINITY	60.0f

class AudioEffectDynamics : public AudioStream
{
public:
	AudioEffectDynamics(void) : AudioStream(1, inputQueueArray) {
		
		gate();
		compression();
		limit();
		autoMakeupGain();
		
		gatedb = MIN_DB;
		compdb = MIN_DB;
		limitdb = MIN_DB;
	}

	//Sets the gate parameters.
	//threshold is in dbFS
	//attack & release are in seconds
	void gate(float threshold = -50.0f, float attack = MIN_T, float release = 0.3f, float hysterisis = 6.0f) {
		
		gateEnabled = threshold > MIN_DB;
		
		gateThresholdOpen = constrain(threshold, MIN_DB, MAX_DB);
		gateThresholdClose = gateThresholdOpen - constrain(hysterisis, 0.0f, 6.0f);
		
		float gateAttackTime = constrain(attack, MIN_T, MAX_T);
		float gateReleaseTime = constrain(release, MIN_T, MAX_T);
		
		aGateAttack = timeToAlpha(gateAttackTime);
		aOneMinusGateAttack = 1.0f - aGateAttack;
		aGateRelease = timeToAlpha(gateReleaseTime);
		aOneMinusGateRelease = 1.0f - aGateRelease;
	}

	//Sets the compression parameters.
	//threshold & kneeWidth are in db(FS)
	//attack and release are in seconds
	//ratio is expressed as x:1 i.e. 1 for no compression, 60 for brickwall limiting
	//Set kneeWidth to 0 for hard knee
	void compression(float threshold = -40.0f, float attack = MIN_T, float release = 0.5f, float ratio = 35.0f, float kneeWidth = 6.0f) {
		
		compEnabled = threshold < MAX_DB;
		
		compThreshold = constrain(threshold, MIN_DB, MAX_DB);
		float compAttackTime = constrain(attack, MIN_T, MAX_T);
		float compReleaseTime = constrain(release, MIN_T, MAX_T);
		compRatio = 1.0f / constrain(abs(ratio), RATIO_OFF, RATIO_INFINITY);
		float compKneeWidth = constrain(abs(kneeWidth), 0.0f, 32.0f);
		computeMakeupGain();
		
		aCompAttack = timeToAlpha(compAttackTime);
		aOneMinusCompAttack = 1.0f - aCompAttack;
		aCompRelease = timeToAlpha(compReleaseTime);
		aOneMinusCompRelease = 1.0f - aCompRelease;
		aHalfKneeWidth = compKneeWidth / 2.0f;
		aTwoKneeWidth = 1.0f / (compKneeWidth * 2.0f);
		aKneeRatio = compRatio - 1.0f;
		aLowKnee = compThreshold - aHalfKneeWidth;
		aHighKnee = compThreshold + aHalfKneeWidth;
	}

	//Sets the hard limiter parameters
	//threshold is in dbFS
	//attack & release are in seconds
	void limit(float threshold = -3.0f, float attack = MIN_T, float release = MIN_T) {
		
		limiterEnabled = threshold < MAX_DB;
		
		limitThreshold = constrain(threshold, MIN_DB, MAX_DB);
		float limitAttackTime = constrain(attack, MIN_T, MAX_T);
		float limitReleaseTime = constrain(release, MIN_T, MAX_T); 

		computeMakeupGain();
		
		aLimitAttack = timeToAlpha(limitAttackTime);
		aOneMinusLimitAttack = 1.0f - aLimitAttack;
		aLimitRelease = timeToAlpha(limitReleaseTime);
	}
	
	//Enables automatic makeup gain setting
	//headroom is in dbFS
	void autoMakeupGain(float headroom = 6.0f) {
		
		mgAutoEnabled = true;
		mgHeadroom = constrain(headroom, 0.0f, 60.0f);
		computeMakeupGain();
	}
	
	//Sets a fixed makeup gain value.
	//gain is in dbFS
	void makeupGain(float gain = 0.0f) {
		
		mgAutoEnabled = false;
		makeupdb = constrain(gain, -12.0f, 24.0f);
	}
	
private:
	audio_block_t *inputQueueArray[1];

	bool gateEnabled = false;
	float gateThresholdOpen;
	float gateThresholdClose;
	float gatedb;

	bool compEnabled = false;
	float compThreshold;
	float compRatio;
	float compdb;

	bool limiterEnabled = false;
	float limitThreshold;
	float limitdb;

	bool mgAutoEnabled;
	float mgHeadroom;
	float makeupdb;
	
	float aGateAttack;
	float aOneMinusGateAttack;
	float aGateRelease;
	float aOneMinusGateRelease;
	float aHalfKneeWidth;
	float aTwoKneeWidth;
	float aKneeRatio;
	float aLowKnee;
	float aHighKnee;
	float aCompAttack;
	float aOneMinusCompAttack;
	float aCompRelease;
	float aOneMinusCompRelease;
	float aLimitAttack;
	float aOneMinusLimitAttack;
	float aLimitRelease;
	const static unsigned int sampleBufferSize = AUDIO_SAMPLE_RATE / 10; // number of samples to use for running RMS calulation = 1/10th of a second 
	u_int64_t sumOfSamplesSquared = 0;
	uint32_t samplesSquared[sampleBufferSize] = {0};
	uint16_t sampleIndex = 0;

	void computeMakeupGain() {
		if (mgAutoEnabled) {
			makeupdb = -compThreshold + (compThreshold * compRatio) + limitThreshold - mgHeadroom;
		}
	}
	
	//Computes smoothing time constants for a 10% to 90% change
	float timeToAlpha(float time) {
		return expf(-0.9542f / (((float)AUDIO_SAMPLE_RATE_EXACT / (float)AUDIO_BLOCK_SAMPLES) * time));
	}
	
	virtual void update(void);
};
#endif

#endif
