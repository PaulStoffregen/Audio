/* Teensyduino Core Library
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2017 PJRC.COM, LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * 2. If the Software is incorporated into a build system that allows
 * selection among a list of target devices, then similar target
 * devices manufactured by PJRC.COM must be included in the list of
 * target devices and selectable in the same manner.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include <string.h> // for memcpy
#include "AudioStream.h"
#include <Arduino.h>

#if defined(__MKL26Z64__)
  #define MAX_AUDIO_MEMORY 6144
#elif defined(__MK20DX128__)
  #define MAX_AUDIO_MEMORY 12288
#elif defined(__MK20DX256__)
  #define MAX_AUDIO_MEMORY 49152
#elif defined(__MK64FX512__)
  #define MAX_AUDIO_MEMORY 163840
#elif defined(__MK66FX1M0__)
  #define MAX_AUDIO_MEMORY 229376
#elif defined(__SAMD51__)
  #define MAX_AUDIO_MEMORY 229376
#endif

#define NUM_MASKS  (((MAX_AUDIO_MEMORY / AUDIO_BLOCK_SAMPLES / 2) + 31) / 32)

audio_block_t * AudioStream::memory_pool;
uint32_t AudioStream::memory_pool_available_mask[NUM_MASKS];
uint16_t AudioStream::memory_pool_first_mask;

uint16_t AudioStream::cpu_cycles_total = 0;
uint16_t AudioStream::cpu_cycles_total_max = 0;
uint16_t AudioStream::memory_used = 0;
uint16_t AudioStream::memory_used_max = 0;




// Set up the pool of audio data blocks
// placing them all onto the free list
void AudioStream::initialize_memory(audio_block_t *data, unsigned int num)
{
	unsigned int i;
	unsigned int maxnum = MAX_AUDIO_MEMORY / AUDIO_BLOCK_SAMPLES / 2;

	//Serial.println("AudioStream initialize_memory");
	//delay(10);
	if (num > maxnum) num = maxnum;
	__disable_irq();
	memory_pool = data;
	memory_pool_first_mask = 0;
	for (i=0; i < NUM_MASKS; i++) {
		memory_pool_available_mask[i] = 0;
	}
	for (i=0; i < num; i++) {
		memory_pool_available_mask[i >> 5] |= (1 << (i & 0x1F));
	}
	for (i=0; i < num; i++) {
		data[i].memory_pool_index = i;
	}
	__enable_irq();

}

// Allocate 1 audio data block.  If successful
// the caller is the only owner of this new block
audio_block_t * AudioStream::allocate(void)
{
	uint32_t n, index, avail;
	uint32_t *p, *end;
	audio_block_t *block;
	uint32_t used;

	p = memory_pool_available_mask;
	end = p + NUM_MASKS;
	__disable_irq();
	index = memory_pool_first_mask;
	p += index;
	while (1) {
		if (p >= end) {
			__enable_irq();
			//Serial.println("alloc:null");
			return NULL;
		}
		avail = *p;
		if (avail) break;
		index++;
		p++;
	}
	n = __builtin_clz(avail);
	avail &= ~(0x80000000 >> n);
	*p = avail;
	if (!avail) index++;
	memory_pool_first_mask = index;
	used = memory_used + 1;
	memory_used = used;
	__enable_irq();
	index = p - memory_pool_available_mask;
	block = memory_pool + ((index << 5) + (31 - n));
	block->ref_count = 1;
	if (used > memory_used_max) memory_used_max = used;
	//Serial.print("alloc:");
	//Serial.println((uint32_t)block, HEX);
	return block;
}

// Release ownership of a data block.  If no
// other streams have ownership, the block is
// returned to the free pool
void AudioStream::release(audio_block_t *block)
{
	//if (block == NULL) return;
	uint32_t mask = (0x80000000 >> (31 - (block->memory_pool_index & 0x1F)));
	uint32_t index = block->memory_pool_index >> 5;

	__disable_irq();
	if (block->ref_count > 1) {
		block->ref_count--;
	} else {
		//Serial.print("reles:");
		//Serial.println((uint32_t)block, HEX);
		memory_pool_available_mask[index] |= mask;
		if (index < memory_pool_first_mask) memory_pool_first_mask = index;
		memory_used--;
	}
	__enable_irq();
}

// Transmit an audio data block
// to all streams that connect to an output.  The block
// becomes owned by all the recepients, but also is still
// owned by this object.  Normally, a block must be released
// by the caller after it's transmitted.  This allows the
// caller to transmit to same block to more than 1 output,
// and then release it once after all transmit calls.
void AudioStream::transmit(audio_block_t *block, unsigned char index)
{
	for (AudioConnection *c = destination_list; c != NULL; c = c->next_dest) {
		if (c->src_index == index) {
			if (c->dst.inputQueue[c->dest_index] == NULL) {
				c->dst.inputQueue[c->dest_index] = block;
				block->ref_count++;
			}
		}
	}
}


// Receive block from an input.  The block's data
// may be shared with other streams, so it must not be written
audio_block_t * AudioStream::receiveReadOnly(unsigned int index)
{
	audio_block_t *in;

	if (index >= num_inputs) return NULL;
	in = inputQueue[index];
	inputQueue[index] = NULL;
	return in;
}

// Receive block from an input.  The block will not
// be shared, so its contents may be changed.
audio_block_t * AudioStream::receiveWritable(unsigned int index)
{
	audio_block_t *in, *p;

	if (index >= num_inputs) return NULL;
	in = inputQueue[index];
	inputQueue[index] = NULL;
	if (in && in->ref_count > 1) {
		p = allocate();
		if (p) memcpy(p->data, in->data, sizeof(p->data));
		in->ref_count--;
		in = p;
	}
	return in;
}


void AudioConnection::connect(void)
{
	AudioConnection *p;

	if (dest_index > dst.num_inputs) return;
	__disable_irq();
	p = src.destination_list;
	if (p == NULL) {
		src.destination_list = this;
	} else {
		while (p->next_dest) p = p->next_dest;
		p->next_dest = this;
	}
	src.active = true;
	dst.active = true;
	__enable_irq();
}



// When an object has taken responsibility for calling update_all()
// at each block interval (approx 2.9ms), this variable is set to
// true.  Objects that are capable of calling update_all(), typically
// input and output based on interrupts, must check this variable in
// their constructors.
bool AudioStream::update_scheduled = false;

bool AudioStream::update_setup(void)
{
	if (update_scheduled) return false;
	NVIC_SET_PRIORITY(IRQ_SOFTWARE, 208); // 255 = lowest priority
	NVIC_ENABLE_IRQ(IRQ_SOFTWARE);
	update_scheduled = true;
	return true;
}

void AudioStream::update_stop(void)
{
	NVIC_DISABLE_IRQ(IRQ_SOFTWARE);
	update_scheduled = false;
}

AudioStream * AudioStream::first_update = NULL;

void software_isr(void) // AudioStream::update_all()
{
	AudioStream *p;

#if defined(ARDUINO_ARCH_SAMD)
//TODO: this
	uint32_t totalcycles = 0;
#else
	ARM_DEMCR |= ARM_DEMCR_TRCENA;
	ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
	uint32_t totalcycles = ARM_DWT_CYCCNT;
#endif
	//digitalWriteFast(2, HIGH);
	for (p = AudioStream::first_update; p; p = p->next_update) {
		if (p->active) {
#if defined(ARDUINO_ARCH_SAMD)
//TODO: this
			uint32_t cycles = 0;
#else
			uint32_t cycles = ARM_DWT_CYCCNT;
#endif
			p->update();
			// TODO: traverse inputQueueArray and release
			// any input blocks that weren't consumed?
#if defined(ARDUINO_ARCH_SAMD)
//TODO: this
#else
			cycles = (ARM_DWT_CYCCNT - cycles) >> 4;
#endif
			p->cpu_cycles = cycles;
			if (cycles > p->cpu_cycles_max) p->cpu_cycles_max = cycles;
		}
	}
	//digitalWriteFast(2, LOW);
#if defined(ARDUINO_ARCH_SAMD)
//TODO: this
#else
	totalcycles = (ARM_DWT_CYCCNT - totalcycles) >> 4;;
#endif
	AudioStream::cpu_cycles_total = totalcycles;
	if (totalcycles > AudioStream::cpu_cycles_total_max)
		AudioStream::cpu_cycles_total_max = totalcycles;
}

#if defined(__SAMD51__)

extern "C" {

void SOFTWARE_Handler(void)
{
	software_isr();
}

};

#endif
