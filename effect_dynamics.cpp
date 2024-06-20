/* Audio Library for Teensy 3.X
 * Dynamics Processor (Gate, Compressor & Limiter)
 * Copyright (c) 2017, Marc Paquette (marc@dacsystemes.com)
 * Based on analyse_rms & mixer objects by Paul Stoffregen
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
#if !defined(KINETISL)

#include "effect_dynamics.h"
#include "utility/dspinst.h"
#include "utility/sqrt_integer.h"

static float analyse_rms(int16_t *data) {
	
	uint32_t *p = (uint32_t *)data;
	const uint32_t *end = p + AUDIO_BLOCK_SAMPLES / 2;
	int64_t sum = 0;
	do {
		uint32_t n1 = *p++;
		uint32_t n2 = *p++;
		uint32_t n3 = *p++;
		uint32_t n4 = *p++;
		sum = multiply_accumulate_16tx16t_add_16bx16b(sum, n1, n1);
		sum = multiply_accumulate_16tx16t_add_16bx16b(sum, n2, n2);
		sum = multiply_accumulate_16tx16t_add_16bx16b(sum, n3, n3);
		sum = multiply_accumulate_16tx16t_add_16bx16b(sum, n4, n4);
		
	} while (p < end);
	if (sum == 0) return 0;
	int32_t meansq = sum / AUDIO_BLOCK_SAMPLES;
	return sqrt_uint32(meansq) / 32767.0f;
}

static void applyGain(int16_t *data, int32_t mult1, int32_t mult2) {
	
	uint32_t *p = (uint32_t *)data;
	const uint32_t *end = p + AUDIO_BLOCK_SAMPLES / 2;
	int32_t inc = (mult2 - mult1) / (AUDIO_BLOCK_SAMPLES / 2);
	
	do {
		uint32_t tmp32 = *p; // read 2 samples from *data
		int32_t val1 = signed_multiply_32x16b(mult1, tmp32);
		mult1 += inc;
		int32_t val2 = signed_multiply_32x16t(mult1, tmp32);
		mult1 += inc;
		val1 = signed_saturate_rshift(val1, 16, 0);
		val2 = signed_saturate_rshift(val2, 16, 0);
		*p++ = pack_16b_16b(val2, val1);
	} while (p < end);
}

/* ----------------------------------------------------------------------
* https://community.arm.com/tools/f/discussions/4292/cmsis-dsp-new-functionality-proposal/22621#22621
* Fast approximation to the log2() function.  It uses a two step
* process.  First, it decomposes the floating-point number into
* a fractional component F and an exponent E.  The fraction component
* is used in a polynomial approximation and then the exponent added
* to the result.  A 3rd order polynomial is used and the result
* when computing db20() is accurate to 7.984884e-003 dB.
** ------------------------------------------------------------------- */

float log2f_approx_coeff[4] = {1.23149591368684f, -4.11852516267426f, 6.02197014179219f, -3.13396450166353f};

float log2f_approx(float X)
{
  float *C = &log2f_approx_coeff[0];
  float Y;
  float F;
  int E;

  // This is the approximation to log2()
  F = frexpf(fabsf(X), &E);

  //  Y = C[0]*F*F*F + C[1]*F*F + C[2]*F + C[3] + E;
  Y = *C++;
  Y *= F;
  Y += (*C++);
  Y *= F;
  Y += (*C++);
  Y *= F;
  Y += (*C++);
  Y += E;
  return(Y);
}

// https://codingforspeed.com/using-faster-exponential-approximation/
inline float expf_approx(float x) {
  x = 1.0f + x / 1024;
  x *= x; x *= x; x *= x; x *= x;
  x *= x; x *= x; x *= x; x *= x;
  x *= x; x *= x;
  return x;
}

inline float unitToDb(float unit) {
	return 6.02f * log2f_approx(unit);
}

inline float dbToUnit(float db) {
	return expf_approx(db * 2.302585092994046f * 0.05f);
}

void AudioEffectDynamics::update(void) {

	audio_block_t *block;
	
	block = receiveWritable(0);

	if (!block) return;
	
	if (!gateEnabled && !compEnabled && !limiterEnabled) {
		
		//Transmit & release
		transmit(block);
		release(block);
		return;
	}

	for (int i=0; i<AUDIO_BLOCK_SAMPLES; i++) {

        unsigned int sampleIndexPlus1 = (sampleIndex + 1) % sampleBufferSize;

        uint32_t sampleToRemove = samplesSquared[sampleIndexPlus1];
        sumOfSamplesSquared -= (sampleToRemove * sampleToRemove);

        int16_t sample = block->data[i];
        samplesSquared[sampleIndex] = abs(sample);
        uint32_t sampleSquared = sample * sample;
        sumOfSamplesSquared += sampleSquared;

        sampleIndex = (sampleIndex + 1) % sampleBufferSize;

        float rms = sqrt(sumOfSamplesSquared / float(sampleBufferSize)) / 32768.0;

        //Compute block RMS level in Db
        float inputdb = MIN_DB;
        if (rms > 0) inputdb = unitToDb(rms);

        //Gate
        if (gateEnabled) {
            if (inputdb >= gateThresholdOpen) gatedb = (aGateAttack * gatedb) + (aOneMinusGateAttack * MAX_DB);
            else if (inputdb < gateThresholdClose) gatedb = (aGateRelease * gatedb) + (aOneMinusGateRelease * MIN_DB);
        } else gatedb = MAX_DB;

        //Compressor
        if (compEnabled) {
            float attdb = MAX_DB; //Below knee
            if (inputdb >= aLowKnee) {
                if (inputdb <= aHighKnee) {
                    //Knee transition
                    float knee = inputdb - aLowKnee;
                    attdb = aKneeRatio * knee * knee * aTwoKneeWidth;
                } else {
                    //Above knee
                    attdb = compThreshold + ((inputdb - compThreshold) * compRatio) - inputdb;
                }
            }
            if (attdb <= compdb) compdb = (aCompAttack * compdb) + (aOneMinusCompAttack * attdb);
            else compdb = (aCompRelease * compdb) + (aOneMinusCompRelease * attdb);
        } else compdb = MAX_DB;

        //Brickwall Limiter
        if (limiterEnabled) {
            float outdb = inputdb + compdb + makeupdb;
            if (outdb >= limitThreshold) limitdb = (aLimitAttack * limitdb) +
                                                   (aOneMinusLimitAttack * (limitThreshold - outdb));
            else limitdb *= aLimitRelease;
        } else limitdb = MAX_DB;

        //Compute linear gain
        float totalGain = gatedb + compdb + makeupdb + limitdb;

        float multiplier = dbToUnit(totalGain);
        int16_t result = sample * multiplier;
        block->data[i] = result;
        //Apply gain to block
    }

	//Transmit & release
	transmit(block);
	release(block);
}

#endif