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
#define SPISETTING 		SPISettings(20'000'000, MSBFIRST, SPI_MODE0)
#define SPISETTING_PS 	SPISettings(72'000'000, MSBFIRST, SPI_MODE0) // you actually get 30MHz, but it seems the best achievable

// Use these with the audio adaptor board  (should be adjustable by the user...)
#define SPIRAM_MOSI_PIN  7
#define SPIRAM_MISO_PIN  12
#define SPIRAM_SCK_PIN   14

#define SPIRAM_CS_PIN    6

// Chip selects for 6x 23LC1024 board
#define MEMBOARD_CS0_PIN 2
#define MEMBOARD_CS1_PIN 3
#define MEMBOARD_CS2_PIN 4

// Chip selects for 8x PSRAM board
// The chip numbering on the PCB is slightly misleading. You'd expect
// Y0 to enable IC1, Y1 => IC2 and so on, but in fact it's:
// Y0 => IC7
// Y1 => IC5
// Y2 => IC3
// Y3 => IC1
// Y4 => IC8
// Y5 => IC6
// Y6 => IC4
// Y7 => IC2
// Could be of use if you want to make a partially populated board.
// setMuxPSRAMx8() does bit-twiddling to address from IC1 upwards
#define MEMBRD8M_CS0_PIN 2 
#define MEMBRD8M_CS1_PIN 3
#define MEMBRD8M_CS2_PIN 4
#define MEMBRD8M_ENL_PIN 5 // enable pin, active low: marked CS3

#define SIZEOF_SAMPLE (sizeof(((audio_block_t*) 0)->data[0]))

static const uint32_t NOT_ENOUGH_MEMORY = 0xFFFFFFFF;

// This memory size array needs to match the sizes of 
// the entries in AudioEffectDelayMemoryType_t
const uint32_t AudioExtMem::memSizeSamples[AUDIO_MEMORY_UNDEFINED+1] = {65536,65536*6,262144,4194304,8000,0,0,4194304*8};
bool AudioExtMem::chipResetNeeded[AUDIO_MEMORY_UNDEFINED+1] = {0,0,0,1,0,0,0,1};
AudioExtMem* AudioExtMem::first[AUDIO_MEMORY_UNDEFINED+1] = {nullptr};


AudioExtMem::~AudioExtMem() 
{
	switch (memory_type)
	{
		case AUDIO_MEMORY_HEAP:
			free((void*) memory_begin);
			break;
			
#if defined(ARDUINO_TEENSY41)
		case AUDIO_MEMORY_EXTMEM:
			extmem_free((void*) memory_begin);
			break;
#endif // defined(ARDUINO_TEENSY41)
			
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


/**
 * Prepare to initialise memory, but don't necessarily so it.
 * Due to the "C++ static initialization fiasco" we can't guarantee the SPI
 * object is initialised, so if the memory type in use is SPI-based we
 * wait until the first delay() call is made.
 */
void AudioExtMem::preInitialize(AudioEffectDelayMemoryType_t type, uint32_t samples, bool forceInitialize)
{
	memory_type = type;
	memory_length = samples;
	
	if (!IS_SPI_TYPE || forceInitialize)
		initialize();
}

void AudioExtMem::initialize(void)
{
	uint32_t avail;
	void* mem;
	AudioEffectDelayMemoryType_t type = memory_type;
	uint32_t samples = memory_length;
	
#if defined(INTERNAL_TEST)
	type = AUDIO_MEMORY_INTERNAL;
#endif // defined(INTERNAL_TEST)
	
	head_offset = 0;

	if (IS_SPI_TYPE)
	{
		// This takes advantage of the fact that for Teensy 4.x
		// the alternate settings are invalid, so the defaults
		// actually remain unchanged.
		SPI.setMOSI(SPIRAM_MOSI_PIN);
		SPI.setMISO(SPIRAM_MISO_PIN);
		SPI.setSCK(SPIRAM_SCK_PIN);

		SPI.begin();
	}
	
	// might need to do chip reset
	if (chipResetNeeded[type])
		chipReset(type);
		
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
			break;
			
		case AUDIO_MEMORY_PSRAM64_X8:
			pinMode(MEMBRD8M_CS0_PIN, OUTPUT);
			pinMode(MEMBRD8M_CS1_PIN, OUTPUT);
			pinMode(MEMBRD8M_CS2_PIN, OUTPUT);
			pinMode(MEMBRD8M_ENL_PIN, OUTPUT);
			digitalWriteFast(MEMBRD8M_CS0_PIN, LOW);
			digitalWriteFast(MEMBRD8M_CS1_PIN, LOW);
			digitalWriteFast(MEMBRD8M_CS2_PIN, LOW);		
			digitalWriteFast(MEMBRD8M_ENL_PIN, LOW);		
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
		case AUDIO_MEMORY_PSRAM64_X8:
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
			
#if defined(ARDUINO_TEENSY41)
		// PSRAM external memory
		case AUDIO_MEMORY_EXTMEM:
			mem = extmem_malloc(samples * SIZEOF_SAMPLE);
			if (nullptr != mem)
				memory_begin = (uint32_t) mem;
			else
				memory_begin = NOT_ENOUGH_MEMORY;
			break;
#endif // defined(ARDUINO_TEENSY41)
			
		default:  // invalid memory type
			memory_begin = NOT_ENOUGH_MEMORY;
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
	
	initialisationDone = true;
}


#ifdef INTERNAL_TEST
static int16_t testmem[8000]; // testing only
#endif

/**
 * Read multiple samples from SPI memory.
 * This version will use the transfer32() capability of the
 * Teensy 4.x SPI library to give a speed improvement
 * of ~10% (122us read time for 128 samples vs 136us
 * for the 16-bit-only option).
 */
void AudioExtMem::SPIreadMany(int16_t* data, 	//!< data destination, or NULL for dummy transfer
							  uint32_t samples)	//!< number of samples to read
{
	if (nullptr != data)
	{
#if defined(__IMXRT1062__)
		// get data pointer on a 4-byte boundary
		if (0 != (((uint32_t) data) & 2) && samples > 0)
		{
			*data++ = (int16_t)(SPI.transfer16(0));
			samples--;
		}
		
		// do as many 32-bit transfers as possible
		uint32_t* data32 = (uint32_t*) data;
		while (samples > 1)
		{
			*data32++ = (uint32_t)(SPI.transfer32(0));
			samples -= 2;
		}
		
		// drop out to mop up any remaining 16-bit reads
		data = (int16_t*) data32;
#endif // defined(__IMXRT1062__)
		while (samples--) 
			*data++ = (int16_t)(SPI.transfer16(0));
	}
	else
	{
		while (samples--) 
			(int16_t)(SPI.transfer16(0));
	}
}


/**
 * Write multiple samples to SPI memory.
 * Similar structure to SPIreadMany().
 */
void AudioExtMem::SPIwriteMany(const int16_t* data, uint32_t samples)
{
	if (nullptr != data)
	{
#if defined(__IMXRT1062__)
		// get data pointer on a 4-byte boundary
		if (0 != (((uint32_t) data) & 2) && samples > 0)
		{
			SPI.transfer16(*data++);
			samples--;
		}
		
		// do as many 32-bit transfers as possible
		uint32_t* data32 = (uint32_t*) data;
		while (samples > 1)
		{
			SPI.transfer32(*data32++);
			samples -= 2;
		}
		
		// drop out to mop up any remaining 16-bit reads
		data = (int16_t*) data32;
#endif // defined(__IMXRT1062__)		
		while (samples--) 
			SPI.transfer16(*data++);
	}
	else
	{
		while (samples--) 
			SPI.transfer16(0);
	}		
}

/**
 * Internal functions to set multiplexer for multi-chip boards
 */
static void setMuxMEMORYBOARD(int chip) //!< chip number, or -1 to de-select
{
	if (chip >= 0)
	{
		digitalWriteFast(MEMBOARD_CS0_PIN, chip & 1);
		digitalWriteFast(MEMBOARD_CS1_PIN, chip & 2);
		digitalWriteFast(MEMBOARD_CS2_PIN, chip & 4);
	}
	else
	{
		digitalWriteFast(MEMBOARD_CS0_PIN, 0);
		digitalWriteFast(MEMBOARD_CS1_PIN, 0);
		digitalWriteFast(MEMBOARD_CS2_PIN, 0);
	}
}


// This function ensures the PSRAM chips are addressed in ascending order,
// so a partially populated board can be assembled with the lowest numbered
// chips only.
static void setMuxPSRAMx8(int chip) //!< chip number, or -1 to de-select
{
	if (chip >= 0)
	{
		digitalWriteFast(MEMBRD8M_CS0_PIN, ~chip & 2);
		digitalWriteFast(MEMBRD8M_CS1_PIN, ~chip & 4);
		digitalWriteFast(MEMBRD8M_CS2_PIN,  chip & 1);
		digitalWriteFast(MEMBRD8M_ENL_PIN, LOW);
	}
	else
	{
		digitalWriteFast(MEMBRD8M_ENL_PIN, HIGH);
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
				uint32_t chip = (addr >> 16) + 1; // mux uses 1..6
				setMuxMEMORYBOARD(chip);
				uint32_t chipaddr = (addr & 0xFFFF) * SIZEOF_SAMPLE;
				SPI.transfer16((0x03 << 8) | (chipaddr >> 16));
				SPI.transfer16(chipaddr & 0xFFFF);
				uint32_t num = 0x10000 - (addr & 0xFFFF);
				
				if (num > count) num = count;
				count -= num;
				addr += num;
				SPIreadMany(data,num);
			}
			setMuxMEMORYBOARD(-1);
			SPI.endTransaction();
			break;
					
		case AUDIO_MEMORY_PSRAM64_X8:
			SPI.beginTransaction(SPISETTING_PS);
			while (count) {
				uint32_t chip = (addr >> 22);
				setMuxPSRAMx8(chip);
				uint32_t chipaddr = (addr & 0x3FFFFF) * SIZEOF_SAMPLE;
				SPI.transfer16((0x0B << 8) | (chipaddr >> 16)); // using fast read...
				SPI.transfer16(chipaddr & 0xFFFF);
				SPI.transfer((uint8_t) 0x0); // ...put in the 8 wait cycles (datasheet rev 2.3, figure 7)
				uint32_t num = 0x400000 - (addr & 0x3FFFFF);
				
				if (num > count) num = count;
				count -= num;
				addr += num;				
				SPIreadMany(data,num);
			}
			setMuxPSRAMx8(-1);
			SPI.endTransaction();
			break;
			
		case AUDIO_MEMORY_HEAP:
#if defined(ARDUINO_TEENSY41)
		case AUDIO_MEMORY_EXTMEM:
#endif // defined(ARDUINO_TEENSY41)
			addr = memory_begin + offset*SIZEOF_SAMPLE;
			if (nullptr != data)
				memcpy(data,(void*) addr,count*SIZEOF_SAMPLE);
			break;
			
		default:
			break;
	}
#endif
}


void AudioExtMem::readWrap(uint32_t offset, uint32_t count, int16_t *data)
{
	if (offset+count < memory_length)
		read(offset,count,data);
	else
	{
		uint32_t esz = memory_length - offset; // number of samples we can fit in at the end
		read(offset,esz,data);
		if (nullptr != data) // get null pointer when discarding
			data += esz;
		read(0,count - esz,data);
	}
}


void AudioExtMem::writeWrap(uint32_t offset, uint32_t count, const int16_t *data)
{
	if (offset+count < memory_length)
		write(offset,count,data);
	else
	{
		uint32_t esz = memory_length - offset; // number of samples we can fit in at the end
		write(offset,esz,data);
		if (nullptr != data) // get null pointer when zeroing
			data += esz;
		write(0,count - esz,data);
	}
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
				setMuxMEMORYBOARD(chip);
				uint32_t chipaddr = (addr & 0xFFFF) * SIZEOF_SAMPLE;
				SPI.transfer16((0x02 << 8) | (chipaddr >> 16));
				SPI.transfer16(chipaddr & 0xFFFF);
				uint32_t num = 0x10000 - (addr & 0xFFFF);
				
				if (num > count) num = count;
				count -= num;
				addr += num;				
				SPIwriteMany(data,num);
			}
				setMuxMEMORYBOARD(-1);
			SPI.endTransaction();
			break;
			
		case AUDIO_MEMORY_PSRAM64_X8:		
			SPI.beginTransaction(SPISETTING_PS);
			while (count) {
				uint32_t chip = (addr >> 22);
				setMuxPSRAMx8(chip);
				uint32_t chipaddr = (addr & 0x3FFFFF) * SIZEOF_SAMPLE;
				SPI.transfer16((0x02 << 8) | (chipaddr >> 16));
				SPI.transfer16(chipaddr & 0xFFFF);
				uint32_t num = 0x400000 - (addr & 0x3FFFFF);
				
				if (num > count) num = count;
				count -= num;
				addr += num;
				SPIwriteMany(data,num);
			}
			setMuxPSRAMx8(-1);
			SPI.endTransaction();
			break;
			
		case AUDIO_MEMORY_HEAP:
#if defined(ARDUINO_TEENSY41)
		case AUDIO_MEMORY_EXTMEM:
#endif // defined(ARDUINO_TEENSY41)
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


/**
 * Do reset sequence for a memory type, and mark as no longer needed.
 * Reset is documented for PSRAM parts, not clear if it's needed or
 * just useful to reduce power consumption.
 */
void AudioExtMem::chipReset(AudioEffectDelayMemoryType_t type)
{
	switch (type)
	{
		default: 
			break;
		
		case AUDIO_MEMORY_PSRAM64_X8:
			for (int chip = 0;chip < 8;chip++)
			{
				SPI.beginTransaction(SPISETTING_PS);
				setMuxPSRAMx8(chip);
				SPI.transfer(0x66);
				SPI.transfer(0x99);
				setMuxPSRAMx8(-1);
				SPI.endTransaction();
				delayMicroseconds(1);
			}
			chipResetNeeded[type] = false;
			break;
			
		case AUDIO_MEMORY_PSRAM64:
			SPI.beginTransaction(SPISETTING);
			digitalWriteFast(SPIRAM_CS_PIN, LOW);
			SPI.transfer(0x66);
			SPI.transfer(0x99);
			digitalWriteFast(SPIRAM_CS_PIN, HIGH);
			SPI.endTransaction();
			chipResetNeeded[type] = false;
			break;
	}
}