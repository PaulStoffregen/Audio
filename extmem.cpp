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

#include <Arduino.h>
#include "extmem.h"

//#define INTERNAL_TEST

// While 20 MHz (Teensy actually uses 16 MHz in most cases) and even 24 MHz
// have worked well in testing at room temperature with 3.3V power, to fully
// meet all the worst case timing specs, the SPI clock low time would need
// to be 40ns (12.5 MHz clock) for the single chip case and 51ns (9.8 MHz
// clock) for the 6-chip memoryboard with 74LCX126 buffers.
//
// Timing analysis and info is here:
// https://forum.pjrc.com/threads/29276-Limits-of-delay-effect-in-audio-library?p=97506&viewfull=1#post97506
#define SPISETTING SPISettings(40000000, MSBFIRST, SPI_MODE0)

// Use these with the audio adaptor board  (should be adjustable by the user...)
#define SPIRAM_MOSI_PIN  7
#define SPIRAM_MISO_PIN  12
#define SPIRAM_SCK_PIN   14

#define SPIRAM_CS_PIN    6

#define MEMBOARD_CS0_PIN 2
#define MEMBOARD_CS1_PIN 3
#define MEMBOARD_CS2_PIN 4

#define SIZEOF_SAMPLE (sizeof(((audio_block_t*) 0)->data[0]))

static const uint32_t NOT_ENOUGH_MEMORY = 0xFFFFFFFF;

//uint32_t AudioExtMem::allocated[AUDIO_MEMORY_UNDEFINED] = {0};
const uint32_t AudioExtMem::memSizeSamples[] = {65536,393216,262144,4194304,8000};
AudioExtMem* AudioExtMem::first[AUDIO_MEMORY_UNDEFINED] = {nullptr};


AudioExtMem::~AudioExtMem() 
{
	switch (memory_type)
	{
		case AUDIO_MEMORY_HEAP:
			free((void*) memory_begin);
			break;
			
		case AUDIO_MEMORY_EXTMEM:
			extmem_free((void*) memory_begin);
			break;
			
		// audio SPI memory is tracked by AudioExtMem 
		// objects thenselves - no need to free	
		default:
			break;		
	}
	linkOut();
}

void AudioExtMem::linkIn(void)
{
	if (memory_type < AUDIO_MEMORY_UNDEFINED)
	{
		AudioExtMem** ppEM = &first[memory_type]; 
		
		while (nullptr != *ppEM)
		{
			if (memory_begin > (*ppEM)->memory_begin)
				ppEM = &((*ppEM)->next);
			else
				break;
		}
		next = *ppEM;
		*ppEM = this;
	}
}


void AudioExtMem::linkOut(void)
{
	if (memory_type < AUDIO_MEMORY_UNDEFINED) // This Never Happens...
	{
		AudioExtMem** ppEM = &first[memory_type]; 
		
		while (nullptr != *ppEM)
		{
			if (this != *ppEM)
				ppEM = &((*ppEM)->next);
			else
			{
				*ppEM = next;
				break;
			}
		}
		next = nullptr; // not really necessary, but...
	}
}


/**
 * Find space for given number of samples. This MUST be called before the
 * newly-created AudioExtMem object is linked into the allocation list.
 */
uint32_t AudioExtMem::findSpace(AudioEffectDelayMemoryType_t memory_type, uint32_t samples)
{
	uint32_t result = 0;
	bool gotOne = false;
	
	if (memory_type < AUDIO_MEMORY_UNDEFINED) // This Never Happens...
	{
		AudioExtMem** ppEM = &first[memory_type]; 
		
		do
		{
			uint32_t next_start;
			if (nullptr == *ppEM) // end of list, or first memory allocation
				next_start = memSizeSamples[memory_type];
			else // we've found an object using memory
			{
				AudioExtMem* nextObj = (*ppEM)->next;
				result = (*ppEM)->memory_begin + (*ppEM)->memory_length; // end of object's allocation
				if (nullptr != nextObj)
					next_start = nextObj->memory_begin;
				else
					next_start = memSizeSamples[memory_type];
				
				ppEM = &((*ppEM)->next);
			}
			
			// simple-minded allocation: first found fit
			if (samples <= (next_start - result))
				gotOne = true;
			
		} while (!gotOne && nullptr != *ppEM);
	}	
	
	if (!gotOne)
		result = NOT_ENOUGH_MEMORY;
	
	return result;
}


/**
 * Find maximum contiguous space in a memory.
 */
uint32_t AudioExtMem::findMaxSpace(AudioEffectDelayMemoryType_t memory_type)
{
	uint32_t result = 0;
	uint32_t samples = 0;
	
	if (memory_type < AUDIO_MEMORY_UNDEFINED) // This Never Happens...
	{
		AudioExtMem** ppEM = &first[memory_type]; 
		
		do
		{
			uint32_t next_start;
			if (nullptr == *ppEM) // end of list, or first memory allocation
				next_start = memSizeSamples[memory_type];
			else // we've found an object using memory
			{
				AudioExtMem* nextObj = (*ppEM)->next;
				result = (*ppEM)->memory_begin + (*ppEM)->memory_length; // end of object's allocation
				if (nullptr != nextObj)
					next_start = nextObj->memory_begin;
				else
					next_start = memSizeSamples[memory_type];
				
				ppEM = &((*ppEM)->next);
			}
			
			// if space is bigger, bump the answer
			if (samples <= (next_start - result))
				samples  = (next_start - result);
			
		} while (nullptr != *ppEM);
	}	
	
	return samples;
}


void AudioExtMem::initialize(AudioEffectDelayMemoryType_t type, uint32_t samples)
{
	//uint32_t memsize, avail;
	uint32_t avail;
	void* mem;
	
#if defined(INTERNAL_TEST)
	type = AUDIO_MEMORY_INTERNAL;
#endif // defined(INTERNAL_TEST)
	
	head_offset = 0;
	memory_type = type;

	SPI.setMOSI(SPIRAM_MOSI_PIN);
	SPI.setMISO(SPIRAM_MISO_PIN);
	SPI.setSCK(SPIRAM_SCK_PIN);

	SPI.begin();
	//memsize = memSizeSamples[type];
	//Serial.printf("Requested %d samples\n",samples);
	
	switch (type)
	{
		case AUDIO_MEMORY_PSRAM64:
		case AUDIO_MEMORY_23LC1024:
		case AUDIO_MEMORY_CY15B104:
			pinMode(SPIRAM_CS_PIN, OUTPUT);
			digitalWriteFast(SPIRAM_CS_PIN, HIGH);
			break;
			
		case AUDIO_MEMORY_MEMORYBOARD:
			pinMode(MEMBOARD_CS0_PIN, OUTPUT);
			pinMode(MEMBOARD_CS1_PIN, OUTPUT);
			pinMode(MEMBOARD_CS2_PIN, OUTPUT);
			digitalWriteFast(MEMBOARD_CS0_PIN, LOW);
			digitalWriteFast(MEMBOARD_CS1_PIN, LOW);
			digitalWriteFast(MEMBOARD_CS2_PIN, LOW);		
			pinMode(SPIRAM_CS_PIN, OUTPUT);
			digitalWriteFast(SPIRAM_CS_PIN, HIGH);
			break;
			
		case AUDIO_MEMORY_INTERNAL:
		case AUDIO_MEMORY_HEAP:
		case AUDIO_MEMORY_EXTMEM:
			break;
			
		default:
			samples = 0;
			break;
	}
	
#define noOLD_ALLOCATE	
#if defined(OLD_ALLOCATE)
	avail = memsize - allocated[type];
	if (avail < AUDIO_BLOCK_SAMPLES*2+1) {
		memory_type = AUDIO_MEMORY_UNDEFINED;
		//return;
	}
	if (samples > avail) samples = avail;
	memory_begin = allocated[type];
	allocated[type] += samples;
#else
	switch (type)
	{
		// SPI memory
		// Emulate old behaviour: allocate biggest possible chunk
		// of delay memory if asked for more than is available.
		// Slightly different in dynamic system because of fragmentation,
		// but should be the same if used with legacy static design.
		case AUDIO_MEMORY_PSRAM64:
		case AUDIO_MEMORY_23LC1024:
		case AUDIO_MEMORY_CY15B104:
		case AUDIO_MEMORY_MEMORYBOARD:
			avail = findMaxSpace(type);
			if (samples > avail)
				samples = avail;
			//Serial.printf("findSpace says we could use %08lX\n",findSpace(type,samples));
			memory_begin = findSpace(type,samples);
			break;
			
		// processor heap: could be useful on Teensy 4.x etc.
		// In this case don't fill heap if asked for too much
		case AUDIO_MEMORY_HEAP:
			mem = malloc(samples * SIZEOF_SAMPLE);
			if (nullptr != mem)
				memory_begin = (uint32_t) mem;
			else
				memory_begin = NOT_ENOUGH_MEMORY;
			break;
			
		// PSRAM external memory
		case AUDIO_MEMORY_EXTMEM:
			mem = extmem_malloc(samples * SIZEOF_SAMPLE);
			if (nullptr != mem)
				memory_begin = (uint32_t) mem;
			else
				memory_begin = NOT_ENOUGH_MEMORY;
			break;
			
		default:
			break;
	}
	if (NOT_ENOUGH_MEMORY == memory_begin)
		memory_type = AUDIO_MEMORY_UNDEFINED;
#endif // defined(OLD_ALLOCATE)
	
	if (AUDIO_MEMORY_UNDEFINED != memory_type)
	{
		memory_length = samples;
		zero(0, memory_length);
		linkIn();
	}
	else
		memory_length = 0;
}


#ifdef INTERNAL_TEST
static int16_t testmem[8000]; // testing only
#endif

void AudioExtMem::SPIreadMany(int16_t* data, uint32_t samples)
{
	if (nullptr != data)
	{
		while (samples--) 
			*data++ = (int16_t)(SPI.transfer16(0));
	}
	else
	{
		while (samples--) 
			(int16_t)(SPI.transfer16(0));
	}
}


void AudioExtMem::SPIwriteMany(const int16_t* data, uint32_t samples)
{
	if (nullptr != data)
	{
		while (samples--) 
			SPI.transfer16(*data++);
	}
	else
	{
		while (samples--) 
			SPI.transfer16(0);
	}		
}

void AudioExtMem::read(uint32_t offset, uint32_t count, int16_t *data)
{
	uint32_t addr = memory_begin + offset;

#ifdef INTERNAL_TEST
	if (nullptr != data) while (count) { *data++ = testmem[addr++]; count--; } // testing only
#else
	switch (memory_type)
	{
		case AUDIO_MEMORY_23LC1024:
		case AUDIO_MEMORY_PSRAM64: 
		case AUDIO_MEMORY_CY15B104:
			addr *= SIZEOF_SAMPLE;
			SPI.beginTransaction(SPISETTING);
			digitalWriteFast(SPIRAM_CS_PIN, LOW);
			SPI.transfer16((0x03 << 8) | (addr >> 16));
			SPI.transfer16(addr & 0xFFFF);
			SPIreadMany(data,count);
			digitalWriteFast(SPIRAM_CS_PIN, HIGH);
			SPI.endTransaction();
			break;
		
		case AUDIO_MEMORY_MEMORYBOARD:
			SPI.beginTransaction(SPISETTING);
			while (count) {
				uint32_t chip = (addr >> 16) + 1;
				digitalWriteFast(MEMBOARD_CS0_PIN, chip & 1);
				digitalWriteFast(MEMBOARD_CS1_PIN, chip & 2);
				digitalWriteFast(MEMBOARD_CS2_PIN, chip & 4);
				uint32_t chipaddr = (addr & 0xFFFF) * SIZEOF_SAMPLE;
				SPI.transfer16((0x03 << 8) | (chipaddr >> 16));
				SPI.transfer16(chipaddr & 0xFFFF);
				uint32_t num = 0x10000 - (addr & 0xFFFF);
				if (num > count) num = count;
				count -= num;
				addr += num;
				SPIreadMany(data,num);
			}
			digitalWriteFast(MEMBOARD_CS0_PIN, LOW);
			digitalWriteFast(MEMBOARD_CS1_PIN, LOW);
			digitalWriteFast(MEMBOARD_CS2_PIN, LOW);
			SPI.endTransaction();
			break;
			
		case AUDIO_MEMORY_HEAP:
		case AUDIO_MEMORY_EXTMEM:
			addr = memory_begin + offset*SIZEOF_SAMPLE;
			if (nullptr != data)
				memcpy(data,(void*) addr,count*SIZEOF_SAMPLE);
			break;
			
		default:
			break;
	}
#endif
}

void AudioExtMem::write(uint32_t offset, uint32_t count, const int16_t *data)
{
	uint32_t addr = memory_begin + offset;

#ifdef INTERNAL_TEST
	while (count) { testmem[addr++] = *data++; count--; } // testing only
#else
	switch (memory_type)
	{
		case AUDIO_MEMORY_23LC1024:
		case AUDIO_MEMORY_PSRAM64:
			addr *= SIZEOF_SAMPLE;
			SPI.beginTransaction(SPISETTING);
			digitalWriteFast(SPIRAM_CS_PIN, LOW);
			SPI.transfer16((0x02 << 8) | (addr >> 16));
			SPI.transfer16(addr & 0xFFFF);
			SPIwriteMany(data,count);
			digitalWriteFast(SPIRAM_CS_PIN, HIGH);
			SPI.endTransaction();
			break;
			
		case AUDIO_MEMORY_CY15B104:
			addr *= SIZEOF_SAMPLE;

			SPI.beginTransaction(SPISETTING);
			digitalWriteFast(SPIRAM_CS_PIN, LOW);
			SPI.transfer(0x06); //write-enable before every write
			digitalWriteFast(SPIRAM_CS_PIN, HIGH);
			asm volatile ("NOP\n NOP\n NOP\n NOP\n NOP\n NOP\n");
			digitalWriteFast(SPIRAM_CS_PIN, LOW);
			SPI.transfer16((0x02 << 8) | (addr >> 16));
			SPI.transfer16(addr & 0xFFFF);
			SPIwriteMany(data,count);
			digitalWriteFast(SPIRAM_CS_PIN, HIGH);
			SPI.endTransaction();	
			break;
			
		case AUDIO_MEMORY_MEMORYBOARD:		
			SPI.beginTransaction(SPISETTING);
			while (count) {
				uint32_t chip = (addr >> 16) + 1;
				digitalWriteFast(MEMBOARD_CS0_PIN, chip & 1);
				digitalWriteFast(MEMBOARD_CS1_PIN, chip & 2);
				digitalWriteFast(MEMBOARD_CS2_PIN, chip & 4);
				uint32_t chipaddr = (addr & 0xFFFF) * SIZEOF_SAMPLE;
				SPI.transfer16((0x02 << 8) | (chipaddr >> 16));
				SPI.transfer16(chipaddr & 0xFFFF);
				uint32_t num = 0x10000 - (addr & 0xFFFF);
				if (num > count) num = count;
				count -= num;
				addr += num;
				SPIwriteMany(data,num);
			}
			digitalWriteFast(MEMBOARD_CS0_PIN, LOW);
			digitalWriteFast(MEMBOARD_CS1_PIN, LOW);
			digitalWriteFast(MEMBOARD_CS2_PIN, LOW);
			SPI.endTransaction();
			break;
			
		case AUDIO_MEMORY_HEAP:
		case AUDIO_MEMORY_EXTMEM:
			addr = memory_begin + offset*SIZEOF_SAMPLE;
			if (nullptr != data)
				memcpy((void*) addr,data,count*SIZEOF_SAMPLE);
			else
				memset((void*) addr,0,count*SIZEOF_SAMPLE);
			break;	
			
		default:
			break;
	}
#endif
}
