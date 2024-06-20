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

#ifndef analyze_fft1024_h_
#define analyze_fft1024_h_

#include <Arduino.h>     // github.com/PaulStoffregen/cores/blob/master/teensy4/Arduino.h
#include <AudioStream.h> // github.com/PaulStoffregen/cores/blob/master/teensy4/AudioStream.h
#include <arm_math.h>    // github.com/PaulStoffregen/cores/blob/master/teensy4/arm_math.h

// windows.c
extern "C" {
extern const int16_t AudioWindowHanning1024[];
extern const int16_t AudioWindowBartlett1024[];
extern const int16_t AudioWindowBlackman1024[];
extern const int16_t AudioWindowFlattop1024[];
extern const int16_t AudioWindowBlackmanHarris1024[];
extern const int16_t AudioWindowNuttall1024[];
extern const int16_t AudioWindowBlackmanNuttall1024[];
extern const int16_t AudioWindowWelch1024[];
extern const int16_t AudioWindowHamming1024[];
extern const int16_t AudioWindowCosine1024[];
extern const int16_t AudioWindowTukey1024[];
}

class AudioAnalyzeFFT1024 : public AudioStream
{
public:
	AudioAnalyzeFFT1024() : AudioStream(1, inputQueueArray),
	  window(AudioWindowHanning1024), state(0), outputflag(false) {
		arm_cfft_radix4_init_q15(&fft_inst, 1024, 0, 1);
	}
	bool available() {
		if (outputflag == true) {
			outputflag = false;
			return true;
		}
		return false;
	}

	// For details on the internal structure of complex numbers, see the declaration of output[] down below.
	// Note that only the raw complex numbers and their magnitudes are stored while all phase computations
	// are done "live" without any form of precalculation.
	// That's a deliberate design decision since it's unlikely that a user needs all 512 phases.
	// Moreover, most users don't require the phases anyway.
	// Instead, the phase retrieval method only computes what the user actually needs and when he needs it.
	// Although a few users are now responsible for trying to only retrieve each phase once for maximum efficiency,
	// that's better than wasting a lot of other users' processing time with useless computations.
	uint32_t readCmplxNumber(unsigned int binNumber) {
		if (binNumber > 511) return 0;
		return outputCmplx[binNumber];
	}
	int16_t readCmplxRealPart(unsigned int binNumber) {
		if (binNumber > 511) return 0;
		return outputCmplx[binNumber] & 0x0000ffff;
	}
	int16_t readCmplxImaginaryPart(unsigned int binNumber) {
		if (binNumber > 511) return 0;
		return outputCmplx[binNumber] >> 16;
	}
	uint16_t readCmplxMagnitude(unsigned int binNumber) {
		if (binNumber > 511) return 0.0;
		return output[binNumber];
	}
	float readAmplitude(unsigned int binNumber) {
		if (binNumber > 511) return 0.0;
		return (float)(output[binNumber]) * (1.0f / 16384.0f);
	}
	float readAmplitudes(unsigned int binFirst, unsigned int binLast) {
		if (binFirst > binLast) {
			unsigned int tmp = binLast;
			binLast = binFirst;
			binFirst = tmp;
		}
		if (binFirst > 511) return 0.0;
		if (binLast > 511) binLast = 511;
		uint32_t sum = 0;
		do {
			sum += output[binFirst++];
		} while (binFirst <= binLast);
		return (float)sum * (1.0f / 16384.0f);
	}
	float readPhase(unsigned int binNumber);

	void averageTogether(uint8_t n) {
		// not implemented yet (may never be, 86 Hz output rate is ok)
	}
	void windowFunction(const int16_t *w) {
		window = w;
	}
	virtual void update(void);

	// DEPRECATED members; they exist for backwards compatibility
	float read(unsigned int binNumber) {
		return readAmplitude(binNumber);
	}
	float read(unsigned int binFirst, unsigned int binLast) {
		return readAmplitudes(binFirst, binLast);
	}
	uint16_t output[512] __attribute__ ((aligned (4))); // stores raw magnitudes; should be private
private:
	void init(void);
	const int16_t *window;
	audio_block_t *blocklist[8];
	int16_t buffer[2048] __attribute__ ((aligned (4)));
	//uint32_t sum[512];
	//uint8_t count;
	uint8_t state;
	//uint8_t naverage;
	volatile bool outputflag;
	audio_block_t *inputQueueArray[1];
	arm_cfft_radix4_instance_q15 fft_inst;

	// Stores a 32-bit complex number for each output bin.
	// The 16 least significant bits are the real part, the 16 most significant bits are the imaginary part.
	// Both parts are signed.
	uint32_t outputCmplx[512] __attribute__ ((aligned (4)));
};

#endif
