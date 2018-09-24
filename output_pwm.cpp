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
#include "output_pwm.h"

audio_block_t * AudioOutputPWM::block_1st = NULL;
audio_block_t * AudioOutputPWM::block_2nd = NULL;
uint32_t  AudioOutputPWM::block_offset = 0;
bool AudioOutputPWM::update_responsibility = false;
uint8_t AudioOutputPWM::interrupt_count = 0;

DMAMEM uint32_t pwm_dma_buffer[AUDIO_BLOCK_SAMPLES*2];
DMAChannel AudioOutputPWM::dma(false);

// TODO: this code assumes F_BUS is 48 MHz.
// supporting other speeds is not easy, but should be done someday

#if defined(KINETISK)

void AudioOutputPWM::begin(void)
{
	dma.begin(true); // Allocate the DMA channel first

	//Serial.println("AudioPwmOutput constructor");
	block_1st = NULL;
	FTM1_SC = 0;
	FTM1_CNT = 0;
	FTM1_MOD = 543;
	FTM1_C0SC = 0x69;  // send DMA request on match
	FTM1_C1SC = 0x28;
	FTM1_SC = FTM_SC_CLKS(1) | FTM_SC_PS(0);
	CORE_PIN3_CONFIG = PORT_PCR_MUX(3) | PORT_PCR_DSE | PORT_PCR_SRE;
	CORE_PIN4_CONFIG = PORT_PCR_MUX(3) | PORT_PCR_DSE | PORT_PCR_SRE;
	FTM1_C0V = 120; // range 120 to 375
	FTM1_C1V = 0;   // range 0 to 255
	for (int i=0; i<(AUDIO_BLOCK_SAMPLES*2); i+=2) {
		pwm_dma_buffer[i] = 120; // zero must not be used
		pwm_dma_buffer[i+1] = 0;
	}
	dma.TCD->SADDR = pwm_dma_buffer;
	dma.TCD->SOFF = 4;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2)
		| DMA_TCD_ATTR_DSIZE(2) | DMA_TCD_ATTR_DMOD(4);
	dma.TCD->NBYTES_MLNO = 8;
	dma.TCD->SLAST = -sizeof(pwm_dma_buffer);
	dma.TCD->DADDR = &FTM1_C0V;
	dma.TCD->DOFF = 8;
	dma.TCD->CITER_ELINKNO = sizeof(pwm_dma_buffer) / 8;
	dma.TCD->DLASTSGA = 0;
	dma.TCD->BITER_ELINKNO = sizeof(pwm_dma_buffer) / 8;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM1_CH0);
	dma.enable();
	update_responsibility = update_setup();
	dma.attachInterrupt(isr);
}

void AudioOutputPWM::update(void)
{
	audio_block_t *block;
	block = receiveReadOnly();
	if (!block) return;
	__disable_irq();
	if (block_1st == NULL) {
		block_1st = block;
		block_offset = 0;
		__enable_irq();
	} else if (block_2nd == NULL) {
		block_2nd = block;
		__enable_irq();
	} else {
		audio_block_t *tmp = block_1st;
		block_1st = block_2nd;
		block_2nd = block;
		block_offset = 0;
		__enable_irq();
		release(tmp);
	}
}

void AudioOutputPWM::isr(void)
{
	int16_t *src;
	uint32_t *dest;
	audio_block_t *block;
	uint32_t saddr, offset;

	saddr = (uint32_t)(dma.TCD->SADDR);
	dma.clearInterrupt();
	if (saddr < (uint32_t)pwm_dma_buffer + sizeof(pwm_dma_buffer) / 2) {
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		dest = &pwm_dma_buffer[AUDIO_BLOCK_SAMPLES];
	} else {
		// DMA is transmitting the second half of the buffer
		// so we must fill the first half
		dest = pwm_dma_buffer;
	}
	block = AudioOutputPWM::block_1st;
	offset = AudioOutputPWM::block_offset;

	if (block) {
		src = &block->data[offset];
		for (int i=0; i < AUDIO_BLOCK_SAMPLES/4; i++) {
			uint16_t sample = *src++ + 0x8000;
			uint32_t msb = ((sample >> 8) & 255) + 120;
			uint32_t lsb = sample & 255;
			*dest++ = msb;
			*dest++ = lsb;
			*dest++ = msb;
			*dest++ = lsb;
		}
		offset += AUDIO_BLOCK_SAMPLES/4;
		if (offset < AUDIO_BLOCK_SAMPLES) {
			AudioOutputPWM::block_offset = offset;
		} else {
			AudioOutputPWM::block_offset = 0;
			AudioStream::release(block);
			AudioOutputPWM::block_1st = AudioOutputPWM::block_2nd;
			AudioOutputPWM::block_2nd = NULL;
		}
	} else {
		// fill with silence when no data available
		for (int i=0; i < AUDIO_BLOCK_SAMPLES/4; i++) {
			*dest++ = 248;
			*dest++ = 0;
			*dest++ = 248;
			*dest++ = 0;
		}
	}
	if (AudioOutputPWM::update_responsibility) {
		if (++AudioOutputPWM::interrupt_count >= 4) {
			AudioOutputPWM::interrupt_count = 0;
			AudioStream::update_all();
		}
	}
}




// DMA target is: (registers require 32 bit writes)
//  40039010 Channel 0 Value (FTM1_C0V)
//  40039018 Channel 1 Value (FTM1_C1V)

// TCD:
//  source address = buffer address
//  source offset = 4 bytes
//  attr = no src mod, ssize = 32 bit, dest mod = 16 bytes (4), dsize = 32 bit
//  minor loop byte count = 8
//  source last adjust = -sizeof(buffer)
//  dest address = FTM1_C0V
//  dest address offset = 8
//  citer = sizeof(buffer) / 8    (no minor loop linking)
//  dest last adjust = 0          (dest modulo keeps it ready for more)
//  control:
//    throttling = 0
//    major link to same channel
//    done = 0
//    active = 0
//    majorlink = 1
//    scatter/gather = 0
//    disable request = 0
//    inthalf = 1
//    intmajor = 1
//    start = 0
//  biter = sizeof(buffer) / 8   (no minor loop linking)


#elif defined(KINETISL)

void AudioOutputPWM::update(void)
{
	audio_block_t *block;
	block = receiveReadOnly();
	if (block) release(block);
}

#endif
