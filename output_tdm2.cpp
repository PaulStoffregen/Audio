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

#if defined(__IMXRT1062__)
#include <Arduino.h>
#include "output_tdm2.h"
#include "memcpy_audio.h"
#include "utility/imxrt_hw.h"

audio_block_t * AudioOutputTDM2::block_input[16] = {
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
};
bool AudioOutputTDM2::update_responsibility = false;
DMAChannel AudioOutputTDM2::dma(false);
DMAMEM __attribute__((aligned(32)))
static uint32_t zeros[AUDIO_BLOCK_SAMPLES/2];
DMAMEM __attribute__((aligned(32)))
static uint32_t tdm_tx_buffer[AUDIO_BLOCK_SAMPLES*16];


void AudioOutputTDM2::begin(void)
{
	dma.begin(true); // Allocate the DMA channel first

	for (int i=0; i < 16; i++) {
		block_input[i] = nullptr;
	}

	// TODO: should we set & clear the I2S_TCSR_SR bit here?
	config_tdm();

	CORE_PIN2_CONFIG  = 2;  //2:TX_DATA0

	dma.TCD->SADDR = tdm_tx_buffer;
	dma.TCD->SOFF = 4;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
	dma.TCD->NBYTES_MLNO = 4;
	dma.TCD->SLAST = -sizeof(tdm_tx_buffer);
	dma.TCD->DADDR = &I2S2_TDR0;
	dma.TCD->DOFF = 0;
	dma.TCD->CITER_ELINKNO = sizeof(tdm_tx_buffer) / 4;
	dma.TCD->DLASTSGA = 0;
	dma.TCD->BITER_ELINKNO = sizeof(tdm_tx_buffer) / 4;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI2_TX);

	update_responsibility = update_setup();
	dma.enable();

	//I2S2_RCSR |= I2S_RCSR_RE;
	I2S2_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;

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

		in1 = *src1++;
		in2 = *src2++;
		out1 = (in1 << 16) | (in2 & 0xFFFF);
		out2 = (in1 & 0xFFFF0000) | (in2 >> 16);
		*(dest + 16)= out1;
		*(dest + 24) = out2;

		dest += 32;
	}
}

void AudioOutputTDM2::isr(void)
{
	uint32_t *dest, *dc;
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
	dc = dest;
	for (i=0; i < 16; i += 2) {
		src1 = block_input[i] ? (uint32_t *)(block_input[i]->data) : zeros;
		src2 = block_input[i+1] ? (uint32_t *)(block_input[i+1]->data) : zeros;
		memcpy_tdm_tx(dest, src1, src2);
		dest++;
	}

	#if IMXRT_CACHE_ENABLED >= 2
	arm_dcache_flush_delete(dc, sizeof(tdm_tx_buffer) / 2 );
	#endif

	for (i=0; i < 16; i++) {
		if (block_input[i]) {
			release(block_input[i]);
			block_input[i] = nullptr;
		}
	}
}


void AudioOutputTDM2::update(void)
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

void AudioOutputTDM2::config_tdm(void)
{
	CCM_CCGR5 |= CCM_CCGR5_SAI2(CCM_CCGR_ON);

	// if either transmitter or receiver is enabled, do nothing
	if (I2S2_TCSR & I2S_TCSR_TE) return;
	if (I2S2_RCSR & I2S_RCSR_RE) return;
//PLL:
	int fs = AUDIO_SAMPLE_RATE_EXACT; //176.4 khZ
	// PLL between 27*24 = 648MHz und 54*24=1296MHz
	int n1 = 4; //SAI prescaler 4 => (n1*n2) = multiple of 4
	int n2 = 1 + (24000000 * 27) / (fs * 256 * n1);

	double C = ((double)fs * 256 * n1 * n2) / 24000000;
	int c0 = C;
	int c2 = 10000;
	int c1 = C * c2 - (c0 * c2);
	set_audioClock(c0, c1, c2);
	// clear SAI1_CLK register locations

	CCM_CSCMR1 = (CCM_CSCMR1 & ~(CCM_CSCMR1_SAI2_CLK_SEL_MASK))
		   | CCM_CSCMR1_SAI2_CLK_SEL(2); // &0x03 // (0,1,2): PLL3PFD0, PLL5, PLL4

	n1 = n1 / 2; //Double Speed for TDM

	CCM_CS2CDR = (CCM_CS2CDR & ~(CCM_CS2CDR_SAI2_CLK_PRED_MASK | CCM_CS2CDR_SAI2_CLK_PODF_MASK))
		   | CCM_CS2CDR_SAI2_CLK_PRED(n1-1) // &0x07
		   | CCM_CS2CDR_SAI2_CLK_PODF(n2-1); // &0x3f

	IOMUXC_GPR_GPR1 = (IOMUXC_GPR_GPR1 & ~(IOMUXC_GPR_GPR1_SAI2_MCLK3_SEL_MASK))
			| (IOMUXC_GPR_GPR1_SAI2_MCLK_DIR | IOMUXC_GPR_GPR1_SAI2_MCLK3_SEL(0));	//Select MCLK

	// configure transmitter
	int rsync = 1;
	int tsync = 0;

	I2S2_TMR = 0;
	I2S2_TCR1 = I2S_TCR1_RFW(4);
	I2S2_TCR2 = I2S_TCR2_SYNC(tsync) | I2S_TCR2_BCP | I2S_TCR2_MSEL(1)
		| I2S_TCR2_BCD | I2S_TCR2_DIV(0);
	I2S2_TCR3 = I2S_TCR3_TCE;
	I2S2_TCR4 = I2S_TCR4_FRSZ(7) | I2S_TCR4_SYWD(0) | I2S_TCR4_MF
		| I2S_TCR4_FSE | I2S_TCR4_FSD;
	I2S2_TCR5 = I2S_TCR5_WNW(31) | I2S_TCR5_W0W(31) | I2S_TCR5_FBT(31);

	// configure receiver (sync'd to transmitter clocks)
	I2S2_RMR = 0;
	I2S2_RCR1 = I2S_RCR1_RFW(4);
	I2S2_RCR2 = I2S_RCR2_SYNC(rsync) | I2S_TCR2_BCP | I2S_RCR2_MSEL(1)
		| I2S_RCR2_BCD | I2S_RCR2_DIV(0);
	I2S2_RCR3 = I2S_RCR3_RCE;
	I2S2_RCR4 = I2S_RCR4_FRSZ(7) | I2S_RCR4_SYWD(0) | I2S_RCR4_MF
		| I2S_RCR4_FSE | I2S_RCR4_FSD;
	I2S2_RCR5 = I2S_RCR5_WNW(31) | I2S_RCR5_W0W(31) | I2S_RCR5_FBT(31);

	CORE_PIN33_CONFIG = 2;  //2:MCLK
	CORE_PIN4_CONFIG  = 2;  //2:TX_BCLK
	CORE_PIN3_CONFIG  = 2;  //2:TX_SYNC
}

#endif
