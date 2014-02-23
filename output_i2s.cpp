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

#include "output_i2s.h"

// MCLK needs to be 48e6 / 1088 * 256 = 11.29411765 MHz -> 44.117647 kHz sample rate
// Possible to create using fractional divider for all USB-compatible Kinetis:
// MCLK = 16e6 * 12 / 17
// MCLK = 24e6 * 8 / 17
// MCLK = 48e6 * 4 / 17
// MCLK = 72e6 * 8 / 51
// MCLK = 96e6 * 2 / 17
// MCLK = 120e6 * 8 / 85

// TODO: instigate using I2S0_MCR to select the crystal directly instead of the system
// clock, which has audio band jitter from the PLL


audio_block_t * AudioOutputI2S::block_left_1st = NULL;
audio_block_t * AudioOutputI2S::block_right_1st = NULL;
audio_block_t * AudioOutputI2S::block_left_2nd = NULL;
audio_block_t * AudioOutputI2S::block_right_2nd = NULL;
uint16_t  AudioOutputI2S::block_left_offset = 0;
uint16_t  AudioOutputI2S::block_right_offset = 0;
bool AudioOutputI2S::update_responsibility = false;
DMAMEM static uint32_t i2s_tx_buffer[AUDIO_BLOCK_SAMPLES];

void AudioOutputI2S::begin(void)
{
	//pinMode(2, OUTPUT);
	block_left_1st = NULL;
	block_right_1st = NULL;

	// TODO: should we set & clear the I2S_TCSR_SR bit here?
	config_i2s();
	CORE_PIN22_CONFIG = PORT_PCR_MUX(6); // pin 22, PTC1, I2S0_TXD0

	DMA_CR = 0;
	DMA_TCD0_SADDR = i2s_tx_buffer;
	DMA_TCD0_SOFF = 2;
	DMA_TCD0_ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	DMA_TCD0_NBYTES_MLNO = 2;
	DMA_TCD0_SLAST = -sizeof(i2s_tx_buffer);
	DMA_TCD0_DADDR = &I2S0_TDR0;
	DMA_TCD0_DOFF = 0;
	DMA_TCD0_CITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;
	DMA_TCD0_DLASTSGA = 0;
	DMA_TCD0_BITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;
	DMA_TCD0_CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;

	DMAMUX0_CHCFG0 = DMAMUX_DISABLE;
	DMAMUX0_CHCFG0 = DMAMUX_SOURCE_I2S0_TX | DMAMUX_ENABLE;
	update_responsibility = update_setup();
	DMA_SERQ = 0;

	I2S0_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE | I2S_TCSR_FR;
	NVIC_ENABLE_IRQ(IRQ_DMA_CH0);
}


void dma_ch0_isr(void)
{
	const int16_t *src, *end;
	int16_t *dest;
	audio_block_t *block;
	uint32_t saddr, offset;

	saddr = (uint32_t)DMA_TCD0_SADDR;
        DMA_CINT = 0;
	if (saddr < (uint32_t)i2s_tx_buffer + sizeof(i2s_tx_buffer) / 2) {
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		dest = (int16_t *)&i2s_tx_buffer[AUDIO_BLOCK_SAMPLES/2];
		end = (int16_t *)&i2s_tx_buffer[AUDIO_BLOCK_SAMPLES];
		if (AudioOutputI2S::update_responsibility) AudioStream::update_all();
	} else {
		// DMA is transmitting the second half of the buffer
		// so we must fill the first half
		dest = (int16_t *)i2s_tx_buffer;
		end = (int16_t *)&i2s_tx_buffer[AUDIO_BLOCK_SAMPLES/2];
	}

	// TODO: these copy routines could be merged and optimized, maybe in assembly?
	block = AudioOutputI2S::block_left_1st;
	if (block) {
		offset = AudioOutputI2S::block_left_offset;
		src = &block->data[offset];
		do {
			*dest = *src++;
			dest += 2;
		} while (dest < end);
		offset += AUDIO_BLOCK_SAMPLES/2;
		if (offset < AUDIO_BLOCK_SAMPLES) {
			AudioOutputI2S::block_left_offset = offset;
		} else {
			AudioOutputI2S::block_left_offset = 0;
			AudioStream::release(block);
			AudioOutputI2S::block_left_1st = AudioOutputI2S::block_left_2nd;
			AudioOutputI2S::block_left_2nd = NULL;
		}
	} else {
		do {
			*dest = 0;
			dest += 2;
		} while (dest < end);
	}
	dest -= AUDIO_BLOCK_SAMPLES - 1;
	block = AudioOutputI2S::block_right_1st;
	if (block) {
		offset = AudioOutputI2S::block_right_offset;
		src = &block->data[offset];
		do {
			*dest = *src++;
			dest += 2;
		} while (dest < end);
		offset += AUDIO_BLOCK_SAMPLES/2;
		if (offset < AUDIO_BLOCK_SAMPLES) {
			AudioOutputI2S::block_right_offset = offset;
		} else {
			AudioOutputI2S::block_right_offset = 0;
			AudioStream::release(block);
			AudioOutputI2S::block_right_1st = AudioOutputI2S::block_right_2nd;
			AudioOutputI2S::block_right_2nd = NULL;
		}
	} else {
		do {
			*dest = 0;
			dest += 2;
		} while (dest < end);
	}
}




void AudioOutputI2S::update(void)
{
	// null audio device: discard all incoming data
	//if (!active) return;
	//audio_block_t *block = receiveReadOnly();
	//if (block) release(block);

	audio_block_t *block;
	block = receiveReadOnly(0); // input 0 = left channel
	if (block) {
		__disable_irq();
		if (block_left_1st == NULL) {
			block_left_1st = block;
			block_left_offset = 0;
			__enable_irq();
		} else if (block_left_2nd == NULL) {
			block_left_2nd = block;
			__enable_irq();
		} else {
			audio_block_t *tmp = block_left_1st;
			block_left_1st = block_left_2nd;
			block_left_2nd = block;
			block_left_offset = 0;
			__enable_irq();
			release(tmp);
		}
	}
	block = receiveReadOnly(1); // input 1 = right channel
	if (block) {
		__disable_irq();
		if (block_right_1st == NULL) {
			block_right_1st = block;
			block_right_offset = 0;
			__enable_irq();
		} else if (block_right_2nd == NULL) {
			block_right_2nd = block;
			__enable_irq();
		} else {
			audio_block_t *tmp = block_right_1st;
			block_right_1st = block_right_2nd;
			block_right_2nd = block;
			block_right_offset = 0;
			__enable_irq();
			release(tmp);
		}
	}
}



void AudioOutputI2S::config_i2s(void)
{
	SIM_SCGC6 |= SIM_SCGC6_I2S;
	SIM_SCGC7 |= SIM_SCGC7_DMA;
	SIM_SCGC6 |= SIM_SCGC6_DMAMUX;

	// if either transmitter or receiver is enabled, do nothing
	if (I2S0_TCSR & I2S_TCSR_TE) return;
	if (I2S0_RCSR & I2S_RCSR_RE) return;

	// enable MCLK output
	I2S0_MCR = I2S_MCR_MICS(3) | I2S_MCR_MOE;
	I2S0_MDR = I2S_MDR_FRACT(1) | I2S_MDR_DIVIDE(16);

	// configure transmitter
	I2S0_TMR = 0;
	I2S0_TCR1 = I2S_TCR1_TFW(1);  // watermark at half fifo size
	I2S0_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_BCP | I2S_TCR2_MSEL(1)
		| I2S_TCR2_BCD | I2S_TCR2_DIV(3);
	I2S0_TCR3 = I2S_TCR3_TCE;
	I2S0_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(15) | I2S_TCR4_MF
		| I2S_TCR4_FSE | I2S_TCR4_FSP | I2S_TCR4_FSD;
	I2S0_TCR5 = I2S_TCR5_WNW(15) | I2S_TCR5_W0W(15) | I2S_TCR5_FBT(15);

	// configure receiver (sync'd to transmitter clocks)
	I2S0_RMR = 0;
	I2S0_RCR1 = I2S_RCR1_RFW(1);
	I2S0_RCR2 = I2S_RCR2_SYNC(1) | I2S_TCR2_BCP | I2S_RCR2_MSEL(1)
		| I2S_RCR2_BCD | I2S_RCR2_DIV(3);
	I2S0_RCR3 = I2S_RCR3_RCE;
	I2S0_RCR4 = I2S_RCR4_FRSZ(1) | I2S_RCR4_SYWD(15) | I2S_RCR4_MF
		| I2S_RCR4_FSE | I2S_RCR4_FSP | I2S_RCR4_FSD;
	I2S0_RCR5 = I2S_RCR5_WNW(15) | I2S_RCR5_W0W(15) | I2S_RCR5_FBT(15);

	// configure pin mux for 3 clock signals
	CORE_PIN23_CONFIG = PORT_PCR_MUX(6); // pin 23, PTC2, I2S0_TX_FS (LRCLK)
	CORE_PIN9_CONFIG  = PORT_PCR_MUX(6); // pin  9, PTC3, I2S0_TX_BCLK
	CORE_PIN11_CONFIG = PORT_PCR_MUX(6); // pin 11, PTC6, I2S0_MCLK
}



/******************************************************************/

void AudioOutputI2Sslave::begin(void)
{
	//pinMode(2, OUTPUT);
	block_left_1st = NULL;
	block_right_1st = NULL;

	AudioOutputI2Sslave::config_i2s();
	CORE_PIN22_CONFIG = PORT_PCR_MUX(6); // pin 22, PTC1, I2S0_TXD0

	DMA_CR = 0;
	DMA_TCD0_SADDR = i2s_tx_buffer;
	DMA_TCD0_SOFF = 2;
	DMA_TCD0_ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	DMA_TCD0_NBYTES_MLNO = 2;
	DMA_TCD0_SLAST = -sizeof(i2s_tx_buffer);
	DMA_TCD0_DADDR = &I2S0_TDR0;
	DMA_TCD0_DOFF = 0;
	DMA_TCD0_CITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;
	DMA_TCD0_DLASTSGA = 0;
	DMA_TCD0_BITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;
	DMA_TCD0_CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;

	DMAMUX0_CHCFG0 = DMAMUX_DISABLE;
	DMAMUX0_CHCFG0 = DMAMUX_SOURCE_I2S0_TX | DMAMUX_ENABLE;
	update_responsibility = update_setup();
	DMA_SERQ = 0;

	I2S0_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE | I2S_TCSR_FR;
	NVIC_ENABLE_IRQ(IRQ_DMA_CH0);
}

void AudioOutputI2Sslave::config_i2s(void)
{
	SIM_SCGC6 |= SIM_SCGC6_I2S;
	SIM_SCGC7 |= SIM_SCGC7_DMA;
	SIM_SCGC6 |= SIM_SCGC6_DMAMUX;

	// if either transmitter or receiver is enabled, do nothing
	if (I2S0_TCSR & I2S_TCSR_TE) return;
	if (I2S0_RCSR & I2S_RCSR_RE) return;

	// Select input clock 0
	// Configure to input the bit-clock from pin, bypasses the MCLK divider
	I2S0_MCR = I2S_MCR_MICS(0);
	I2S0_MDR = 0;

	// configure transmitter
	I2S0_TMR = 0;
	I2S0_TCR1 = I2S_TCR1_TFW(1);  // watermark at half fifo size
	I2S0_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_BCP;

	I2S0_TCR3 = I2S_TCR3_TCE;
	I2S0_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(15) | I2S_TCR4_MF
		| I2S_TCR4_FSE | I2S_TCR4_FSP;

	I2S0_TCR5 = I2S_TCR5_WNW(15) | I2S_TCR5_W0W(15) | I2S_TCR5_FBT(15);

	// configure receiver (sync'd to transmitter clocks)
	I2S0_RMR = 0;
	I2S0_RCR1 = I2S_RCR1_RFW(1);
	I2S0_RCR2 = I2S_RCR2_SYNC(1) | I2S_TCR2_BCP;

	I2S0_RCR3 = I2S_RCR3_RCE;
	I2S0_RCR4 = I2S_RCR4_FRSZ(1) | I2S_RCR4_SYWD(15) | I2S_RCR4_MF
		| I2S_RCR4_FSE | I2S_RCR4_FSP | I2S_RCR4_FSD;

	I2S0_RCR5 = I2S_RCR5_WNW(15) | I2S_RCR5_W0W(15) | I2S_RCR5_FBT(15);

	// configure pin mux for 3 clock signals
	CORE_PIN23_CONFIG = PORT_PCR_MUX(6); // pin 23, PTC2, I2S0_TX_FS (LRCLK)
	CORE_PIN9_CONFIG  = PORT_PCR_MUX(6); // pin  9, PTC3, I2S0_TX_BCLK
	CORE_PIN11_CONFIG = PORT_PCR_MUX(6); // pin 11, PTC6, I2S0_MCLK
}


