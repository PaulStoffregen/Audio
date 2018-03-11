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
#include "output_tdm.h"
#include "memcpy_audio.h"
#if defined(KINETISK)

audio_block_t * AudioOutputTDM::block_input[16] = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};
bool AudioOutputTDM::update_responsibility = false;
static uint32_t zeros[AUDIO_BLOCK_SAMPLES/2];
DMAMEM static uint32_t tdm_tx_buffer[AUDIO_BLOCK_SAMPLES*16];
DMAChannel AudioOutputTDM::dma(false);

void AudioOutputTDM::begin(void)
{
	dma.begin(true); // Allocate the DMA channel first

	for (int i=0; i < 16; i++) {
		block_input[i] = NULL;
	}

	// TODO: should we set & clear the I2S_TCSR_SR bit here?
	config_tdm();
	CORE_PIN22_CONFIG = PORT_PCR_MUX(6); // pin 22, PTC1, I2S0_TXD0

	dma.TCD->SADDR = tdm_tx_buffer;
	dma.TCD->SOFF = 4;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
	dma.TCD->NBYTES_MLNO = 4;
	dma.TCD->SLAST = -sizeof(tdm_tx_buffer);
	dma.TCD->DADDR = &I2S0_TDR0;
	dma.TCD->DOFF = 0;
	dma.TCD->CITER_ELINKNO = sizeof(tdm_tx_buffer) / 4;
	dma.TCD->DLASTSGA = 0;
	dma.TCD->BITER_ELINKNO = sizeof(tdm_tx_buffer) / 4;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_TX);
	update_responsibility = update_setup();
	dma.enable();

	I2S0_TCSR = I2S_TCSR_SR;
	I2S0_TCSR = I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;
	dma.attachInterrupt(isr);
}

// TODO: needs optimization...
static void memcpy_tdm_tx(uint32_t *dest, const uint32_t *src1, const uint32_t *src2)
{
	uint32_t i, in1, in2, out1, out2;

	for (i=0; i < AUDIO_BLOCK_SAMPLES/2; i++) {
		in1 = *src1++;
		in2 = *src2++;
		out1 = (in1 << 16) | (in2 & 0xFFFF);
		out2 = (in1 & 0xFFFF0000) | (in2 >> 16);
		*dest = out1;
		*(dest + 8) = out2;
		dest += 16;
	}
}

void AudioOutputTDM::isr(void)
{
	uint32_t *dest;
	const uint32_t *src1, *src2;
	uint32_t i, saddr;

	saddr = (uint32_t)(dma.TCD->SADDR);
	dma.clearInterrupt();
	if (saddr < (uint32_t)tdm_tx_buffer + sizeof(tdm_tx_buffer) / 2) {
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		dest = tdm_tx_buffer + AUDIO_BLOCK_SAMPLES*8;
	} else {
		// DMA is transmitting the second half of the buffer
		// so we must fill the first half
		dest = tdm_tx_buffer;
	}
	if (update_responsibility) AudioStream::update_all();
	for (i=0; i < 16; i += 2) {
		src1 = block_input[i] ? (uint32_t *)(block_input[i]->data) : zeros;
		src2 = block_input[i+1] ? (uint32_t *)(block_input[i+1]->data) : zeros;
		memcpy_tdm_tx(dest, src1, src2);
		dest++;
	}
	for (i=0; i < 16; i++) {
		if (block_input[i]) {
			release(block_input[i]);
			block_input[i] = NULL;
		}
	}
}


void AudioOutputTDM::update(void)
{
	audio_block_t *prev[16];
	unsigned int i;

	__disable_irq();
	for (i=0; i < 16; i++) {
		prev[i] = block_input[i];
		block_input[i] = receiveReadOnly(i);
	}
	__enable_irq();
	for (i=0; i < 16; i++) {
		if (prev[i]) release(prev[i]);
	}
}


// MCLK needs to be 48e6 / 1088 * 512 = 22.588235 MHz -> 44.117647 kHz sample rate
//
#if F_CPU == 96000000 || F_CPU == 48000000 || F_CPU == 24000000
  // PLL is at 96 MHz in these modes
  #define MCLK_MULT 4
  #define MCLK_DIV  17
#elif F_CPU == 72000000
  #define MCLK_MULT 16
  #define MCLK_DIV  51
#elif F_CPU == 120000000
  #define MCLK_MULT 16
  #define MCLK_DIV  85
#elif F_CPU == 144000000
  #define MCLK_MULT 8
  #define MCLK_DIV  51
#elif F_CPU == 168000000
  #define MCLK_MULT 16
  #define MCLK_DIV  119
#elif F_CPU == 180000000
  #define MCLK_MULT 32
  #define MCLK_DIV  255
  #define MCLK_SRC  0
#elif F_CPU == 192000000
  #define MCLK_MULT 2
  #define MCLK_DIV  17
#elif F_CPU == 216000000
  #define MCLK_MULT 16
  #define MCLK_DIV  153
  #define MCLK_SRC  0
#elif F_CPU == 240000000
  #define MCLK_MULT 8
  #define MCLK_DIV  85
#else
  #error "This CPU Clock Speed is not supported by the Audio library";
#endif

#ifndef MCLK_SRC
#if F_CPU >= 20000000
  #define MCLK_SRC  3  // the PLL
#else
  #define MCLK_SRC  0  // system clock
#endif
#endif

void AudioOutputTDM::config_tdm(void)
{
	SIM_SCGC6 |= SIM_SCGC6_I2S;
	SIM_SCGC7 |= SIM_SCGC7_DMA;
	SIM_SCGC6 |= SIM_SCGC6_DMAMUX;

	// if either transmitter or receiver is enabled, do nothing
	if (I2S0_TCSR & I2S_TCSR_TE) return;
	if (I2S0_RCSR & I2S_RCSR_RE) return;

	// enable MCLK output
	I2S0_MCR = I2S_MCR_MICS(MCLK_SRC) | I2S_MCR_MOE;
	while (I2S0_MCR & I2S_MCR_DUF) ;
	I2S0_MDR = I2S_MDR_FRACT((MCLK_MULT-1)) | I2S_MDR_DIVIDE((MCLK_DIV-1));

	// configure transmitter
	I2S0_TMR = 0;
	I2S0_TCR1 = I2S_TCR1_TFW(4);
	I2S0_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_BCP | I2S_TCR2_MSEL(1)
		| I2S_TCR2_BCD | I2S_TCR2_DIV(0);
	I2S0_TCR3 = I2S_TCR3_TCE;
	I2S0_TCR4 = I2S_TCR4_FRSZ(7) | I2S_TCR4_SYWD(0) | I2S_TCR4_MF
		| I2S_TCR4_FSE | I2S_TCR4_FSD;
	I2S0_TCR5 = I2S_TCR5_WNW(31) | I2S_TCR5_W0W(31) | I2S_TCR5_FBT(31);

	// configure receiver (sync'd to transmitter clocks)
	I2S0_RMR = 0;
	I2S0_RCR1 = I2S_RCR1_RFW(4);
	I2S0_RCR2 = I2S_RCR2_SYNC(1) | I2S_TCR2_BCP | I2S_RCR2_MSEL(1)
		| I2S_RCR2_BCD | I2S_RCR2_DIV(0);
	I2S0_RCR3 = I2S_RCR3_RCE;
	I2S0_RCR4 = I2S_RCR4_FRSZ(7) | I2S_RCR4_SYWD(0) | I2S_RCR4_MF
		| I2S_RCR4_FSE | I2S_RCR4_FSD;
	I2S0_RCR5 = I2S_RCR5_WNW(31) | I2S_RCR5_W0W(31) | I2S_RCR5_FBT(31);

	// configure pin mux for 3 clock signals
	CORE_PIN23_CONFIG = PORT_PCR_MUX(6); // pin 23, PTC2, I2S0_TX_FS (LRCLK)
	CORE_PIN9_CONFIG  = PORT_PCR_MUX(6); // pin  9, PTC3, I2S0_TX_BCLK
	CORE_PIN11_CONFIG = PORT_PCR_MUX(6); // pin 11, PTC6, I2S0_MCLK
}



#endif // KINETISK
