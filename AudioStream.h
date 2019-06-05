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

#ifndef AudioStream_h
#define AudioStream_h

#ifndef __ASSEMBLER__
#include <stdio.h>  // for NULL
#include <string.h> // for memcpy
#if defined(__SAMD51__)
#include "sam.h"

#define NVIC_ENABLE_IRQ NVIC_EnableIRQ
#define NVIC_DISABLE_IRQ NVIC_DisableIRQ
#define IRQ_SOFTWARE SOFTWARE_IRQn
#define NVIC_SET_PRIORITY NVIC_SetPriority

//we're gonna commandeer this kinda useless IQRn and use it as a software interrupt 
#define SOFTWARE_IRQn EVSYS_4_IRQn
#define SOFTWARE_Handler EVSYS_4_Handler

//TODO: lets try to find an unused timer instead of hardcoding
#if defined(ADAFRUIT_PYBADGE_M4_EXPRESS) || defined(ADAFRUIT_PYGAMER_M4_EXPRESS) || defined(ADAFRUIT_PYGAMER_ADVANCE_M4_EXPRESS)
// TC2 is backlight, TC3 is Tone()
  #define AUDIO_TC TC5
  #define AUDIO_IRQn TC5_IRQn
  #define AUDIO_Handler TC5_Handler
  #define AUDIO_GCLK_ID TC5_GCLK_ID
  #define AUDIO_TC_DMAC_ID_OVF TC5_DMAC_ID_OVF
#else
  #define AUDIO_TC TC2
  #define AUDIO_IRQn TC2_IRQn
  #define AUDIO_Handler TC2_Handler
  #define AUDIO_GCLK_ID TC2_GCLK_ID
  #define AUDIO_TC_DMAC_ID_OVF TC2_DMAC_ID_OVF
#endif

#define WAIT_TC8_REGS_SYNC(x) while(x->COUNT8.SYNCBUSY.bit.ENABLE || x->COUNT8.SYNCBUSY.bit.SWRST);

//TODO: remove
#define DMAMEM

#else
#include "kinetis.h"
#endif
#endif

// AUDIO_BLOCK_SAMPLES determines how many samples the audio library processes
// per update.  It may be reduced to achieve lower latency response to events,
// at the expense of higher interrupt and DMA setup overhead.
//
// Less than 32 may not work with some input & output objects.  Multiples of 16
// should be used, since some synthesis objects generate 16 samples per loop.
//
// Some parts of the audio library may have hard-coded dependency on 128 samples.
// Please report these on the forum with reproducible test cases.

#ifndef AUDIO_BLOCK_SAMPLES
#if defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)
#define AUDIO_BLOCK_SAMPLES  128
#elif defined(__MKL26Z64__)
#define AUDIO_BLOCK_SAMPLES  64
#elif defined(__SAMD51__)
#define AUDIO_BLOCK_SAMPLES  128
#endif
#endif

#ifndef AUDIO_SAMPLE_RATE_EXACT
#if defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)
#define AUDIO_SAMPLE_RATE_EXACT 44117.64706 // 48 MHz / 1088, or 96 MHz * 2 / 17 / 256
#elif defined(__MKL26Z64__)
#define AUDIO_SAMPLE_RATE_EXACT 22058.82353 // 48 MHz / 2176, or 96 MHz * 1 / 17 / 256
#elif defined(__SAMD51__)

//#define AUDIO_CLKRATE (SystemCoreClock >> 6)
//#define AUDIO_PRESCALER TC_CTRLA_PRESCALER_DIV64
//#define AUDIO_SAMPLE_RATE_EXACT 22058.82353 // 120 MHz / 64 / 85 
//#define AUDIO_TC_FREQ 22000

#define AUDIO_CLKRATE (VARIANT_GCLK2_FREQ >> 4)
#define AUDIO_PRESCALER TC_CTRLA_PRESCALER_DIV16
#define AUDIO_SAMPLE_RATE_EXACT 44014.085 // 100 MHz / 16 / 142
#define AUDIO_TC_FREQ 44100

#endif // SAMD51
#endif // AUDIO_SAMPLE_RATE_EXACT

#define AUDIO_SAMPLE_RATE AUDIO_SAMPLE_RATE_EXACT

#ifndef __ASSEMBLER__
class AudioStream;
class AudioConnection;

typedef struct audio_block_struct {
	uint8_t  ref_count;
	uint8_t  reserved1;
	uint16_t memory_pool_index;
	int16_t  data[AUDIO_BLOCK_SAMPLES];
} audio_block_t;


class AudioConnection
{
public:
	AudioConnection(AudioStream &source, AudioStream &destination) :
		src(source), dst(destination), src_index(0), dest_index(0),
		next_dest(NULL)
		{ connect(); }
	AudioConnection(AudioStream &source, unsigned char sourceOutput,
		AudioStream &destination, unsigned char destinationInput) :
		src(source), dst(destination),
		src_index(sourceOutput), dest_index(destinationInput),
		next_dest(NULL)
		{ connect(); }
	friend class AudioStream;
protected:
	void connect(void);
	AudioStream &src;
	AudioStream &dst;
	unsigned char src_index;
	unsigned char dest_index;
	AudioConnection *next_dest;
};


#define AudioMemory(num) ({ \
	static DMAMEM audio_block_t data[num]; \
	AudioStream::initialize_memory(data, num); \
})

#define CYCLE_COUNTER_APPROX_PERCENT(n) (((n) + (F_CPU / 32 / AUDIO_SAMPLE_RATE * AUDIO_BLOCK_SAMPLES / 100)) / (F_CPU / 16 / AUDIO_SAMPLE_RATE * AUDIO_BLOCK_SAMPLES / 100))

#define AudioProcessorUsage() (CYCLE_COUNTER_APPROX_PERCENT(AudioStream::cpu_cycles_total))
#define AudioProcessorUsageMax() (CYCLE_COUNTER_APPROX_PERCENT(AudioStream::cpu_cycles_total_max))
#define AudioProcessorUsageMaxReset() (AudioStream::cpu_cycles_total_max = AudioStream::cpu_cycles_total)
#define AudioMemoryUsage() (AudioStream::memory_used)
#define AudioMemoryUsageMax() (AudioStream::memory_used_max)
#define AudioMemoryUsageMaxReset() (AudioStream::memory_used_max = AudioStream::memory_used)

class AudioStream
{
public:
	AudioStream(unsigned char ninput, audio_block_t **iqueue) :
		num_inputs(ninput), inputQueue(iqueue) {
			active = false;
			destination_list = NULL;
			for (int i=0; i < num_inputs; i++) {
				inputQueue[i] = NULL;
			}
			// add to a simple list, for update_all
			// TODO: replace with a proper data flow analysis in update_all
			if (first_update == NULL) {
				first_update = this;
			} else {
				AudioStream *p;
				for (p=first_update; p->next_update; p = p->next_update) ;
				p->next_update = this;
			}
			next_update = NULL;
			cpu_cycles = 0;
			cpu_cycles_max = 0;
		}
	static void initialize_memory(audio_block_t *data, unsigned int num);
	int processorUsage(void) { return CYCLE_COUNTER_APPROX_PERCENT(cpu_cycles); }
	int processorUsageMax(void) { return CYCLE_COUNTER_APPROX_PERCENT(cpu_cycles_max); }
	void processorUsageMaxReset(void) { cpu_cycles_max = cpu_cycles; }
	uint16_t cpu_cycles;
	uint16_t cpu_cycles_max;
	static uint16_t cpu_cycles_total;
	static uint16_t cpu_cycles_total_max;
	static uint16_t memory_used;
	static uint16_t memory_used_max;
protected:
	bool active;
	unsigned char num_inputs;
	static audio_block_t * allocate(void);
	static void release(audio_block_t * block);
	void transmit(audio_block_t *block, unsigned char index = 0);
	audio_block_t * receiveReadOnly(unsigned int index = 0);
	audio_block_t * receiveWritable(unsigned int index = 0);
	static bool update_setup(void);
	static void update_stop(void);
#if defined(__SAMD51__)
	static void update_all(void) { NVIC_SetPendingIRQ(SOFTWARE_IRQn); }
#else
	static void update_all(void) { NVIC_SET_PENDING(IRQ_SOFTWARE); }
#endif
	friend void software_isr(void);
	friend class AudioConnection;
private:
	AudioConnection *destination_list;
	audio_block_t **inputQueue;
	static bool update_scheduled;
	virtual void update(void) = 0;
	static AudioStream *first_update; // for update_all
	AudioStream *next_update; // for update_all
	static audio_block_t *memory_pool;
	static uint32_t memory_pool_available_mask[];
	static uint16_t memory_pool_first_mask;
};

#endif
#endif
