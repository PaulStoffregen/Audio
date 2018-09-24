/* Audio Library for Teensy 3.X
 * Copyright (c) 2016, Paul Stoffregen, paul@pjrc.com
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
#include "output_dacs.h"
#include "utility/pdb.h"

#if defined(__MK64FX512__) || defined(__MK66FX1M0__)

DMAMEM static uint32_t dac_buffer[AUDIO_BLOCK_SAMPLES*2];
audio_block_t * AudioOutputAnalogStereo::block_left_1st = NULL;
audio_block_t * AudioOutputAnalogStereo::block_left_2nd = NULL;
audio_block_t * AudioOutputAnalogStereo::block_right_1st = NULL;
audio_block_t * AudioOutputAnalogStereo::block_right_2nd = NULL;
audio_block_t AudioOutputAnalogStereo::block_silent;
bool AudioOutputAnalogStereo::update_responsibility = false;
DMAChannel AudioOutputAnalogStereo::dma(false);

void AudioOutputAnalogStereo::begin(void)
{
	dma.begin(true); // Allocate the DMA channel first

	SIM_SCGC2 |= SIM_SCGC2_DAC0 | SIM_SCGC2_DAC1;
	DAC0_C0 = DAC_C0_DACEN;                   // 1.2V VDDA is DACREF_2
	DAC1_C0 = DAC_C0_DACEN;
	memset(&block_silent, 0, sizeof(block_silent));

	// slowly ramp up to DC voltage, approx 1/4 second
	for (int16_t i=0; i<=2048; i+=8) {
		*(int16_t *)&(DAC0_DAT0L) = i;
		*(int16_t *)&(DAC1_DAT0L) = i;
		delay(1);
	}

	// set the programmable delay block to trigger DMA requests
	if (!(SIM_SCGC6 & SIM_SCGC6_PDB)
	  || (PDB0_SC & PDB_CONFIG) != PDB_CONFIG
	  || PDB0_MOD != PDB_PERIOD
	  || PDB0_IDLY != 1
	  || PDB0_CH0C1 != 0x0101) {
		SIM_SCGC6 |= SIM_SCGC6_PDB;
		PDB0_IDLY = 1;
		PDB0_MOD = PDB_PERIOD;
		PDB0_SC = PDB_CONFIG | PDB_SC_LDOK;
		PDB0_SC = PDB_CONFIG | PDB_SC_SWTRIG;
		PDB0_CH0C1 = 0x0101;
	}

	dma.TCD->SADDR = dac_buffer;
	dma.TCD->SOFF = 4;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(DMA_TCD_ATTR_SIZE_32BIT) |
		DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_16BIT);
	dma.TCD->NBYTES_MLNO = DMA_TCD_NBYTES_MLOFFYES_NBYTES(4) | DMA_TCD_NBYTES_DMLOE |
		DMA_TCD_NBYTES_MLOFFYES_MLOFF((&DAC0_DAT0L - &DAC1_DAT0L) * 2);
	dma.TCD->SLAST = -sizeof(dac_buffer);
	dma.TCD->DADDR = &DAC0_DAT0L;
	dma.TCD->DOFF = &DAC1_DAT0L - &DAC0_DAT0L;
	dma.TCD->CITER_ELINKNO = sizeof(dac_buffer) / 4;
	dma.TCD->DLASTSGA = (&DAC0_DAT0L - &DAC1_DAT0L) * 2;
	dma.TCD->BITER_ELINKNO = sizeof(dac_buffer) / 4;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_PDB);
	update_responsibility = update_setup();
	dma.enable();
	dma.attachInterrupt(isr);
}

void AudioOutputAnalogStereo::analogReference(int ref)
{
	// TODO: this should ramp gradually to the new DC level
	if (ref == INTERNAL) {
		DAC0_C0 &= ~DAC_C0_DACRFS; // 1.2V
		DAC1_C0 &= ~DAC_C0_DACRFS;
	} else {
		DAC0_C0 |= DAC_C0_DACRFS;  // 3.3V
		DAC1_C0 |= DAC_C0_DACRFS;
	}
}


void AudioOutputAnalogStereo::update(void)
{
	audio_block_t *block_left, *block_right;

	block_left = receiveReadOnly(0);  // input 0
	block_right = receiveReadOnly(1); // input 1
	__disable_irq();
	if (block_left) {
		if (block_left_1st == NULL) {
			block_left_1st = block_left;
			block_left = NULL;
		} else if (block_left_2nd == NULL) {
			block_left_2nd = block_left;
			block_left = NULL;
		} else {
			audio_block_t *tmp = block_left_1st;
			block_left_1st = block_left_2nd;
			block_left_2nd = block_left;
			block_left = tmp;
		}
	}
	if (block_right) {
		if (block_right_1st == NULL) {
			block_right_1st = block_right;
			block_right = NULL;
		} else if (block_right_2nd == NULL) {
			block_right_2nd = block_right;
			block_right = NULL;
		} else {
			audio_block_t *tmp = block_right_1st;
			block_right_1st = block_right_2nd;
			block_right_2nd = block_right;
			block_right = tmp;
		}
	}
	__enable_irq();
	if (block_left) release(block_left);
	if (block_right) release(block_right);
}

// TODO: the DAC has much higher bandwidth than the datasheet says
// can we output a 2X oversampled output, for easier filtering?

void AudioOutputAnalogStereo::isr(void)
{
	const uint32_t *src_left, *src_right, *end;
	uint32_t *dest;
	audio_block_t *block_left, *block_right;
	uint32_t saddr;

	saddr = (uint32_t)(dma.TCD->SADDR);
	dma.clearInterrupt();
	if (saddr < (uint32_t)dac_buffer + sizeof(dac_buffer) / 2) {
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		dest = dac_buffer + AUDIO_BLOCK_SAMPLES;
		end = dac_buffer + AUDIO_BLOCK_SAMPLES*2;
	} else {
		// DMA is transmitting the second half of the buffer
		// so we must fill the first half
		dest = dac_buffer;
		end = dac_buffer + AUDIO_BLOCK_SAMPLES;
	}
	block_left = block_left_1st;
	if (!block_left) block_left = &block_silent;
	block_right = block_right_1st;
	if (!block_right) block_right = &block_silent;

	src_left = (const uint32_t *)(block_left->data);
	src_right = (const uint32_t *)(block_right->data);
	do {
		// TODO: can this be optimized?
		uint32_t left = *src_left++;
		uint32_t right = *src_right++;
		uint32_t out1 = ((left & 0xFFFF) + 32768) >> 4;
		out1 |= (((right & 0xFFFF) + 32768) >> 4) << 16;
		uint32_t out2 = ((left >> 16) + 32768) >> 4;
		out2 |= (((right >> 16) + 32768) >> 4) << 16;
		*dest++ = out1;
		*dest++ = out2;
	} while (dest < end);

	if (block_left != &block_silent) {
		release(block_left);
		block_left_1st = block_left_2nd;
		block_left_2nd = NULL;
	}
	if (block_right != &block_silent) {
		release(block_right);
		block_right_1st = block_right_2nd;
		block_right_2nd = NULL;
	}

	if (update_responsibility) update_all();
}

#else // not __MK64FX512__ or __MK66FX1M0__

void AudioOutputAnalogStereo::begin(void)
{
}

void AudioOutputAnalogStereo::update(void)
{
	audio_block_t *block;
	block = receiveReadOnly(0); // input 0
	if (block) release(block);
	block = receiveReadOnly(1); // input 1
	if (block) release(block);
}

#endif


