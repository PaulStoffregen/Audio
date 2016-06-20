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

#include "output_i2s_quad.h"
#include "memcpy_audio.h"

#if defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)

audio_block_t * AudioOutputI2SQuad::block_ch1_1st = NULL;
audio_block_t * AudioOutputI2SQuad::block_ch2_1st = NULL;
audio_block_t * AudioOutputI2SQuad::block_ch3_1st = NULL;
audio_block_t * AudioOutputI2SQuad::block_ch4_1st = NULL;
audio_block_t * AudioOutputI2SQuad::block_ch1_2nd = NULL;
audio_block_t * AudioOutputI2SQuad::block_ch2_2nd = NULL;
audio_block_t * AudioOutputI2SQuad::block_ch3_2nd = NULL;
audio_block_t * AudioOutputI2SQuad::block_ch4_2nd = NULL;
uint16_t  AudioOutputI2SQuad::ch1_offset = 0;
uint16_t  AudioOutputI2SQuad::ch2_offset = 0;
uint16_t  AudioOutputI2SQuad::ch3_offset = 0;
uint16_t  AudioOutputI2SQuad::ch4_offset = 0;
//audio_block_t * AudioOutputI2SQuad::inputQueueArray[4];
bool AudioOutputI2SQuad::update_responsibility = false;
DMAMEM static uint32_t i2s_tx_buffer[AUDIO_BLOCK_SAMPLES*2];
DMAChannel AudioOutputI2SQuad::dma(false);

static const uint32_t zerodata[AUDIO_BLOCK_SAMPLES/4] = {0};

void AudioOutputI2SQuad::begin(void)
{
#if 1
	dma.begin(true); // Allocate the DMA channel first

	block_ch1_1st = NULL;
	block_ch2_1st = NULL;
	block_ch3_1st = NULL;
	block_ch4_1st = NULL;

	// TODO: can we call normal config_i2s, and then just enable the extra output?
	config_i2s();
	CORE_PIN22_CONFIG = PORT_PCR_MUX(6); // pin 22, PTC1, I2S0_TXD0 -> ch1 & ch2
	CORE_PIN15_CONFIG = PORT_PCR_MUX(6); // pin 15, PTC0, I2S0_TXD1 -> ch3 & ch4

	dma.TCD->SADDR = i2s_tx_buffer;
	dma.TCD->SOFF = 2;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1) | DMA_TCD_ATTR_DMOD(3);
	dma.TCD->NBYTES_MLNO = 4;
	dma.TCD->SLAST = -sizeof(i2s_tx_buffer);
	dma.TCD->DADDR = &I2S0_TDR0;
	dma.TCD->DOFF = 4;
	dma.TCD->CITER_ELINKNO = sizeof(i2s_tx_buffer) / 4;
	dma.TCD->DLASTSGA = 0;
	dma.TCD->BITER_ELINKNO = sizeof(i2s_tx_buffer) / 4;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_TX);
	update_responsibility = update_setup();
	dma.enable();

	I2S0_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE | I2S_TCSR_FR;
	dma.attachInterrupt(isr);
#endif
}

void AudioOutputI2SQuad::isr(void)
{
	uint32_t saddr;
	const int16_t *src1, *src2, *src3, *src4;
	const int16_t *zeros = (const int16_t *)zerodata;
	int16_t *dest;

	saddr = (uint32_t)(dma.TCD->SADDR);
	dma.clearInterrupt();
	if (saddr < (uint32_t)i2s_tx_buffer + sizeof(i2s_tx_buffer) / 2) {
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		dest = (int16_t *)&i2s_tx_buffer[AUDIO_BLOCK_SAMPLES];
		if (update_responsibility) update_all();
	} else {
		dest = (int16_t *)i2s_tx_buffer;
	}

	src1 = (block_ch1_1st) ? block_ch1_1st->data + ch1_offset : zeros;
	src2 = (block_ch2_1st) ? block_ch2_1st->data + ch2_offset : zeros;
	src3 = (block_ch3_1st) ? block_ch3_1st->data + ch3_offset : zeros;
	src4 = (block_ch4_1st) ? block_ch4_1st->data + ch4_offset : zeros;

	// TODO: fast 4-way interleaved memcpy...
#if 1
	memcpy_tointerleaveQuad(dest, src1, src2, src3, src4);
#else
	for (int i=0; i < AUDIO_BLOCK_SAMPLES/2; i++) {
		*dest++ = *src1++;
		*dest++ = *src3++;
		*dest++ = *src2++;
		*dest++ = *src4++;
	}
#endif

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
}


void AudioOutputI2SQuad::update(void)
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
}


// MCLK needs to be 48e6 / 1088 * 256 = 11.29411765 MHz -> 44.117647 kHz sample rate
//
#if F_CPU == 96000000 || F_CPU == 48000000 || F_CPU == 24000000
  // PLL is at 96 MHz in these modes
  #define MCLK_MULT 2
  #define MCLK_DIV  17
#elif F_CPU == 72000000
  #define MCLK_MULT 8
  #define MCLK_DIV  51
#elif F_CPU == 120000000
  #define MCLK_MULT 8
  #define MCLK_DIV  85
#elif F_CPU == 144000000
  #define MCLK_MULT 4
  #define MCLK_DIV  51
#elif F_CPU == 168000000
  #define MCLK_MULT 8
  #define MCLK_DIV  119
#elif F_CPU == 180000000
  #define MCLK_MULT 16
  #define MCLK_DIV  255
  #define MCLK_SRC  0
#elif F_CPU == 192000000
  #define MCLK_MULT 1
  #define MCLK_DIV  17
#elif F_CPU == 216000000
  #define MCLK_MULT 8
  #define MCLK_DIV  153
  #define MCLK_SRC  0
#elif F_CPU == 240000000
  #define MCLK_MULT 4
  #define MCLK_DIV  85
#elif F_CPU == 16000000
  #define MCLK_MULT 12
  #define MCLK_DIV  17
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

void AudioOutputI2SQuad::config_i2s(void)
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
	I2S0_TCR1 = I2S_TCR1_TFW(1);  // watermark at half fifo size
	I2S0_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_BCP | I2S_TCR2_MSEL(1)
		| I2S_TCR2_BCD | I2S_TCR2_DIV(3);
	I2S0_TCR3 = I2S_TCR3_TCE_2CH;
	I2S0_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(15) | I2S_TCR4_MF
		| I2S_TCR4_FSE | I2S_TCR4_FSP | I2S_TCR4_FSD;
	I2S0_TCR5 = I2S_TCR5_WNW(15) | I2S_TCR5_W0W(15) | I2S_TCR5_FBT(15);

	// configure receiver (sync'd to transmitter clocks)
	I2S0_RMR = 0;
	I2S0_RCR1 = I2S_RCR1_RFW(1);
	I2S0_RCR2 = I2S_RCR2_SYNC(1) | I2S_TCR2_BCP | I2S_RCR2_MSEL(1)
		| I2S_RCR2_BCD | I2S_RCR2_DIV(3);
	I2S0_RCR3 = I2S_RCR3_RCE_2CH;
	I2S0_RCR4 = I2S_RCR4_FRSZ(1) | I2S_RCR4_SYWD(15) | I2S_RCR4_MF
		| I2S_RCR4_FSE | I2S_RCR4_FSP | I2S_RCR4_FSD;
	I2S0_RCR5 = I2S_RCR5_WNW(15) | I2S_RCR5_W0W(15) | I2S_RCR5_FBT(15);

	// configure pin mux for 3 clock signals
	CORE_PIN23_CONFIG = PORT_PCR_MUX(6); // pin 23, PTC2, I2S0_TX_FS (LRCLK)
	CORE_PIN9_CONFIG  = PORT_PCR_MUX(6); // pin  9, PTC3, I2S0_TX_BCLK
	CORE_PIN11_CONFIG = PORT_PCR_MUX(6); // pin 11, PTC6, I2S0_MCLK
}


#else // not __MK20DX256__


void AudioOutputI2SQuad::begin(void)
{
}

void AudioOutputI2SQuad::update(void)
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
}

#endif
