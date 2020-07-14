<<<<<<< HEAD
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

#ifndef mixer_h_
#define mixer_h_

#include "Arduino.h"
#include "AudioStream.h"

#define AudioMixer4 AudioMixer<4>

// the following Forward declarations 
// must be defined when we use template 
// the compiler throws some warnings that should be errors otherwise
static inline int32_t signed_multiply_32x16b(int32_t a, uint32_t b); 
static inline int32_t signed_multiply_32x16t(int32_t a, uint32_t b);
static inline int32_t signed_saturate_rshift(int32_t val, int bits, int rshift);
static inline uint32_t pack_16b_16b(int32_t a, int32_t b);
static inline uint32_t signed_add_16_and_16(uint32_t a, uint32_t b);

// because of the template use applyGain and applyGainThenAdd functions
// must be in this file and NOT in cpp file
#if defined(__ARM_ARCH_7EM__)
#define MULTI_UNITYGAIN 65536
#define MULTI_UNITYGAIN_F 65536.0f
#define MAX_GAIN 32767.0f
#define MIN_GAIN -32767.0f
#define MULT_DATA_TYPE int32_t

	static void applyGain(int16_t *data, int32_t mult)
	{
		uint32_t *p = (uint32_t *)data;
		const uint32_t *end = (uint32_t *)(data + AUDIO_BLOCK_SAMPLES);

		do {
			uint32_t tmp32 = *p; // read 2 samples from *data
			int32_t val1 = signed_multiply_32x16b(mult, tmp32);
			int32_t val2 = signed_multiply_32x16t(mult, tmp32);
			val1 = signed_saturate_rshift(val1, 16, 0);
			val2 = signed_saturate_rshift(val2, 16, 0);
			*p++ = pack_16b_16b(val2, val1);
		} while (p < end);
	}

	static void applyGainThenAdd(int16_t *data, const int16_t *in, int32_t mult)
	{
		uint32_t *dst = (uint32_t *)data;
		const uint32_t *src = (uint32_t *)in;
		const uint32_t *end = (uint32_t *)(data + AUDIO_BLOCK_SAMPLES);

		if (mult == MULTI_UNITYGAIN) {
			do {
				uint32_t tmp32 = *dst;
				*dst++ =  signed_add_16_and_16(tmp32, *src++);
				tmp32 = *dst;
				*dst++ =  signed_add_16_and_16(tmp32, *src++);
			} while (dst < end);
		} else {
			do {
				uint32_t tmp32 = *src++; // read 2 samples from *data
				int32_t val1 =  signed_multiply_32x16b(mult, tmp32);
				int32_t val2 =  signed_multiply_32x16t(mult, tmp32);
				val1 =  signed_saturate_rshift(val1, 16, 0);
				val2 =  signed_saturate_rshift(val2, 16, 0);
				tmp32 =  pack_16b_16b(val2, val1);
				uint32_t tmp32b = *dst;
				*dst++ =  signed_add_16_and_16(tmp32, tmp32b);
			} while (dst < end);
		}
	}

#elif defined(KINETISL)
#define MULTI_UNITYGAIN 256
#define MULTI_UNITYGAIN_F 256.0f
#define MAX_GAIN 127.0f
#define MIN_GAIN -127.0f
#define MULT_DATA_TYPE int16_t

	static void applyGain(int16_t *data, int32_t mult)
	{
		const int16_t *end = data + AUDIO_BLOCK_SAMPLES;

		do {
			int32_t val = *data * mult;
			*data++ = signed_saturate_rshift(val, 16, 0);
		} while (data < end);
	}

	static void applyGainThenAdd(int16_t *dst, const int16_t *src, int32_t mult)
	{
		const int16_t *end = dst + AUDIO_BLOCK_SAMPLES;

		if (mult == MULTI_UNITYGAIN) {
			do {
				int32_t val = *dst + *src++;
				*dst++ = signed_saturate_rshift(val, 16, 0);
			} while (dst < end);
		} else {
			do {
				int32_t val = *dst + ((*src++ * mult) >> 8); // overflow possible??
				*dst++ = signed_saturate_rshift(val, 16, 0);
			} while (dst < end);
		}
	}
#endif

template <int NN>
class AudioMixer : public AudioStream
{
public:
	AudioMixer(void) : AudioStream(NN, inputQueueArray) {
		for (int i=0; i<NN; i++) multiplier[i] = MULTI_UNITYGAIN;
	}
	
	void update() {
		audio_block_t *in, *out=NULL;
		unsigned int channel;

		for (channel=0; channel < NN; channel++) {
			if (!out) {
				out = receiveWritable(channel);
				if (out) {
					int32_t mult = multiplier[channel];
					if (mult != MULTI_UNITYGAIN) applyGain(out->data, mult);
				}
			} else {
				in = receiveReadOnly(channel);
				if (in) {
					applyGainThenAdd(out->data, in->data, multiplier[channel]);
					release(in);
				}
			}
		}
		if (out) {
			transmit(out);
			release(out);
		}
	}
	/**
	 * this sets the individual gain, this function is not used on AudioAmplifier
	 * and the gain is not set when used on that.
	 * @param channel
	 * @param gain
	 */
	void gain(unsigned int channel, float gain) {
		if (channel >= NN) return;
		if (gain > MAX_GAIN) gain = MAX_GAIN;
		else if (gain < MIN_GAIN) gain = MIN_GAIN;
		multiplier[channel] = gain * MULTI_UNITYGAIN_F; // TODO: proper roundoff?
	}
	/**
	 * set all multipliers to specified gain
	 */
	void gain(float gain) {
		for (int i = 0; i < NN; i++)
		{
			if (gain > MAX_GAIN) gain = MAX_GAIN;
			else if (gain < MIN_GAIN) gain = MIN_GAIN;
			multiplier[i] = gain * MULTI_UNITYGAIN_F; // TODO: proper roundoff?
		} 
	}

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

	void update() {
		audio_block_t *block;
		int32_t mult = multiplier;

		if (mult == 0) {
			// zero gain, discard any input and transmit nothing
			block = receiveReadOnly(0);
			if (block) release(block);
		} else if (mult == MULTI_UNITYGAIN) {
			// unity gain, pass input to output without any change
			block = receiveReadOnly(0);
			if (block) {
				transmit(block);
				release(block);
			}
		} else {
			// apply gain to signal
			block = receiveWritable(0);
			if (block) {
				applyGain(block->data, mult);
				transmit(block);
				release(block);
			}
		}
		digitalWrite(13, HIGH);
		digitalWrite(13, LOW);
	}
	/**
	 * shorthand for gain(0, value)
	 */
	void gain(float gain) {
		if (gain > MAX_GAIN) gain = MAX_GAIN;
		else if (gain < MIN_GAIN) gain = MIN_GAIN;
		multiplier = gain * MULTI_UNITYGAIN_F; // TODO: proper roundoff?
	}

private:
	MULT_DATA_TYPE multiplier;
	audio_block_t *inputQueueArray[1];
};

#endif
=======
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

#ifndef mixer_h_
#define mixer_h_

#include "Arduino.h"
#include "AudioStream.h"

#define AudioMixer4 AudioMixer<4>

// the following Forward declarations 
// must be defined when we use template 
// the compiler throws some warnings that should be errors otherwise
static inline int32_t signed_multiply_32x16b(int32_t a, uint32_t b); 
static inline int32_t signed_multiply_32x16t(int32_t a, uint32_t b);
static inline int32_t signed_saturate_rshift(int32_t val, int bits, int rshift);
static inline uint32_t pack_16b_16b(int32_t a, int32_t b);
static inline uint32_t signed_add_16_and_16(uint32_t a, uint32_t b);

// because of the template use applyGain and applyGainThenAdd functions
// must be in this file and NOT in cpp file
#if defined(__ARM_ARCH_7EM__)
#define MULTI_UNITYGAIN 65536
#define MULTI_UNITYGAIN_F 65536.0f
#define MAX_GAIN 32767.0f
#define MIN_GAIN -32767.0f
#define MULT_DATA_TYPE int32_t

	static void applyGain(int16_t *data, int32_t mult)
	{
		uint32_t *p = (uint32_t *)data;
		const uint32_t *end = (uint32_t *)(data + AUDIO_BLOCK_SAMPLES);

		do {
			uint32_t tmp32 = *p; // read 2 samples from *data
			int32_t val1 = signed_multiply_32x16b(mult, tmp32);
			int32_t val2 = signed_multiply_32x16t(mult, tmp32);
			val1 = signed_saturate_rshift(val1, 16, 0);
			val2 = signed_saturate_rshift(val2, 16, 0);
			*p++ = pack_16b_16b(val2, val1);
		} while (p < end);
	}

	static void applyGainThenAdd(int16_t *data, const int16_t *in, int32_t mult)
	{
		uint32_t *dst = (uint32_t *)data;
		const uint32_t *src = (uint32_t *)in;
		const uint32_t *end = (uint32_t *)(data + AUDIO_BLOCK_SAMPLES);

		if (mult == MULTI_UNITYGAIN) {
			do {
				uint32_t tmp32 = *dst;
				*dst++ =  signed_add_16_and_16(tmp32, *src++);
				tmp32 = *dst;
				*dst++ =  signed_add_16_and_16(tmp32, *src++);
			} while (dst < end);
		} else {
			do {
				uint32_t tmp32 = *src++; // read 2 samples from *data
				int32_t val1 =  signed_multiply_32x16b(mult, tmp32);
				int32_t val2 =  signed_multiply_32x16t(mult, tmp32);
				val1 =  signed_saturate_rshift(val1, 16, 0);
				val2 =  signed_saturate_rshift(val2, 16, 0);
				tmp32 =  pack_16b_16b(val2, val1);
				uint32_t tmp32b = *dst;
				*dst++ =  signed_add_16_and_16(tmp32, tmp32b);
			} while (dst < end);
		}
	}

#elif defined(KINETISL)
#define MULTI_UNITYGAIN 256
#define MULTI_UNITYGAIN_F 256.0f
#define MAX_GAIN 127.0f
#define MIN_GAIN -127.0f
#define MULT_DATA_TYPE int16_t

	static void applyGain(int16_t *data, int32_t mult)
	{
		const int16_t *end = data + AUDIO_BLOCK_SAMPLES;

		do {
			int32_t val = *data * mult;
			*data++ = signed_saturate_rshift(val, 16, 0);
		} while (data < end);
	}

	static void applyGainThenAdd(int16_t *dst, const int16_t *src, int32_t mult)
	{
		const int16_t *end = dst + AUDIO_BLOCK_SAMPLES;

		if (mult == MULTI_UNITYGAIN) {
			do {
				int32_t val = *dst + *src++;
				*dst++ = signed_saturate_rshift(val, 16, 0);
			} while (dst < end);
		} else {
			do {
				int32_t val = *dst + ((*src++ * mult) >> 8); // overflow possible??
				*dst++ = signed_saturate_rshift(val, 16, 0);
			} while (dst < end);
		}
	}
#endif

template <int N>
class AudioMixer : public AudioStream
{
public:
	AudioMixer(void) : AudioStream(N, inputQueueArray) {
		for (int i=0; i<N; i++) multiplier[i] = MULTI_UNITYGAIN;
	}
	
	void update() {
		audio_block_t *in, *out=NULL;
		unsigned int channel;

		for (channel=0; channel < N; channel++) {
			if (!out) {
				out = receiveWritable(channel);
				if (out) {
					int32_t mult = multiplier[channel];
					if (mult != MULTI_UNITYGAIN) applyGain(out->data, mult);
				}
			} else {
				in = receiveReadOnly(channel);
				if (in) {
					applyGainThenAdd(out->data, in->data, multiplier[channel]);
					release(in);
				}
			}
		}
		if (out) {
			transmit(out);
			release(out);
		}
	}
	/**
	 * this sets the individual gain, this function is not used on AudioAmplifier
	 * and the gain is not set when used on that.
	 * @param channel
	 * @param gain
	 */
	void gain(unsigned int channel, float gain) {
		if (channel >= N) return;
		if (gain > MAX_GAIN) gain = MAX_GAIN;
		else if (gain < MIN_GAIN) gain = MIN_GAIN;
		multiplier[channel] = gain * MULTI_UNITYGAIN_F; // TODO: proper roundoff?
	}
	/**
	 * set all multipliers to specified gain
	 */
	void gain(float gain) {
		for (int i = 0; i < N; i++)
		{
			if (gain > MAX_GAIN) gain = MAX_GAIN;
			else if (gain < MIN_GAIN) gain = MIN_GAIN;
			multiplier[i] = gain * MULTI_UNITYGAIN_F; // TODO: proper roundoff?
		} 
	}

private:
	MULT_DATA_TYPE multiplier[N];
	audio_block_t *inputQueueArray[N];
};

class AudioAmplifier : public AudioStream
{
public:
	AudioAmplifier(void) : AudioStream(1, inputQueueArray) {
		multiplier = MULTI_UNITYGAIN;
	}

	void update() {
		audio_block_t *block;
		int32_t mult = multiplier;

		if (mult == 0) {
			// zero gain, discard any input and transmit nothing
			block = receiveReadOnly(0);
			if (block) release(block);
		} else if (mult == MULTI_UNITYGAIN) {
			// unity gain, pass input to output without any change
			block = receiveReadOnly(0);
			if (block) {
				transmit(block);
				release(block);
			}
		} else {
			// apply gain to signal
			block = receiveWritable(0);
			if (block) {
				applyGain(block->data, mult);
				transmit(block);
				release(block);
			}
		}
	}
	/**
	 * shorthand for gain(0, value)
	 */
	void gain(float gain) {
		if (gain > MAX_GAIN) gain = MAX_GAIN;
		else if (gain < MIN_GAIN) gain = MIN_GAIN;
		multiplier = gain * MULTI_UNITYGAIN_F; // TODO: proper roundoff?
	}

private:
	MULT_DATA_TYPE multiplier;
	audio_block_t *inputQueueArray[1];
};

#endif
>>>>>>> d5f297bdc1f9a85bfae2e4fabbe15222f83f0d47
