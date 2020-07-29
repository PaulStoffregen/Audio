/* Audio Library for Teensy 3.X
 * Copyright (c) 2017, Paul Stoffregen, paul@pjrc.com
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
#include "input_tdm.h"
#include "output_tdm.h"
#if defined(KINETISK) || defined(__IMXRT1062__)
#include "utility/imxrt_hw.h"

DMAMEM __attribute__((aligned(32)))
static uint32_t tdm_rx_buffer[AUDIO_BLOCK_SAMPLES*16];
audio_block_t * AudioInputTDM::block_incoming[16] = {
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
};
bool AudioInputTDM::update_responsibility = false;
DMAChannel AudioInputTDM::dma(false);


void AudioInputTDM::begin(void)
{
	dma.begin(true); // Allocate the DMA channel first

	// TODO: should we set & clear the I2S_RCSR_SR bit here?
	AudioOutputTDM::config_tdm();
#if defined(KINETISK)
	CORE_PIN13_CONFIG = PORT_PCR_MUX(4); // pin 13, PTC5, I2S0_RXD0
	dma.TCD->SADDR = &I2S0_RDR0;
	dma.TCD->SOFF = 0;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
	dma.TCD->NBYTES_MLNO = 4;
	dma.TCD->SLAST = 0;
	dma.TCD->DADDR = tdm_rx_buffer;
	dma.TCD->DOFF = 4;
	dma.TCD->CITER_ELINKNO = sizeof(tdm_rx_buffer) / 4;
	dma.TCD->DLASTSGA = -sizeof(tdm_rx_buffer);
	dma.TCD->BITER_ELINKNO = sizeof(tdm_rx_buffer) / 4;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_RX);
	update_responsibility = update_setup();
	dma.enable();

	I2S0_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE | I2S_RCSR_FRDE | I2S_RCSR_FR;
	I2S0_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE; // TX clock enable, because sync'd to TX
	dma.attachInterrupt(isr);
#elif defined(__IMXRT1062__)
	CORE_PIN8_CONFIG  = 3;  //RX_DATA0
	IOMUXC_SAI1_RX_DATA0_SELECT_INPUT = 2;
	dma.TCD->SADDR = &I2S1_RDR0;
	dma.TCD->SOFF = 0;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
	dma.TCD->NBYTES_MLNO = 4;
	dma.TCD->SLAST = 0;
	dma.TCD->DADDR = tdm_rx_buffer;
	dma.TCD->DOFF = 4;
	dma.TCD->CITER_ELINKNO = sizeof(tdm_rx_buffer) / 4;
	dma.TCD->DLASTSGA = -sizeof(tdm_rx_buffer);
	dma.TCD->BITER_ELINKNO = sizeof(tdm_rx_buffer) / 4;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI1_RX);
	update_responsibility = update_setup();
	dma.enable();

	I2S1_RCSR = I2S_RCSR_RE | I2S_RCSR_BCE | I2S_RCSR_FRDE | I2S_RCSR_FR;
	dma.attachInterrupt(isr);	
#endif	
}

// TODO: needs optimization...
static void memcpy_tdm_rx(uint32_t *dest1, uint32_t *dest2, const uint32_t *src)
{
	uint32_t i, in1, in2;

	for (i=0; i < AUDIO_BLOCK_SAMPLES/2; i++) {
		in1 = *src;
		in2 = *(src+8);
		src += 16;
		*dest1++ = (in1 >> 16) | (in2 & 0xFFFF0000);
		*dest2++ = (in1 << 16) | (in2 & 0x0000FFFF);
	}
}

void AudioInputTDM::isr(void)
{
	uint32_t daddr;
	const uint32_t *src;
	unsigned int i;

	daddr = (uint32_t)(dma.TCD->DADDR);
	dma.clearInterrupt();

	if (daddr < (uint32_t)tdm_rx_buffer + sizeof(tdm_rx_buffer) / 2) {
		// DMA is receiving to the first half of the buffer
		// need to remove data from the second half
		src = &tdm_rx_buffer[AUDIO_BLOCK_SAMPLES*8];
	} else {
		// DMA is receiving to the second half of the buffer
		// need to remove data from the first half
		src = &tdm_rx_buffer[0];
	}
	if (block_incoming[0] != nullptr) {
		#if IMXRT_CACHE_ENABLED >=1
		arm_dcache_delete((void*)src, sizeof(tdm_rx_buffer) / 2);
		#endif
		for (i=0; i < 16; i += 2) {
			uint32_t *dest1 = (uint32_t *)(block_incoming[i]->data);
			uint32_t *dest2 = (uint32_t *)(block_incoming[i+1]->data);
			memcpy_tdm_rx(dest1, dest2, src);
			src++;
		}
	}
	if (update_responsibility) update_all();
}


void AudioInputTDM::update(void)
{
	unsigned int i, j;
	audio_block_t *new_block[16];
	audio_block_t *out_block[16];

	// allocate 16 new blocks.  If any fails, allocate none
	for (i=0; i < 16; i++) {
		new_block[i] = allocate();
		if (new_block[i] == nullptr) {
			for (j=0; j < i; j++) {
				release(new_block[j]);
			}
			memset(new_block, 0, sizeof(new_block));
			break;
		}
	}
	__disable_irq();
	memcpy(out_block, block_incoming, sizeof(out_block));
	memcpy(block_incoming, new_block, sizeof(block_incoming));
	__enable_irq();
	if (out_block[0] != nullptr) {		
		// if we got 1 block, all 16 are filled
		for (i=0; i < 16; i++) {
			transmit(out_block[i], i);
			release(out_block[i]);
		}
	}
}


#endif
