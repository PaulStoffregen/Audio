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

#ifndef _extmem_h_
#define _extmem_h_
#include "Arduino.h"
#include "spi_interrupt.h"

// A quick experiment with a Teensy 4.1 
// suggests a 1-input 3-tap delay line uses:
// mem type		CPU%
// Heap			<tiny%>
// EXTMEM		1.3%
// SPI PSRAM	12% (40MHz SPI clock)
// SPI PSRAM	19% (20MHz SPI clock)

enum AudioEffectDelayMemoryType_t {
	AUDIO_MEMORY_23LC1024 = 0,	// 128k x 8 S-RAM (1.48s @ 44kHz / 16 bit)
	AUDIO_MEMORY_MEMORYBOARD = 1, // 6x 128k x 8 (8.9s @ 44kHz / 16 bit)
	AUDIO_MEMORY_CY15B104 = 2,	// 512k x 8 F-RAM	(5.9s @ 44kHz / 16 bit)
	AUDIO_MEMORY_PSRAM64 = 3,	// 64Mb / 8MB PSRAM (95s @ 44kHz / 16 bit)
	AUDIO_MEMORY_INTERNAL = 4,  // 8000 samples (181ms), for test only!
	AUDIO_MEMORY_HEAP,
	AUDIO_MEMORY_EXTMEM,
	AUDIO_MEMORY_UNDEFINED
};


class AudioExtMem
{
public:
	AudioExtMem(AudioEffectDelayMemoryType_t type, uint32_t samples = AUDIO_SAMPLE_RATE_EXACT)
		: memory_begin(0)
	{
		initialize(type, samples);
	}
	AudioExtMem() :	AudioExtMem(AUDIO_MEMORY_23LC1024, 65536) {}
	~AudioExtMem();
	float getMaxDelay(void) {return (float) memory_length * 1000.0f / AUDIO_SAMPLE_RATE_EXACT;}
	
private:	
	void initialize(AudioEffectDelayMemoryType_t type, uint32_t samples);
	inline static void SPIreadMany(int16_t* data, uint32_t samples);
	inline static void SPIwriteMany(const int16_t* data, uint32_t samples);
	//static uint32_t allocated[AUDIO_MEMORY_UNDEFINED];
	const static uint32_t memSizeSamples[AUDIO_MEMORY_UNDEFINED];
	static AudioExtMem* first[AUDIO_MEMORY_UNDEFINED]; // linked lists of all SPI memory types
	AudioExtMem* next; // next allocation after this one
	void linkIn(void);  // link new AudioExtMem object into allocation chain
	void linkOut(void); // unlink this AudioExtMem object from allocation chain
	static uint32_t findSpace(AudioEffectDelayMemoryType_t memory_type, uint32_t samples); // find a space for a delay buffer
	static uint32_t findMaxSpace(AudioEffectDelayMemoryType_t memory_type); // find max available space for a delay buffer
	uint32_t memory_begin;    // the first address in the memory we're using
	
protected:
	void read(uint32_t address, uint32_t count, int16_t *data);
	void write(uint32_t address, uint32_t count, const int16_t *data);
	void zero(uint32_t address, uint32_t count) {
		write(address, count, NULL);
	}
	uint32_t memory_length;   // the amount of memory we're using
	uint32_t head_offset;     // head index (incoming) data into external memory
	AudioEffectDelayMemoryType_t  memory_type;     // 0=23LC1024, 1=Frank's Memoryboard, etc.
};

#endif // _extmem_h_
