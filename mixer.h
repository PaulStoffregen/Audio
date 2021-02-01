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
 
 // mixer.tpp (the implementaion) is included at the end of this file

#ifndef mixer_h_
#define mixer_h_

#include "Arduino.h"
#include "AudioStream.h"

#define AudioMixer4 AudioMixer<4>

#if defined(__ARM_ARCH_7EM__)

#define MULTI_UNITYGAIN 65536
#define MULTI_UNITYGAIN_F 65536.0f
#define MAX_GAIN 32767.0f
#define MIN_GAIN -32767.0f
#define MULT_DATA_TYPE int32_t

#elif defined(KINETISL)

#define MULTI_UNITYGAIN 256
#define MULTI_UNITYGAIN_F 256.0f
#define MAX_GAIN 127.0f
#define MIN_GAIN -127.0f
#define MULT_DATA_TYPE int16_t

#endif

template <int NN> class AudioMixer : public AudioStream
{
public:
	AudioMixer(void) : AudioStream(NN, inputQueueArray) {
		for (int i=0; i<NN; i++) multiplier[i] = MULTI_UNITYGAIN;
	}	
	void update();
	/**
	 * this sets the individual gains
	 * @param channel
	 * @param gain
	 */
	void gain(unsigned int channel, float gain);
	/**
	 * set all channels to specified gain
	 * @param gain
	 */
	void gain(float gain);

private:
	MULT_DATA_TYPE multiplier[NN];
	audio_block_t *inputQueueArray[NN];
};

class AudioAmplifier : public AudioStream
{
public:
	AudioAmplifier(void) : AudioStream(1, inputQueueArray) {
		multiplier = MULTI_UNITYGAIN;
	}
	void update();
	/**
	 * sets the gain for the amplifier 
	 * if 0 it will transmit nothing
	 * if 1 it will transmit without any change
	 * @param gain
	 */
	void gain(float gain);

private:
	MULT_DATA_TYPE multiplier;
	audio_block_t *inputQueueArray[1];
};
#include "mixer.tpp"

#endif