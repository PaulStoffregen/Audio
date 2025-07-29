/* SPDIF for Teensy 3.X
 * Copyright (c) 2015, Frank Bösing, f.boesing@gmx.de,
 * Thanks to KPC & Paul Stoffregen!
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

#if defined(__IMXRT1052__) || defined(__IMXRT1062__)

#include "output_spdif2.h"

audio_block_t * AudioOutputSPDIF2::block_left_1st = NULL;
audio_block_t * AudioOutputSPDIF2::block_right_1st = NULL;
audio_block_t * AudioOutputSPDIF2::block_left_2nd = NULL;
audio_block_t * AudioOutputSPDIF2::block_right_2nd = NULL;
uint16_t  AudioOutputSPDIF2::block_left_offset = 0;
uint16_t  AudioOutputSPDIF2::block_right_offset = 0;
bool AudioOutputSPDIF2::update_responsibility = false;
DMAChannel AudioOutputSPDIF2::dma(false);
int32_t  AudioOutputSPDIF2::vucp;

DMAMEM __attribute__((aligned(32)))
int32_t AudioOutputSPDIF2::tx_buffer[AUDIO_BLOCK_SAMPLES * 4]; //2 KB

FLASHMEM
void AudioOutputSPDIF2::begin(void)
{

	memset(tx_buffer,0,sizeof tx_buffer); // ensure we start with silence
	dma.begin(true); // Allocate the DMA channel first

	block_left_1st = NULL;
	block_right_1st = NULL;

	// TODO: should we set & clear the I2S_TCSR_SR bit here?
	config_SPDIF();

	CORE_PIN2_CONFIG  = 2;  //2:TX_DATA0
	const int nbytes_mlno = 2 * 4; // 8 Bytes per minor loop

	dma.TCD->SADDR = tx_buffer;
	dma.TCD->SOFF = 4;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
	dma.TCD->NBYTES_MLNO = nbytes_mlno;
	dma.TCD->SLAST = -sizeof(SPDIF_tx_buffer);
	dma.TCD->DADDR = &I2S2_TDR0;
	dma.TCD->DOFF = 0;
	dma.TCD->CITER_ELINKNO = sizeof(tx_buffer) / nbytes_mlno;
	dma.TCD->DLASTSGA = 0;
	dma.TCD->BITER_ELINKNO = sizeof(tx_buffer) / nbytes_mlno;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI2_TX);
	update_responsibility = update_setup();
	dma.enable();

	I2S2_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE | I2S_TCSR_FR;
	dma.attachInterrupt(isr);
}

FLASHMEM
void AudioOutputSPDIF2::config_SPDIF(void)
{
	CCM_CCGR5 |= CCM_CCGR5_SAI2(CCM_CCGR_ON);
//PLL:
	int fs = AUDIO_SAMPLE_RATE_EXACT;
	// PLL between 27*24 = 648MHz und 54*24=1296MHz
	int n1 = 4; //SAI prescaler 4 => (n1*n2) = multiple of 4
	int n2 = 1 + (24000000 * 27) / (fs * 256 * n1);

	double C = ((double)fs * 256 * n1 * n2) / 24000000;
	int c0 = C;
	int c2 = 10000;
	int c1 = C * c2 - (c0 * c2);
	set_audioClock(c0, c1, c2);

	CCM_CSCMR1 = (CCM_CSCMR1 & ~(CCM_CSCMR1_SAI2_CLK_SEL_MASK))
		   | CCM_CSCMR1_SAI2_CLK_SEL(2); // &0x03 // (0,1,2): PLL3PFD0, PLL5, PLL4,
	CCM_CS2CDR = (CCM_CS2CDR & ~(CCM_CS2CDR_SAI2_CLK_PRED_MASK | CCM_CS2CDR_SAI2_CLK_PODF_MASK))
		   | CCM_CS2CDR_SAI2_CLK_PRED(n1-1)
		   | CCM_CS2CDR_SAI2_CLK_PODF(n2-1);
	IOMUXC_GPR_GPR1 = (IOMUXC_GPR_GPR1 & ~(IOMUXC_GPR_GPR1_SAI2_MCLK3_SEL_MASK))
			| (IOMUXC_GPR_GPR1_SAI2_MCLK_DIR | IOMUXC_GPR_GPR1_SAI2_MCLK3_SEL(0));	//Select MCLK

	// configure transmitter
	I2S2_TMR = 0;
	I2S2_TCR1 = I2S_TCR1_RFW(0);  // watermark
	I2S2_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_MSEL(1) | I2S_TCR2_BCD | I2S_TCR2_DIV(0);
	I2S2_TCR3 = I2S_TCR3_TCE;

	//4 Words per Frame 32 Bit Word-Length -> 128 Bit Frame-Length, MSB First:
	I2S2_TCR4 = I2S_TCR4_FRSZ(3) | I2S_TCR4_SYWD(0) | I2S_TCR4_MF | I2S_TCR4_FSP | I2S_TCR4_FSD;
	I2S2_TCR5 = I2S_TCR5_WNW(31) | I2S_TCR5_W0W(31) | I2S_TCR5_FBT(31);
#if 0
	//debug only:
	CORE_PIN5_CONFIG  = 2;  //2:MCLK	11.43MHz
	CORE_PIN4_CONFIG  = 2;  //2:TX_BCLK	5 MHz
	CORE_PIN3_CONFIG  = 2;  //2:TX_SYNC	44.1 KHz
#endif
}



#endif
