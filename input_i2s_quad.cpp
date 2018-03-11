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
#include "input_i2s_quad.h"
#include "output_i2s_quad.h"

DMAMEM static uint32_t i2s_rx_buffer[AUDIO_BLOCK_SAMPLES*2];
audio_block_t * AudioInputI2SQuad::block_ch1 = NULL;
audio_block_t * AudioInputI2SQuad::block_ch2 = NULL;
audio_block_t * AudioInputI2SQuad::block_ch3 = NULL;
audio_block_t * AudioInputI2SQuad::block_ch4 = NULL;
uint16_t AudioInputI2SQuad::block_offset = 0;
bool AudioInputI2SQuad::update_responsibility = false;
DMAChannel AudioInputI2SQuad::dma(false);

#if defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)

void AudioInputI2SQuad::begin(void)
{
	dma.begin(true); // Allocate the DMA channel first

	// TODO: should we set & clear the I2S_RCSR_SR bit here?
	AudioOutputI2SQuad::config_i2s();

	CORE_PIN13_CONFIG = PORT_PCR_MUX(4); // pin 13, PTC5, I2S0_RXD0
#if defined(__MK20DX256__)
	CORE_PIN30_CONFIG = PORT_PCR_MUX(4); // pin 30, PTC11, I2S0_RXD1
#elif defined(__MK64FX512__) || defined(__MK66FX1M0__)
	CORE_PIN38_CONFIG = PORT_PCR_MUX(4); // pin 38, PTC11, I2S0_RXD1
#endif

#if defined(KINETISK)
	dma.TCD->SADDR = &I2S0_RDR0;
	dma.TCD->SOFF = 4;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_SMOD(3) | DMA_TCD_ATTR_DSIZE(1);
	dma.TCD->NBYTES_MLNO = 4;
	dma.TCD->SLAST = 0;
	dma.TCD->DADDR = i2s_rx_buffer;
	dma.TCD->DOFF = 2;
	dma.TCD->CITER_ELINKNO = sizeof(i2s_rx_buffer) / 4;
	dma.TCD->DLASTSGA = -sizeof(i2s_rx_buffer);
	dma.TCD->BITER_ELINKNO = sizeof(i2s_rx_buffer) / 4;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
#endif
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_RX);
	update_responsibility = update_setup();
	dma.enable();

	I2S0_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE | I2S_RCSR_FRDE | I2S_RCSR_FR;
	I2S0_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE; // TX clock enable, because sync'd to TX
	dma.attachInterrupt(isr);
}

void AudioInputI2SQuad::isr(void)
{
	uint32_t daddr, offset;
	const int16_t *src;
	int16_t *dest1, *dest2, *dest3, *dest4;

	//digitalWriteFast(3, HIGH);
	daddr = (uint32_t)(dma.TCD->DADDR);
	dma.clearInterrupt();

	if (daddr < (uint32_t)i2s_rx_buffer + sizeof(i2s_rx_buffer) / 2) {
		// DMA is receiving to the first half of the buffer
		// need to remove data from the second half
		src = (int16_t *)&i2s_rx_buffer[AUDIO_BLOCK_SAMPLES];
		if (update_responsibility) update_all();
	} else {
		// DMA is receiving to the second half of the buffer
		// need to remove data from the first half
		src = (int16_t *)&i2s_rx_buffer[0];
	}
	if (block_ch1) {
		offset = block_offset;
		if (offset <= AUDIO_BLOCK_SAMPLES/2) {
			block_offset = offset + AUDIO_BLOCK_SAMPLES/2;
			dest1 = &(block_ch1->data[offset]);
			dest2 = &(block_ch2->data[offset]);
			dest3 = &(block_ch3->data[offset]);
			dest4 = &(block_ch4->data[offset]);
			for (int i=0; i < AUDIO_BLOCK_SAMPLES/2; i++) {
				*dest1++ = *src++;
				*dest3++ = *src++;
				*dest2++ = *src++;
				*dest4++ = *src++;
			}
		}
	}
	//digitalWriteFast(3, LOW);
}


void AudioInputI2SQuad::update(void)
{
	audio_block_t *new1, *new2, *new3, *new4;
	audio_block_t *out1, *out2, *out3, *out4;

	// allocate 4 new blocks
	new1 = allocate();
	new2 = allocate();
	new3 = allocate();
	new4 = allocate();
	// but if any fails, allocate none
	if (!new1 || !new2 || !new3 || !new4) {
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
	} else if (new1 != NULL) {
		// the DMA didn't fill blocks, but we allocated blocks
		if (block_ch1 == NULL) {
			// the DMA doesn't have any blocks to fill, so
			// give it the ones we just allocated
			block_ch1 = new1;
			block_ch2 = new2;
			block_ch3 = new3;
			block_ch4 = new4;
			block_offset = 0;
			__enable_irq();
		} else {
			// the DMA already has blocks, doesn't need these
			__enable_irq();
			release(new1);
			release(new2);
			release(new3);
			release(new4);
		}
	} else {
		// The DMA didn't fill blocks, and we could not allocate
		// memory... the system is likely starving for memory!
		// Sadly, there's nothing we can do.
		__enable_irq();
	}
}

#else // not __MK20DX256__

void AudioInputI2SQuad::begin(void)
{
}



#endif

