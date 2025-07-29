/* SPDIF for Teensy 3.X
 * Copyright (c) 2015, Frank Bösing, f.boesing@gmx.de,
 * Copyright (c) 2025, Andrew Dunstan, a_dunstan@hotmail.com
 * Thanks to KPC & Paul Stoffregen!
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

#ifndef output_I2S_SPDIF_h_
#define output_I2S_SPDIF_h_

#include <Arduino.h>     // github.com/PaulStoffregen/cores/blob/master/teensy4/Arduino.h
#include <AudioStream.h> // github.com/PaulStoffregen/cores/blob/master/teensy4/AudioStream.h
#include <DMAChannel.h>  // github.com/PaulStoffregen/cores/blob/master/teensy4/DMAChannel.h
#include "utility/imxrt_hw.h"


template <class SPDIF>
class AudioOutputI2S_SPDIF : public AudioStream
{
public:
	static void mute_PCM(bool mute);
protected:
	AudioOutputI2S_SPDIF(void) : AudioStream(2, inputQueueArray) { mute_PCM(false); };
	static void isr(void);
private:
	void update(void) override;
	audio_block_t *inputQueueArray[2];
};

union channel_status {
	struct {
		// byte 0
		uint32_t pro:1;            // 0=consumer format, 1=professional format
		uint32_t audio:1;          // 0=linear PCM, 1=non-PCM
		uint32_t copy_permitted:1; // 0=copy inhibited, 1=copy permitted
		uint32_t pre_emphasis:3;   // 0=none(2ch), 1=50/15us(2ch), 2/3=reserved(2ch), others=reserved(4ch)
		uint32_t mode:2;           // 0=mode 0 (defines next 24 bits)
		// byte 1
		uint32_t category:7;       // 0=general
		uint32_t generation:1;     // 0=none/1st generation, 1=original/commercial
		// byte 2
		uint32_t source:4;         // 0=unspecified
		uint32_t channel:4;        // 0=unspecified
		// byte 3
		uint32_t Fs:4;             // sampling rate: 0=44.1kHz, 2=48, 3=32, others=reserved
		uint32_t clock_accuracy:2; // 0=+/- 1000ppm, 1=+/- 50ppm, 2=variable, 3=reserved
		uint32_t :2;               // reserved
	};
	uint32_t bits;

	bool operator[] (int index) const {
		if (index >= 0 && index < 32)
			return (bits >> index) & 1;
		return false;
	}
};

// 1 bit of aux is included here, since it gets used as the "real" parity
#define VUCP_VALID    (0xCC008000) // V=0,U=0,C=0,P=0
#define VUCP_INVALID  (0xAC008000) // V=1,U=1,C=0,P=0. To mute PCM, set VUCP = invalid.
#define VUCP_C_TOGGLE (0x18000000) // XOR with VUCP to flip U and C (keep parity even)

#define PREAMBLE_B  ((0xE8) << 16) //11101000
#define PREAMBLE_M  ((0xE2) << 16) //11100010
#define PREAMBLE_W  ((0xE4) << 16) //11100100

extern const int16_t spdif_bmclookup[256];
extern const channel_status consumer_channel_status;

#if defined(KINETISK) || defined(__IMXRT1062__)

/*

 http://www.hardwarebook.info/S/PDIF

 1. To make it easier and a bit faster, the parity-bit is always the same.
  - With a alternating parity we had to adjust the next subframe. Instead, use a bit from the aux-info as parity.

 2. The buffer is filled with an offset of 1 byte, so the last parity (which is always 0 now (see 1.) ) is written as first byte.
  -> A bit easier and faster to construct both subframes.

*/

// pulls a 16-bit sample from the given channel, returns encoded 32-bit value
static int32_t next_encoded_sample(const int16_t* &src) {
	if (src) {
		uint16_t sample = (uint16_t)*src++;

		int32_t hi = spdif_bmclookup[sample >> 8];
		int32_t lo = ~spdif_bmclookup[sample & 0xFF];
		return (lo << 16) ^ hi;
	}

	// silence: return encoded 0
	return 0xCCCCCCCC;
}

template <class S>
void AudioOutputI2S_SPDIF<S>::isr(void)
{
	static int frame = 0;
	int32_t *end, *dest;
	uint32_t saddr;

#if defined(KINETISK) || defined(__IMXRT1062__)
	saddr = (uint32_t)(S::dma.TCD->SADDR);
#endif
	S::dma.clearInterrupt();
	if (saddr < (uint32_t)S::tx_buffer + sizeof(S::tx_buffer) / 2) {
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		dest = &S::tx_buffer[AUDIO_BLOCK_SAMPLES * 4/2];
		end = &S::tx_buffer[AUDIO_BLOCK_SAMPLES * 4];
		if (S::update_responsibility) update_all();
	} else {
		// DMA is transmitting the second half of the buffer
		// so we must fill the first half
		dest = S::tx_buffer;
		end = &S::tx_buffer[AUDIO_BLOCK_SAMPLES * 4/2];
	}

	const int16_t* leftsrc = S::block_left_1st != NULL ? &S::block_left_1st->data[S::block_left_offset] : NULL;
	const int16_t* rightsrc = S::block_right_1st != NULL ? &S::block_right_1st->data[S::block_right_offset] : NULL;

	do {
		// each output sample is 8 bytes, generate 2 stereo samples (2x2x8=32) to fill a cacheline
		for (int i=0; i < 2; i++) {
			int32_t left  = next_encoded_sample(leftsrc);
			int32_t right = next_encoded_sample(rightsrc);

			uint16_t laux = 0x3333 ^ (left >> 31);
			uint16_t raux = 0x3333 ^ (right >> 31);

			if (++frame > 191) frame = 0;

			// 1 byte of previous subframe 1, 3 bytes subframe 0 (channel 1)
			dest[0] = S::vucp | (frame==0 ? PREAMBLE_B : PREAMBLE_M) | laux;
			dest[1] = left;
			// 1 byte of subframe 0, 3 bytes of subframe 1 (channel 2)
			dest[2] = S::vucp | PREAMBLE_W | raux;
			dest[3] = right;

			/* channel status is identical for both channels, but
			 * the status for channel 2 is one frame delayed due to
			 * the one byte offset in our output.
			 */
			if (frame <= 32) {
				// set status for previous subframe 1/channel 2
				if (consumer_channel_status[frame-1]) dest[0] ^= VUCP_C_TOGGLE;
				// set status for subframe 0/channel 1
				if (consumer_channel_status[frame]) dest[2] ^= VUCP_C_TOGGLE;
			}

			dest += 4;
		}

		#if IMXRT_CACHE_ENABLED >= 2
		// flush cacheline
		SCB_CACHE_DCCMVAC = (uint32_t)(dest-8);
		#endif
	} while (dest < end);

	if (leftsrc) {
		if (S::block_left_offset >= AUDIO_BLOCK_SAMPLES/2) {
			S::block_left_offset = 0;
			release(S::block_left_1st);
			S::block_left_1st = S::block_left_2nd;
			S::block_left_2nd = NULL;
		}
		else S::block_left_offset += AUDIO_BLOCK_SAMPLES/2;
	}
	if (rightsrc) {
		if (S::block_right_offset >= AUDIO_BLOCK_SAMPLES/2) {
			S::block_right_offset = 0;
			release(S::block_right_1st);
			S::block_right_1st = S::block_right_2nd;
			S::block_right_2nd = NULL;
		}
		else S::block_right_offset += AUDIO_BLOCK_SAMPLES/2;
	}

	#if IMXRT_CACHE_ENABLED >= 2
	// ensure cache operations are complete
	asm volatile("dsb");
	#endif
}

template <class S>
void AudioOutputI2S_SPDIF<S>::mute_PCM(bool mute)
{
	S::vucp = mute?VUCP_INVALID:VUCP_VALID;
}

template <class S>
void AudioOutputI2S_SPDIF<S>::update(void)
{
	audio_block_t *block;
	block = receiveReadOnly(0); // input 0 = left channel
	if (block) {
		__disable_irq();
		if (S::block_left_1st == NULL) {
			S::block_left_1st = block;
			S::block_left_offset = 0;
			__enable_irq();
		} else if (S::block_left_2nd == NULL) {
			S::block_left_2nd = block;
			__enable_irq();
		} else {
			audio_block_t *tmp = S::block_left_1st;
			S::block_left_1st = S::block_left_2nd;
			S::block_left_2nd = block;
			S::block_left_offset = 0;
			__enable_irq();
			release(tmp);
		}
	}
	block = receiveReadOnly(1); // input 1 = right channel
	if (block) {
		__disable_irq();
		if (S::block_right_1st == NULL) {
			S::block_right_1st = block;
			S::block_right_offset = 0;
			__enable_irq();
		} else if (S::block_right_2nd == NULL) {
			S::block_right_2nd = block;
			__enable_irq();
		} else {
			audio_block_t *tmp = S::block_right_1st;
			S::block_right_1st = S::block_right_2nd;
			S::block_right_2nd = block;
			S::block_right_offset = 0;
			__enable_irq();
			release(tmp);
		}
	}
}

#endif // KINETISK || __IMXRT1062__


#if defined(KINETISL)

template <class S>
void AudioOutputI2S_SPDIF<S>::update(void)
{
	audio_block_t *block;
	block = receiveReadOnly(0); // input 0 = left channel
	if (block) release(block);
	block = receiveReadOnly(1); // input 1 = right channel
	if (block) release(block);
}

#endif


#endif
