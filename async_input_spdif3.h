/* Audio Library for Teensy 3.X
 * Copyright (c) 2019, Paul Stoffregen, paul@pjrc.com
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
/*
 by Alexander Walch
 */
#ifndef async_input_spdif3_h_
#define async_input_spdif3_h_
#include "Resampler.h"
#include "Quantizer.h"
#include <Arduino.h>     // github.com/PaulStoffregen/cores/blob/master/teensy4/Arduino.h
#include <AudioStream.h> // github.com/PaulStoffregen/cores/blob/master/teensy4/AudioStream.h
#include <DMAChannel.h>  // github.com/PaulStoffregen/cores/blob/master/teensy4/DMAChannel.h
#include <arm_math.h>    // github.com/PaulStoffregen/cores/blob/master/teensy4/arm_math.h

//#define DEBUG_SPDIF_IN	//activates debug output

class AsyncAudioInputSPDIF3 : public AudioStream
{
public:
	///@param attenuation target attenuation [dB] of the anti-aliasing filter. Only used if AUDIO_SAMPLE_RATE_EXACT < input sample rate (input fs). The attenuation can't be reached if the needed filter length exceeds 2*MAX_FILTER_SAMPLES+1
	///@param minHalfFilterLength If AUDIO_SAMPLE_RATE_EXACT >= input fs), the filter length of the resampling filter is 2*minHalfFilterLength+1. If AUDIO_SAMPLE_RATE_EXACT < input fs the filter is maybe longer to reach the desired attenuation
	///@param maxHalfFilterLength Can be used to restrict the maximum filter length at the cost of a lower attenuation
	AsyncAudioInputSPDIF3(bool dither=false, bool noiseshaping=false,float attenuation=100, int32_t minHalfFilterLength=20, int32_t maxHalfFilterLength=80);
	~AsyncAudioInputSPDIF3();
	void begin();
	virtual void update(void);
	double getBufferedTime() const;
	double getInputFrequency() const;
	static bool isLocked();
	double getTargetLantency() const;
	double getAttenuation() const;
	int32_t getHalfFilterLength() const;
protected:	
	static DMAChannel dma;
	static void isr(void);
private:
	void resample(int16_t* data_left, int16_t* data_right, int32_t& block_offset);
	void monitorResampleBuffer();
	void configure();
	double getNewValidInputFrequ();
	void config_spdifIn();

	//accessed in isr ====
	static volatile int32_t buffer_offset;
	static int32_t resample_offset;
    static volatile uint32_t microsLast;
	//====================

	Resampler _resampler;
	Quantizer* quantizer[2];
	arm_biquad_cascade_df2T_instance_f32 _bufferLPFilter;
	
	volatile double _bufferedTime;
	volatile double _lastValidInputFrequ;
	double _inputFrequency=0.;
	double _targetLatencyS;	//target latency [seconds]
	const double _blockDuration=AUDIO_BLOCK_SAMPLES/AUDIO_SAMPLE_RATE_EXACT; //[seconds] 
	double _maxLatency=2.*_blockDuration;

#ifdef DEBUG_SPDIF_IN
	static volatile bool bufferOverflow;
#endif
};

#endif
