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
#include "output_i2s_hex.h"
#include "output_i2s.h"
#include "memcpy_audio.h"

#if defined(__IMXRT1062__)
#include "utility/imxrt_hw.h"

audio_block_t * AudioOutputI2SHex::block_ch1_1st = NULL;
audio_block_t * AudioOutputI2SHex::block_ch2_1st = NULL;
audio_block_t * AudioOutputI2SHex::block_ch3_1st = NULL;
audio_block_t * AudioOutputI2SHex::block_ch4_1st = NULL;
audio_block_t * AudioOutputI2SHex::block_ch5_1st = NULL;
audio_block_t * AudioOutputI2SHex::block_ch6_1st = NULL;
audio_block_t * AudioOutputI2SHex::block_ch1_2nd = NULL;
audio_block_t * AudioOutputI2SHex::block_ch2_2nd = NULL;
audio_block_t * AudioOutputI2SHex::block_ch3_2nd = NULL;
audio_block_t * AudioOutputI2SHex::block_ch4_2nd = NULL;
audio_block_t * AudioOutputI2SHex::block_ch5_2nd = NULL;
audio_block_t * AudioOutputI2SHex::block_ch6_2nd = NULL;
uint16_t  AudioOutputI2SHex::ch1_offset = 0;
uint16_t  AudioOutputI2SHex::ch2_offset = 0;
uint16_t  AudioOutputI2SHex::ch3_offset = 0;
uint16_t  AudioOutputI2SHex::ch4_offset = 0;
uint16_t  AudioOutputI2SHex::ch5_offset = 0;
uint16_t  AudioOutputI2SHex::ch6_offset = 0;
bool AudioOutputI2SHex::update_responsibility = false;
DMAMEM __attribute__((aligned(32))) static uint32_t i2s_tx_buffer[AUDIO_BLOCK_SAMPLES*3];
DMAChannel AudioOutputI2SHex::dma(false);

static const uint32_t zerodata[AUDIO_BLOCK_SAMPLES/4] = {0};

void AudioOutputI2SHex::begin(void)
{
	dma.begin(true); // Allocate the DMA channel first

	block_ch1_1st = NULL;
	block_ch2_1st = NULL;
	block_ch3_1st = NULL;
	block_ch4_1st = NULL;
	block_ch5_1st = NULL;
	block_ch6_1st = NULL;

	const int pinoffset = 0; // TODO: make this configurable...
	memset(i2s_tx_buffer, 0, sizeof(i2s_tx_buffer));
	AudioOutputI2S::config_i2s();
	I2S1_TCR3 = I2S_TCR3_TCE_3CH << pinoffset;
	switch (pinoffset) {
	  case 0:
		CORE_PIN7_CONFIG  = 3;
		CORE_PIN32_CONFIG = 3;
		CORE_PIN9_CONFIG  = 3;
		break;
	  case 1:
		CORE_PIN32_CONFIG = 3;
		CORE_PIN9_CONFIG  = 3;
		CORE_PIN6_CONFIG  = 3;
	}
	dma.TCD->SADDR = i2s_tx_buffer;
	dma.TCD->SOFF = 2;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	dma.TCD->NBYTES_MLOFFYES = DMA_TCD_NBYTES_DMLOE |
		DMA_TCD_NBYTES_MLOFFYES_MLOFF(-12) |
		DMA_TCD_NBYTES_MLOFFYES_NBYTES(6);
	dma.TCD->SLAST = -sizeof(i2s_tx_buffer);
	dma.TCD->DADDR = (void *)((uint32_t)&I2S1_TDR0 + 2 + pinoffset * 4);
	dma.TCD->DOFF = 4;
	dma.TCD->CITER_ELINKNO = AUDIO_BLOCK_SAMPLES * 2;
	dma.TCD->DLASTSGA = -12;
	dma.TCD->BITER_ELINKNO = AUDIO_BLOCK_SAMPLES * 2;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI1_TX);
	dma.enable();
	I2S1_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE;
	I2S1_TCSR = I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;
	I2S1_TCR3 = I2S_TCR3_TCE_3CH << pinoffset;
	update_responsibility = update_setup();
	dma.attachInterrupt(isr);
}

void AudioOutputI2SHex::isr(void)
{
	uint32_t saddr;
	const int16_t *src1, *src2, *src3, *src4, *src5, *src6;
	const int16_t *zeros = (const int16_t *)zerodata;
	int16_t *dest;

	saddr = (uint32_t)(dma.TCD->SADDR);
	dma.clearInterrupt();
	if (saddr < (uint32_t)i2s_tx_buffer + sizeof(i2s_tx_buffer) / 2) {
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		dest = (int16_t *)((uint32_t)i2s_tx_buffer + sizeof(i2s_tx_buffer) / 2);
		if (update_responsibility) update_all();
	} else {
		dest = (int16_t *)i2s_tx_buffer;
	}

	src1 = (block_ch1_1st) ? block_ch1_1st->data + ch1_offset : zeros;
	src2 = (block_ch2_1st) ? block_ch2_1st->data + ch2_offset : zeros;
	src3 = (block_ch3_1st) ? block_ch3_1st->data + ch3_offset : zeros;
	src4 = (block_ch4_1st) ? block_ch4_1st->data + ch4_offset : zeros;
	src5 = (block_ch5_1st) ? block_ch5_1st->data + ch5_offset : zeros;
	src6 = (block_ch6_1st) ? block_ch6_1st->data + ch6_offset : zeros;
#if 0
	// TODO: optimized 6 channel copy...
	memcpy_tointerleaveQuad(dest, src1, src2, src3, src4);
#else
	int16_t *p=dest;
	for (int i=0; i < AUDIO_BLOCK_SAMPLES/2; i++) {
		*p++ = *src1++;
		*p++ = *src3++;
		*p++ = *src5++;
		*p++ = *src2++;
		*p++ = *src4++;
		*p++ = *src6++;
	}
#endif
	arm_dcache_flush_delete(dest, sizeof(i2s_tx_buffer) / 2);

	if (block_ch1_1st) {
		if (ch1_offset == 0) {
			ch1_offset = AUDIO_BLOCK_SAMPLES/2;
		} else {
			ch1_offset = 0;
			release(block_ch1_1st);
			block_ch1_1st = block_ch1_2nd;
			block_ch1_2nd = NULL;
		}
	}
	if (block_ch2_1st) {
		if (ch2_offset == 0) {
			ch2_offset = AUDIO_BLOCK_SAMPLES/2;
		} else {
			ch2_offset = 0;
			release(block_ch2_1st);
			block_ch2_1st = block_ch2_2nd;
			block_ch2_2nd = NULL;
		}
	}
	if (block_ch3_1st) {
		if (ch3_offset == 0) {
			ch3_offset = AUDIO_BLOCK_SAMPLES/2;
		} else {
			ch3_offset = 0;
			release(block_ch3_1st);
			block_ch3_1st = block_ch3_2nd;
			block_ch3_2nd = NULL;
		}
	}
	if (block_ch4_1st) {
		if (ch4_offset == 0) {
			ch4_offset = AUDIO_BLOCK_SAMPLES/2;
		} else {
			ch4_offset = 0;
			release(block_ch4_1st);
			block_ch4_1st = block_ch4_2nd;
			block_ch4_2nd = NULL;
		}
	}
	if (block_ch5_1st) {
		if (ch5_offset == 0) {
			ch5_offset = AUDIO_BLOCK_SAMPLES/2;
		} else {
			ch5_offset = 0;
			release(block_ch5_1st);
			block_ch5_1st = block_ch5_2nd;
			block_ch5_2nd = NULL;
		}
	}
	if (block_ch6_1st) {
		if (ch6_offset == 0) {
			ch6_offset = AUDIO_BLOCK_SAMPLES/2;
		} else {
			ch6_offset = 0;
			release(block_ch6_1st);
			block_ch6_1st = block_ch6_2nd;
			block_ch6_2nd = NULL;
		}
	}
}


void AudioOutputI2SHex::update(void)
{
	audio_block_t *block, *tmp;

	block = receiveReadOnly(0); // channel 1
	if (block) {
		__disable_irq();
		if (block_ch1_1st == NULL) {
			block_ch1_1st = block;
			ch1_offset = 0;
			__enable_irq();
		} else if (block_ch1_2nd == NULL) {
			block_ch1_2nd = block;
			__enable_irq();
		} else {
			tmp = block_ch1_1st;
			block_ch1_1st = block_ch1_2nd;
			block_ch1_2nd = block;
			ch1_offset = 0;
			__enable_irq();
			release(tmp);
		}
	}
	block = receiveReadOnly(1); // channel 2
	if (block) {
		__disable_irq();
		if (block_ch2_1st == NULL) {
			block_ch2_1st = block;
			ch2_offset = 0;
			__enable_irq();
		} else if (block_ch2_2nd == NULL) {
			block_ch2_2nd = block;
			__enable_irq();
		} else {
			tmp = block_ch2_1st;
			block_ch2_1st = block_ch2_2nd;
			block_ch2_2nd = block;
			ch2_offset = 0;
			__enable_irq();
			release(tmp);
		}
	}
	block = receiveReadOnly(2); // channel 3
	if (block) {
		__disable_irq();
		if (block_ch3_1st == NULL) {
			block_ch3_1st = block;
			ch3_offset = 0;
			__enable_irq();
		} else if (block_ch3_2nd == NULL) {
			block_ch3_2nd = block;
			__enable_irq();
		} else {
			tmp = block_ch3_1st;
			block_ch3_1st = block_ch3_2nd;
			block_ch3_2nd = block;
			ch3_offset = 0;
			__enable_irq();
			release(tmp);
		}
	}
	block = receiveReadOnly(3); // channel 4
	if (block) {
		__disable_irq();
		if (block_ch4_1st == NULL) {
			block_ch4_1st = block;
			ch4_offset = 0;
			__enable_irq();
		} else if (block_ch4_2nd == NULL) {
			block_ch4_2nd = block;
			__enable_irq();
		} else {
			tmp = block_ch4_1st;
			block_ch4_1st = block_ch4_2nd;
			block_ch4_2nd = block;
			ch4_offset = 0;
			__enable_irq();
			release(tmp);
		}
	}
	block = receiveReadOnly(4); // channel 5
	if (block) {
		__disable_irq();
		if (block_ch5_1st == NULL) {
			block_ch5_1st = block;
			ch5_offset = 0;
			__enable_irq();
		} else if (block_ch5_2nd == NULL) {
			block_ch5_2nd = block;
			__enable_irq();
		} else {
			tmp = block_ch5_1st;
			block_ch5_1st = block_ch5_2nd;
			block_ch5_2nd = block;
			ch5_offset = 0;
			__enable_irq();
			release(tmp);
		}
	}
	block = receiveReadOnly(5); // channel 6
	if (block) {
		__disable_irq();
		if (block_ch6_1st == NULL) {
			block_ch6_1st = block;
			ch6_offset = 0;
			__enable_irq();
		} else if (block_ch6_2nd == NULL) {
			block_ch6_2nd = block;
			__enable_irq();
		} else {
			tmp = block_ch6_1st;
			block_ch6_1st = block_ch6_2nd;
			block_ch6_2nd = block;
			ch6_offset = 0;
			__enable_irq();
			release(tmp);
		}
	}
}


#else // not supported

void AudioOutputI2SHex::begin(void)
{
}

void AudioOutputI2SHex::update(void)
{
	audio_block_t *block;

	block = receiveReadOnly(0);
	if (block) release(block);
	block = receiveReadOnly(1);
	if (block) release(block);
	block = receiveReadOnly(2);
	if (block) release(block);
	block = receiveReadOnly(3);
	if (block) release(block);
	block = receiveReadOnly(4);
	if (block) release(block);
	block = receiveReadOnly(5);
	if (block) release(block);
}

#endif
