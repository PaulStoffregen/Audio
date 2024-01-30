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
static uint32_t tdm_rx_buffer[AUDIO_BLOCK_SAMPLES*TDM_IN_N_AUDIO_BLOCKS];
audio_block_t * AudioInputTDM::block_incoming[TDM_IN_N_AUDIO_BLOCKS];
bool AudioInputTDM::update_responsibility = false;
DMAChannel AudioInputTDM::dma(false);

void AudioInputTDM::begin(void)
{
	for (int i = 0; i < TDM_IN_N_AUDIO_BLOCKS; i++)
		block_incoming[i] = nullptr;
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
	IOMUXC_SAI1_RX_DATA0_SELECT_INPUT = 2; // TEENSY P8, GPIO_B1_00_ALT3  as rx 0
        // When using combine mode, you *must* use the data pins in order.
	switch (AUDIO_N_SAI1_RX_DATAPINS) {
        case 4: 
            CORE_PIN32_CONFIG  = 3;  //RX_DATA3
            IOMUXC_SAI1_RX_DATA3_SELECT_INPUT = 1; // TEENSY P32, GPIO_B0_12_ALT3 as rx 1
        case 3:
            CORE_PIN9_CONFIG  = 3;  //RX_DATA2
            IOMUXC_SAI1_RX_DATA2_SELECT_INPUT = 1; // TEENSY P9, GPIO_B0_11_ALT3 as rx 2
        case 2:
            CORE_PIN6_CONFIG  = 3;  //RX_DATA1
            IOMUXC_SAI1_RX_DATA1_SELECT_INPUT = 1; // TEENSY P6, GPIO_B0_10_ALT3 as rx 1
            break;
        default:
            break;
	}
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

// Since we're grabbing data out of the fifo at 32-bits, but the words
// are 16-bits and we're sharing fifos in a round-robbin manner, with
//
// 1 FIFO (pin) active, the word/channel order for the first frame is
// C00 C02 C04 C06 C08 C10 C12 C14
// C01 C03 C05 C07 C09 C11 C13 C15
//
// 2 FIFO (pin) active, the word/channel order for the first frame is
// C00 C16 C02 C18 C04 C20 C06 C22 C08 C24 C10 C26 C12 C28 C14 C30 
// C01 C17 C03 C19 C05 C21 C07 C23 C09 C25 C11 C27 C13 C29 C15 C31 
//
// 3 FIFO (pin) active, the word/channel order for the first frame is
// C00 C16 C32 C02 C18 C34 C04 C20 C36 C06 C22 C38 C08 C24 C40 C10 C26 C42 C12 C28 C44 C14 C30 C46
// C01 C17 C33 C03 C19 C35 C05 C21 C37 C07 C23 C39 C09 C25 C41 C11 C27 C43 C13 C29 C45 C15 C31 C47
//
// 4 FIFO (pin) active, the word/channel order for the first frame is
// C00 C16 C32 C48|C02 C18 C34 C50|C04 C20 C36 C52|C06 C22 C38 C54|C08 C24 C40 C56|C10 C26 C42 C58|C12 C28 C44 C60|C14 C30 C46 C62
// C01 C17 C33 C49|C03 C19 C35 C51|C05 C21 C37 C53|C07 C23 C39 C55|C09 C25 C41 C57|C11 C27 C43 C59|C13 C29 C45 C61|C15 C31 C47 C63 
//  0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31 <--- word index
// where each column is a single 32-bit word popped from the fifo
static void deinterleave(audio_block_t *block_incoming[TDM_IN_N_AUDIO_BLOCKS], const uint32_t *src) {
    int l = 0;
    for (int i=0; i < TDM_IN_N_AUDIO_BLOCKS/2; i++) {
        // ix = 0,  2,  4,  6,          ... 1 pin
        // ix = 0, 16,  2, 18,  3, 20, ... 2 pins
        // ix = 0, 16, 32,  2, 18, 33 ... 3 pins
        // ix = 0, 16, 32, 48,  2, 18 ... 4 pins

        //  i = 0,  1,  2,  3,  4,  5,
        // ix = 0, 16, 32,  2, 18, 33 ... 3 pins
        //    = (0*16 1*16 2*16)+i
        int ix = (l*16) + i/AUDIO_N_SAI1_RX_DATAPINS*2;
        l = (l + 1) % AUDIO_N_SAI1_RX_DATAPINS;
        uint32_t *dest1 = (uint32_t *)(block_incoming[ix]->data);
        uint32_t *dest2 = (uint32_t *)(block_incoming[ix+1]->data);
        const uint32_t *psrc = src;
        for (int j=0; j < AUDIO_BLOCK_SAMPLES/2; j++) {
            uint32_t in1 = *psrc;
            uint32_t in2 = *(psrc + TDM_IN_N_AUDIO_BLOCKS * AUDIO_N_SAI1_RX_DATAPINS / 2);
            psrc += TDM_IN_N_AUDIO_BLOCKS;
            *dest1++ = ((in1 >> 16) & 0x0000FFFF) | ((in2 <<  0) & 0xFFFF0000);
            *dest2++ = ((in1 >>  0) & 0x0000FFFF) | ((in2 << 16) & 0xFFFF0000);
        }
        src++;
    }
}

void AudioInputTDM::isr(void)
{
	uint32_t daddr;
	const uint32_t *src;

	daddr = (uint32_t)(dma.TCD->DADDR);
	dma.clearInterrupt();

	if (daddr < (uint32_t)tdm_rx_buffer + sizeof(tdm_rx_buffer) / 2) {
		// DMA is receiving to the first half of the buffer
		// need to remove data from the second half
		src = &tdm_rx_buffer[AUDIO_BLOCK_SAMPLES * TDM_IN_N_AUDIO_BLOCKS / 2];
	} else {
		// DMA is receiving to the second half of the buffer
		// need to remove data from the first half
		src = &tdm_rx_buffer[0];
	}
	if (block_incoming[0] != nullptr) {
		#if IMXRT_CACHE_ENABLED >=1
		arm_dcache_delete((void*)src, sizeof(tdm_rx_buffer) / 2);
		#endif
                deinterleave(block_incoming, src);
	}
	if (update_responsibility) update_all();
}


void AudioInputTDM::update(void)
{
    unsigned int i, j;
	audio_block_t *new_block[TDM_IN_N_AUDIO_BLOCKS];
	audio_block_t *out_block[TDM_IN_N_AUDIO_BLOCKS];
	// allocate TDM_IN_N_AUDIO_BLOCKS new blocks.  If any fails, allocate none
	for (i=0; i < TDM_IN_N_AUDIO_BLOCKS; i++) {
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
		// if we got 1 block, all TDM_IN_N_AUDIO_BLOCKS are filled
		for (i=0; i < TDM_IN_N_AUDIO_BLOCKS; i++) {
			transmit(out_block[i], i);
			release(out_block[i]);
		}
	}
}


#endif
