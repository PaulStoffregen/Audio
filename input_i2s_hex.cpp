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
#include "input_i2s_hex.h"
#include "output_i2s.h"

DMAMEM __attribute__((aligned(32))) static uint32_t i2s_rx_buffer[AUDIO_BLOCK_SAMPLES*3];
audio_block_t * AudioInputI2SHex::block_ch1 = NULL;
audio_block_t * AudioInputI2SHex::block_ch2 = NULL;
audio_block_t * AudioInputI2SHex::block_ch3 = NULL;
audio_block_t * AudioInputI2SHex::block_ch4 = NULL;
audio_block_t * AudioInputI2SHex::block_ch5 = NULL;
audio_block_t * AudioInputI2SHex::block_ch6 = NULL;
uint16_t AudioInputI2SHex::block_offset = 0;
bool AudioInputI2SHex::update_responsibility = false;
DMAChannel AudioInputI2SHex::dma(false);

#if defined(__IMXRT1062__)

void AudioInputI2SHex::begin(void)
{
	dma.begin(true); // Allocate the DMA channel first

	const int pinoffset = 0; // TODO: make this configurable...
	AudioOutputI2S::config_i2s();
	I2S1_RCR3 = I2S_RCR3_RCE_3CH << pinoffset;
	switch (pinoffset) {
	  case 0:
		CORE_PIN8_CONFIG = 3;
		CORE_PIN6_CONFIG = 3;
		CORE_PIN9_CONFIG = 3;
		IOMUXC_SAI1_RX_DATA0_SELECT_INPUT = 2; // GPIO_B1_00_ALT3, pg 873
		IOMUXC_SAI1_RX_DATA1_SELECT_INPUT = 1; // GPIO_B0_10_ALT3, pg 873
		IOMUXC_SAI1_RX_DATA2_SELECT_INPUT = 1; // GPIO_B0_11_ALT3, pg 874
		break;
	  case 1:
		CORE_PIN6_CONFIG = 3;
		CORE_PIN9_CONFIG = 3;
		CORE_PIN32_CONFIG = 3;
		IOMUXC_SAI1_RX_DATA1_SELECT_INPUT = 1; // GPIO_B0_10_ALT3, pg 873
		IOMUXC_SAI1_RX_DATA2_SELECT_INPUT = 1; // GPIO_B0_11_ALT3, pg 874
		IOMUXC_SAI1_RX_DATA3_SELECT_INPUT = 1; // GPIO_B0_12_ALT3, pg 875
		break;
	}
	dma.TCD->SADDR = (void *)((uint32_t)&I2S1_RDR0 + 2 + pinoffset * 4);
	dma.TCD->SOFF = 4;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	dma.TCD->NBYTES_MLOFFYES = DMA_TCD_NBYTES_SMLOE |
		DMA_TCD_NBYTES_MLOFFYES_MLOFF(-12) |
		DMA_TCD_NBYTES_MLOFFYES_NBYTES(6);
	dma.TCD->SLAST = -12;
	dma.TCD->DADDR = i2s_rx_buffer;
	dma.TCD->DOFF = 2;
	dma.TCD->CITER_ELINKNO = AUDIO_BLOCK_SAMPLES * 2;
	dma.TCD->DLASTSGA = -sizeof(i2s_rx_buffer);
	dma.TCD->BITER_ELINKNO = AUDIO_BLOCK_SAMPLES * 2;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI1_RX);

	//I2S1_RCSR = 0;
	//I2S1_RCR3 = I2S_RCR3_RCE_2CH << pinoffset;
	I2S1_RCSR = I2S_RCSR_RE | I2S_RCSR_BCE | I2S_RCSR_FRDE | I2S_RCSR_FR;
	update_responsibility = update_setup();
	dma.enable();
	dma.attachInterrupt(isr);
}

void AudioInputI2SHex::isr(void)
{
	uint32_t daddr, offset;
	const int16_t *src;
	int16_t *dest1, *dest2, *dest3, *dest4, *dest5, *dest6;

	//digitalWriteFast(3, HIGH);
	daddr = (uint32_t)(dma.TCD->DADDR);
	dma.clearInterrupt();

	if (daddr < (uint32_t)i2s_rx_buffer + sizeof(i2s_rx_buffer) / 2) {
		// DMA is receiving to the first half of the buffer
		// need to remove data from the second half
		src = (int16_t *)((uint32_t)i2s_rx_buffer + sizeof(i2s_rx_buffer) / 2);
		if (update_responsibility) update_all();
	} else {
		// DMA is receiving to the second half of the buffer
		// need to remove data from the first half
		src = (int16_t *)&i2s_rx_buffer[0];
	}
	if (block_ch1) {
		offset = block_offset;
		if (offset <= AUDIO_BLOCK_SAMPLES/2) {
			arm_dcache_delete((void*)src, sizeof(i2s_rx_buffer) / 2);
			block_offset = offset + AUDIO_BLOCK_SAMPLES/2;
			dest1 = &(block_ch1->data[offset]);
			dest2 = &(block_ch2->data[offset]);
			dest3 = &(block_ch3->data[offset]);
			dest4 = &(block_ch4->data[offset]);
			dest5 = &(block_ch5->data[offset]);
			dest6 = &(block_ch6->data[offset]);
			for (int i=0; i < AUDIO_BLOCK_SAMPLES/2; i++) {
				*dest1++ = *src++;
				*dest3++ = *src++;
				*dest5++ = *src++;
				*dest2++ = *src++;
				*dest4++ = *src++;
				*dest6++ = *src++;
			}
		}
	}
	//digitalWriteFast(3, LOW);
}


void AudioInputI2SHex::update(void)
{
	audio_block_t *new1, *new2, *new3, *new4, *new5, *new6;
	audio_block_t *out1, *out2, *out3, *out4, *out5, *out6;

	// allocate 6 new blocks
	new1 = allocate();
	new2 = allocate();
	new3 = allocate();
	new4 = allocate();
	new5 = allocate();
	new6 = allocate();
	// but if any fails, allocate none
	if (!new1 || !new2 || !new3 || !new4 || !new5 || !new6) {
		if (new1) {
			release(new1);
			new1 = NULL;
		}
		if (new2) {
			release(new2);
			new2 = NULL;
		}
		if (new3) {
			release(new3);
			new3 = NULL;
		}
		if (new4) {
			release(new4);
			new4 = NULL;
		}
		if (new5) {
			release(new5);
			new5 = NULL;
		}
		if (new6) {
			release(new6);
			new6 = NULL;
		}
	}
	__disable_irq();
	if (block_offset >= AUDIO_BLOCK_SAMPLES) {
		// the DMA filled 4 blocks, so grab them and get the
		// 4 new blocks to the DMA, as quickly as possible
		out1 = block_ch1;
		block_ch1 = new1;
		out2 = block_ch2;
		block_ch2 = new2;
		out3 = block_ch3;
		block_ch3 = new3;
		out4 = block_ch4;
		block_ch4 = new4;
		out5 = block_ch5;
		block_ch5 = new5;
		out6 = block_ch6;
		block_ch6 = new6;
		block_offset = 0;
		__enable_irq();
		// then transmit the DMA's former blocks
		transmit(out1, 0);
		release(out1);
		transmit(out2, 1);
		release(out2);
		transmit(out3, 2);
		release(out3);
		transmit(out4, 3);
		release(out4);
		transmit(out5, 4);
		release(out5);
		transmit(out6, 5);
		release(out6);
	} else if (new1 != NULL) {
		// the DMA didn't fill blocks, but we allocated blocks
		if (block_ch1 == NULL) {
			// the DMA doesn't have any blocks to fill, so
			// give it the ones we just allocated
			block_ch1 = new1;
			block_ch2 = new2;
			block_ch3 = new3;
			block_ch4 = new4;
			block_ch5 = new5;
			block_ch6 = new6;
			block_offset = 0;
			__enable_irq();
		} else {
			// the DMA already has blocks, doesn't need these
			__enable_irq();
			release(new1);
			release(new2);
			release(new3);
			release(new4);
			release(new5);
			release(new6);
		}
	} else {
		// The DMA didn't fill blocks, and we could not allocate
		// memory... the system is likely starving for memory!
		// Sadly, there's nothing we can do.
		__enable_irq();
	}
}

#else // not supported

void AudioInputI2SHex::begin(void)
{
}



#endif

