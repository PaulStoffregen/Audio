/* Audio Library for Teensy 3.X
 * Copyright (c) 2019, Paul Stoffregen, paul@pjrc.com
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
/*
 by Frank Bösing
 */

#if defined(__IMXRT1052__) || defined(__IMXRT1062__)
#include <Arduino.h>
#include "input_spdif3.h"
#include "output_spdif3.h"
#include "utility/imxrt_hw.h"

DMAMEM __attribute__((aligned(32)))
static uint32_t spdif_rx_buffer[AUDIO_BLOCK_SAMPLES * 4];
audio_block_t * AudioInputSPDIF3::block_left = NULL;
audio_block_t * AudioInputSPDIF3::block_right = NULL;
uint16_t AudioInputSPDIF3::block_offset = 0;
bool AudioInputSPDIF3::update_responsibility = false;
DMAChannel AudioInputSPDIF3::dma(false);

FLASHMEM
void AudioInputSPDIF3::begin(void)
{
	dma.begin(true); // Allocate the DMA channel first

	AudioOutputSPDIF3::config_spdif3();

	const int nbytes_mlno = 2 * 4; // 8 Bytes per minor loop
	dma.TCD->SADDR = &SPDIF_SRL;
	dma.TCD->SOFF = 4;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
	dma.TCD->NBYTES_MLNO = DMA_TCD_NBYTES_MLOFFYES_NBYTES(nbytes_mlno) | DMA_TCD_NBYTES_SMLOE |
                         DMA_TCD_NBYTES_MLOFFYES_MLOFF(-8);
	dma.TCD->SLAST = -8;
	dma.TCD->DADDR = spdif_rx_buffer;
	dma.TCD->DOFF = 4;
	dma.TCD->DLASTSGA = -sizeof(spdif_rx_buffer);
	dma.TCD->CITER_ELINKNO = sizeof(spdif_rx_buffer) / nbytes_mlno;
	dma.TCD->BITER_ELINKNO = sizeof(spdif_rx_buffer) / nbytes_mlno;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;

	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SPDIF_RX);
	update_responsibility = update_setup();
	dma.attachInterrupt(isr);
	dma.enable();

	SPDIF_SRCD = 0;
	SPDIF_SCR |= SPDIF_SCR_DMA_RX_EN;
	CORE_PIN15_CONFIG = 3;
	IOMUXC_SPDIF_IN_SELECT_INPUT = 0; // GPIO_AD_B1_03_ALT3
	//pinMode(13, OUTPUT);
}

void AudioInputSPDIF3::isr(void)
{
	uint32_t daddr, offset;
	const int32_t *src, *end;
	int16_t *dest_left, *dest_right;
	audio_block_t *left, *right;

	dma.clearInterrupt();
	//digitalWriteFast(13, !digitalReadFast(13));
	if (AudioInputSPDIF3::update_responsibility) AudioStream::update_all();

	daddr = (uint32_t)(dma.TCD->DADDR);

	if (daddr < (uint32_t)spdif_rx_buffer + sizeof(spdif_rx_buffer) / 2) {
		// DMA is receiving to the first half of the buffer
		// need to remove data from the second half
		src = (int32_t *)&spdif_rx_buffer[AUDIO_BLOCK_SAMPLES * 2];
		end = (int32_t *)&spdif_rx_buffer[AUDIO_BLOCK_SAMPLES * 4];
	} else {
		// DMA is receiving to the second half of the buffer
		// need to remove data from the first half
		src = (int32_t *)&spdif_rx_buffer[0];
		end = (int32_t *)&spdif_rx_buffer[AUDIO_BLOCK_SAMPLES*2];
	}

	left = AudioInputSPDIF3::block_left;
	right = AudioInputSPDIF3::block_right;

	if (left != NULL && right != NULL) {
		offset = AudioInputSPDIF3::block_offset;
		if (offset <= AUDIO_BLOCK_SAMPLES*2) {
			dest_left = &(left->data[offset]);
			dest_right = &(right->data[offset]);
			AudioInputSPDIF3::block_offset = offset + AUDIO_BLOCK_SAMPLES*2;

			do {
				#if IMXRT_CACHE_ENABLED >=1
				SCB_CACHE_DCIMVAC = (uintptr_t)src;
				asm("dsb":::"memory");
				#endif

				*dest_left++ = (*src++) >> 8;
				*dest_right++ = (*src++) >> 8;

				*dest_left++ = (*src++) >> 8;
				*dest_right++ = (*src++) >> 8;

				*dest_left++ = (*src++) >> 8;
				*dest_right++ = (*src++) >> 8;

				*dest_left++ = (*src++) >> 8;
				*dest_right++ = (*src++) >> 8;

			} while (src < end);
		}
	}
	else if (left != NULL) {
		offset = AudioInputSPDIF3::block_offset;
		if (offset <= AUDIO_BLOCK_SAMPLES*2) {
			dest_left = &(left->data[offset]);
			AudioInputSPDIF3::block_offset = offset + AUDIO_BLOCK_SAMPLES*2;

			do {
				#if IMXRT_CACHE_ENABLED >=1
				SCB_CACHE_DCIMVAC = (uintptr_t)src;
				asm("dsb":::"memory");
				#endif

				*dest_left++ = (*src++) >> 8;
				src++;

				*dest_left++ = (*src++) >> 8;
				src++;

				*dest_left++ = (*src++) >> 8;
				src++;

				*dest_left++ = (*src++) >> 8;
				src++;

			} while (src < end);
		}		
	}
	else if (right != NULL) {
		offset = AudioInputSPDIF3::block_offset;
		if (offset <= AUDIO_BLOCK_SAMPLES*2) {
			dest_right = &(right->data[offset]);
			AudioInputSPDIF3::block_offset = offset + AUDIO_BLOCK_SAMPLES*2;

			do {
				#if IMXRT_CACHE_ENABLED >=1
				SCB_CACHE_DCIMVAC = (uintptr_t)src;
				asm("dsb":::"memory");
				#endif

				src++;
				*dest_right++ = (*src++) >> 8;

				src++;
				*dest_right++ = (*src++) >> 8;

				src++;
				*dest_right++ = (*src++) >> 8;

				src++;
				*dest_right++ = (*src++) >> 8;

			} while (src < end);
		}		
	}

}


void AudioInputSPDIF3::update(void)
{
	audio_block_t *new_left=NULL, *new_right=NULL, *out_left=NULL, *out_right=NULL;

	// allocate 2 new blocks, but if one fails, allocate neither
	new_left = allocate();
	if (new_left != NULL) {
		new_right = allocate();
		if (new_right == NULL) {
			release(new_left);
			new_left = NULL;
		}
	}
	__disable_irq();
	if (block_offset >= AUDIO_BLOCK_SAMPLES) {
		// the DMA filled 2 blocks, so grab them and get the
		// 2 new blocks to the DMA, as quickly as possible
		out_left = block_left;
		block_left = new_left;
		out_right = block_right;
		block_right = new_right;
		block_offset = 0;
		__enable_irq();
		// then transmit the DMA's former blocks
		transmit(out_left, 0);
		release(out_left);
		transmit(out_right, 1);
		release(out_right);
		//Serial.print(".");
	} else if (new_left != NULL) {
		// the DMA didn't fill blocks, but we allocated blocks
		if (block_left == NULL) {
			// the DMA doesn't have any blocks to fill, so
			// give it the ones we just allocated
			block_left = new_left;
			block_right = new_right;
			block_offset = 0;
			__enable_irq();
		} else {
			// the DMA already has blocks, doesn't need these
			__enable_irq();
			release(new_left);
			release(new_right);
		}
	} else {
		// The DMA didn't fill blocks, and we could not allocate
		// memory... the system is likely starving for memory!
		// Sadly, there's nothing we can do.
		__enable_irq();
	}
}

bool AudioInputSPDIF3::pllLocked(void)
{
	return (SPDIF_SRPC & SPDIF_SRPC_LOCK) == SPDIF_SRPC_LOCK ? true:false;
}

unsigned int AudioInputSPDIF3::sampleRate(void) {
	if (!pllLocked()) return 0;
	return (float)((uint64_t)F_BUS_ACTUAL * SPDIF_SRFM) / (0x8000000ULL * AudioOutputSPDIF3::dpll_Gain()) + 0.5F;
}

#endif
