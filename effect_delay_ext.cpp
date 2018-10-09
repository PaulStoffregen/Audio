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

#include "effect_delay_ext.h"

#if defined(ARDUINO_ARCH_SAMD)
#include <Arduino.h>
#define digitalWriteFast digitalWrite
#endif


//#define INTERNAL_TEST

// While 20 MHz (Teensy actually uses 16 MHz in most cases) and even 24 MHz
// have worked well in testing at room temperature with 3.3V power, to fully
// meet all the worst case timing specs, the SPI clock low time would need
// to be 40ns (12.5 MHz clock) for the single chip case and 51ns (9.8 MHz
// clock) for the 6-chip memoryboard with 74LCX126 buffers.
//
// Timing analysis and info is here:
// https://forum.pjrc.com/threads/29276-Limits-of-delay-effect-in-audio-library?p=97506&viewfull=1#post97506
#define SPISETTING SPISettings(20000000, MSBFIRST, SPI_MODE0)

// Use these with the audio adaptor board  (should be adjustable by the user...)
#define SPIRAM_MOSI_PIN  7
#define SPIRAM_MISO_PIN  12
#define SPIRAM_SCK_PIN   14

#define SPIRAM_CS_PIN    6

#define MEMBOARD_CS0_PIN 2
#define MEMBOARD_CS1_PIN 3
#define MEMBOARD_CS2_PIN 4

void AudioEffectDelayExternal::update(void)
{
	audio_block_t *block;
	uint32_t n, channel, read_offset;

	// grab incoming data and put it into the memory
	block = receiveReadOnly();
	if (memory_type >= AUDIO_MEMORY_UNDEFINED) {
		// ignore input and do nothing if undefined memory type
		release(block);
		return;
	}
	if (block) {
		if (head_offset + AUDIO_BLOCK_SAMPLES <= memory_length) {
			// a single write is enough
			write(head_offset, AUDIO_BLOCK_SAMPLES, block->data);
			head_offset += AUDIO_BLOCK_SAMPLES;
		} else {
			// write wraps across end-of-memory
			n = memory_length - head_offset;
			write(head_offset, n, block->data);
			head_offset = AUDIO_BLOCK_SAMPLES - n;
			write(0, head_offset, block->data + n);
		}
		release(block);
	} else {
		// if no input, store zeros, so later playback will
		// not be random garbage previously stored in memory
		if (head_offset + AUDIO_BLOCK_SAMPLES <= memory_length) {
			zero(head_offset, AUDIO_BLOCK_SAMPLES);
			head_offset += AUDIO_BLOCK_SAMPLES;
		} else {
			n = memory_length - head_offset;
			zero(head_offset, n);
			head_offset = AUDIO_BLOCK_SAMPLES - n;
			zero(0, head_offset);
		}
	}

	// transmit the delayed outputs
	for (channel = 0; channel < 8; channel++) {
		if (!(activemask & (1<<channel))) continue;
		block = allocate();
		if (!block) continue;
		// compute the delayed location where we read
		if (delay_length[channel] <= head_offset) {
			read_offset = head_offset - delay_length[channel];
		} else {
			read_offset = memory_length + head_offset - delay_length[channel];
		}
		if (read_offset + AUDIO_BLOCK_SAMPLES <= memory_length) {
			// a single read will do it
			read(read_offset, AUDIO_BLOCK_SAMPLES, block->data);
		} else {
			// read wraps across end-of-memory
			n = memory_length - read_offset;
			read(read_offset, n, block->data);
			read(0, AUDIO_BLOCK_SAMPLES - n, block->data + n);
		}
		transmit(block, channel);
		release(block);
	}
}

uint32_t AudioEffectDelayExternal::allocated[2] = {0, 0};

void AudioEffectDelayExternal::initialize(AudioEffectDelayMemoryType_t type, uint32_t samples)
{
	uint32_t memsize, avail;

	activemask = 0;
	head_offset = 0;
	memory_type = type;

#if !defined(ARDUINO_ARCH_SAMD)
	SPI.setMOSI(SPIRAM_MOSI_PIN);
	SPI.setMISO(SPIRAM_MISO_PIN);
	SPI.setSCK(SPIRAM_SCK_PIN);
#endif

	SPI.begin();	
	
	if (type == AUDIO_MEMORY_23LC1024) {
#ifdef INTERNAL_TEST
		memsize = 8000;
#else
		memsize = 65536;
#endif
		pinMode(SPIRAM_CS_PIN, OUTPUT);
		digitalWriteFast(SPIRAM_CS_PIN, HIGH);
	} else if (type == AUDIO_MEMORY_MEMORYBOARD) {
		memsize = 393216;
		pinMode(MEMBOARD_CS0_PIN, OUTPUT);
		pinMode(MEMBOARD_CS1_PIN, OUTPUT);
		pinMode(MEMBOARD_CS2_PIN, OUTPUT);
		digitalWriteFast(MEMBOARD_CS0_PIN, LOW);
		digitalWriteFast(MEMBOARD_CS1_PIN, LOW);
		digitalWriteFast(MEMBOARD_CS2_PIN, LOW);		
	} else if (type == AUDIO_MEMORY_CY15B104) {
#ifdef INTERNAL_TEST
		memsize = 8000;
#else		
		memsize = 262144;
#endif	
		pinMode(SPIRAM_CS_PIN, OUTPUT);
		digitalWriteFast(SPIRAM_CS_PIN, HIGH);
			
	} else {
		return;
	}
	avail = memsize - allocated[type];
	if (avail < AUDIO_BLOCK_SAMPLES*2+1) {
		memory_type = AUDIO_MEMORY_UNDEFINED;
		return;
	}
	if (samples > avail) samples = avail;
	memory_begin = allocated[type];
	allocated[type] += samples;
	memory_length = samples;

	zero(0, memory_length);
}


#ifdef INTERNAL_TEST
static int16_t testmem[8000]; // testing only
#endif

void AudioEffectDelayExternal::read(uint32_t offset, uint32_t count, int16_t *data)
{
	uint32_t addr = memory_begin + offset;

#ifdef INTERNAL_TEST
	while (count) { *data++ = testmem[addr++]; count--; } // testing only
#else
	if (memory_type == AUDIO_MEMORY_23LC1024 || 
		memory_type == AUDIO_MEMORY_CY15B104) {
		addr *= 2;
		SPI.beginTransaction(SPISETTING);
		digitalWriteFast(SPIRAM_CS_PIN, LOW);
		SPI.transfer16((0x03 << 8) | (addr >> 16));
		SPI.transfer16(addr & 0xFFFF);
		while (count) {
			*data++ = (int16_t)(SPI.transfer16(0));
			count--;
		}
		digitalWriteFast(SPIRAM_CS_PIN, HIGH);
		SPI.endTransaction();
	} else if (memory_type == AUDIO_MEMORY_MEMORYBOARD) {
		SPI.beginTransaction(SPISETTING);
		while (count) {
			uint32_t chip = (addr >> 16) + 1;
			digitalWriteFast(MEMBOARD_CS0_PIN, chip & 1);
			digitalWriteFast(MEMBOARD_CS1_PIN, chip & 2);
			digitalWriteFast(MEMBOARD_CS2_PIN, chip & 4);
			uint32_t chipaddr = (addr & 0xFFFF) << 1;
			SPI.transfer16((0x03 << 8) | (chipaddr >> 16));
			SPI.transfer16(chipaddr & 0xFFFF);
			uint32_t num = 0x10000 - (addr & 0xFFFF);
			if (num > count) num = count;
			count -= num;
			addr += num;
			do {
				*data++ = (int16_t)(SPI.transfer16(0));
			} while (--num > 0);
		}
		digitalWriteFast(MEMBOARD_CS0_PIN, LOW);
		digitalWriteFast(MEMBOARD_CS1_PIN, LOW);
		digitalWriteFast(MEMBOARD_CS2_PIN, LOW);
		SPI.endTransaction();
	}
#endif
}

void AudioEffectDelayExternal::write(uint32_t offset, uint32_t count, const int16_t *data)
{
	uint32_t addr = memory_begin + offset;

#ifdef INTERNAL_TEST
	while (count) { testmem[addr++] = *data++; count--; } // testing only
#else
	if (memory_type == AUDIO_MEMORY_23LC1024) {
		addr *= 2;
		SPI.beginTransaction(SPISETTING);
		digitalWriteFast(SPIRAM_CS_PIN, LOW);
		SPI.transfer16((0x02 << 8) | (addr >> 16));
		SPI.transfer16(addr & 0xFFFF);
		while (count) {
			int16_t w = 0;
			if (data) w = *data++;
			SPI.transfer16(w);
			count--;
		}
		digitalWriteFast(SPIRAM_CS_PIN, HIGH);
		SPI.endTransaction();
	} else if (memory_type == AUDIO_MEMORY_CY15B104) {
		addr *= 2;

		SPI.beginTransaction(SPISETTING);
		digitalWriteFast(SPIRAM_CS_PIN, LOW);
		SPI.transfer(0x06); //write-enable before every write
		digitalWriteFast(SPIRAM_CS_PIN, HIGH);
		asm volatile ("NOP\n NOP\n NOP\n NOP\n NOP\n NOP\n");
		digitalWriteFast(SPIRAM_CS_PIN, LOW);
		SPI.transfer16((0x02 << 8) | (addr >> 16));
		SPI.transfer16(addr & 0xFFFF);
		while (count) {
			int16_t w = 0;
			if (data) w = *data++;
			SPI.transfer16(w);
			count--;
		}
		digitalWriteFast(SPIRAM_CS_PIN, HIGH);
		SPI.endTransaction();	
	} else if (memory_type == AUDIO_MEMORY_MEMORYBOARD) {		
		SPI.beginTransaction(SPISETTING);
		while (count) {
			uint32_t chip = (addr >> 16) + 1;
			digitalWriteFast(MEMBOARD_CS0_PIN, chip & 1);
			digitalWriteFast(MEMBOARD_CS1_PIN, chip & 2);
			digitalWriteFast(MEMBOARD_CS2_PIN, chip & 4);
			uint32_t chipaddr = (addr & 0xFFFF) << 1;
			SPI.transfer16((0x02 << 8) | (chipaddr >> 16));
			SPI.transfer16(chipaddr & 0xFFFF);
			uint32_t num = 0x10000 - (addr & 0xFFFF);
			if (num > count) num = count;
			count -= num;
			addr += num;
			do {
				int16_t w = 0;
				if (data) w = *data++;
				SPI.transfer16(w);
			} while (--num > 0);
		}
		digitalWriteFast(MEMBOARD_CS0_PIN, LOW);
		digitalWriteFast(MEMBOARD_CS1_PIN, LOW);
		digitalWriteFast(MEMBOARD_CS2_PIN, LOW);
		SPI.endTransaction();
	}
#endif
}

