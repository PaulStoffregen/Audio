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
#if defined(__IMXRT1062__)
#include <Arduino.h>
#include "output_i2s2.h"
#include "memcpy_audio.h"
#include "utility/imxrt_hw.h"

audio_block_t * AudioOutputI2S2::block_left_1st = NULL;
audio_block_t * AudioOutputI2S2::block_right_1st = NULL;
audio_block_t * AudioOutputI2S2::block_left_2nd = NULL;
audio_block_t * AudioOutputI2S2::block_right_2nd = NULL;
uint16_t  AudioOutputI2S2::block_left_offset = 0;
uint16_t  AudioOutputI2S2::block_right_offset = 0;
bool AudioOutputI2S2::update_responsibility = false;
DMAChannel AudioOutputI2S2::dma(false);
DMAMEM __attribute__((aligned(32))) static uint32_t i2s2_tx_buffer[AUDIO_BLOCK_SAMPLES];

#include "utility/imxrt_hw.h"

void AudioOutputI2S2::begin(void)
{
	dma.begin(true); // Allocate the DMA channel first

	block_left_1st = NULL;
	block_right_1st = NULL;

	config_i2s();

	// if AudioInputI2S2 set I2S_TCSR_TE (for clock sync), disable it
	I2S2_TCSR = 0;
	while (I2S2_TCSR & I2S_TCSR_TE) ; //wait for transmit disabled

	CORE_PIN2_CONFIG  = 2;  //EMC_04, 2=SAI2_TX_DATA, page 428

	dma.TCD->SADDR = i2s2_tx_buffer;
	dma.TCD->SOFF = 2;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	dma.TCD->NBYTES_MLNO = 2;
	dma.TCD->SLAST = -sizeof(i2s2_tx_buffer);
	dma.TCD->DOFF = 0;
	dma.TCD->CITER_ELINKNO = sizeof(i2s2_tx_buffer) / 2;
	dma.TCD->DLASTSGA = 0;
	dma.TCD->BITER_ELINKNO = sizeof(i2s2_tx_buffer) / 2;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.TCD->DADDR = (void *)((uint32_t)&I2S2_TDR0 + 2);
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI2_TX);
	dma.enable();

	I2S2_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE | I2S_TCSR_FR;

	update_responsibility = update_setup();
	dma.attachInterrupt(isr);
}

void AudioOutputI2S2::isr(void)
{
	int16_t *dest;
	audio_block_t *blockL, *blockR;
	uint32_t saddr, offsetL, offsetR;

	saddr = (uint32_t)(dma.TCD->SADDR);
	dma.clearInterrupt();
	if (saddr < (uint32_t)i2s2_tx_buffer + sizeof(i2s2_tx_buffer) / 2) {
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		dest = (int16_t *)&i2s2_tx_buffer[AUDIO_BLOCK_SAMPLES/2];
		if (AudioOutputI2S2::update_responsibility) AudioStream::update_all();
	} else {
		// DMA is transmitting the second half of the buffer
		// so we must fill the first half
		dest = (int16_t *)i2s2_tx_buffer;
	}

	blockL = AudioOutputI2S2::block_left_1st;
	blockR = AudioOutputI2S2::block_right_1st;
	offsetL = AudioOutputI2S2::block_left_offset;
	offsetR = AudioOutputI2S2::block_right_offset;

	if (blockL && blockR) {
		memcpy_tointerleaveLR(dest, blockL->data + offsetL, blockR->data + offsetR);
		offsetL += AUDIO_BLOCK_SAMPLES / 2;
		offsetR += AUDIO_BLOCK_SAMPLES / 2;
	} else if (blockL) {
		memcpy_tointerleaveL(dest, blockL->data + offsetL);
		offsetL += AUDIO_BLOCK_SAMPLES / 2;
	} else if (blockR) {
		memcpy_tointerleaveR(dest, blockR->data + offsetR);
		offsetR += AUDIO_BLOCK_SAMPLES / 2;
	} else {
		memset(dest,0,AUDIO_BLOCK_SAMPLES * 2);		
	}

	#if IMXRT_CACHE_ENABLED >= 2		
	arm_dcache_flush_delete(dest, sizeof(i2s2_tx_buffer) / 2 );
	#endif	
	
	if (offsetL < AUDIO_BLOCK_SAMPLES) {
		AudioOutputI2S2::block_left_offset = offsetL;
	} else {
		AudioOutputI2S2::block_left_offset = 0;
		AudioStream::release(blockL);
		AudioOutputI2S2::block_left_1st = AudioOutputI2S2::block_left_2nd;
		AudioOutputI2S2::block_left_2nd = NULL;
	}
	if (offsetR < AUDIO_BLOCK_SAMPLES) {
		AudioOutputI2S2::block_right_offset = offsetR;
	} else {
		AudioOutputI2S2::block_right_offset = 0;
		AudioStream::release(blockR);
		AudioOutputI2S2::block_right_1st = AudioOutputI2S2::block_right_2nd;
		AudioOutputI2S2::block_right_2nd = NULL;
	}
}




void AudioOutputI2S2::update(void)
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

void AudioOutputI2S2::config_i2s(void)
{
	CCM_CCGR5 |= CCM_CCGR5_SAI2(CCM_CCGR_ON);

	// if either transmitter or receiver is enabled, do nothing
	if (I2S2_TCSR & I2S_TCSR_TE) return;
	if (I2S2_RCSR & I2S_RCSR_RE) return;
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

	// clear SAI2_CLK register locations
	CCM_CSCMR1 = (CCM_CSCMR1 & ~(CCM_CSCMR1_SAI2_CLK_SEL_MASK))
		   | CCM_CSCMR1_SAI2_CLK_SEL(2); // &0x03 // (0,1,2): PLL3PFD0, PLL5, PLL4,
	CCM_CS2CDR = (CCM_CS2CDR & ~(CCM_CS2CDR_SAI2_CLK_PRED_MASK | CCM_CS2CDR_SAI2_CLK_PODF_MASK))
		   | CCM_CS2CDR_SAI2_CLK_PRED(n1-1)
		   | CCM_CS2CDR_SAI2_CLK_PODF(n2-1);
	IOMUXC_GPR_GPR1 = (IOMUXC_GPR_GPR1 & ~(IOMUXC_GPR_GPR1_SAI2_MCLK3_SEL_MASK))
			| (IOMUXC_GPR_GPR1_SAI2_MCLK_DIR | IOMUXC_GPR_GPR1_SAI2_MCLK3_SEL(0));	//Select MCLK

	CORE_PIN33_CONFIG = 2;  //EMC_07, 2=SAI2_MCLK
	CORE_PIN4_CONFIG  = 2;  //EMC_06, 2=SAI2_TX_BCLK
	CORE_PIN3_CONFIG  = 2;  //EMC_05, 2=SAI2_TX_SYNC, page 429

	int rsync = 1;
	int tsync = 0;

	I2S2_TMR = 0;
	//I2S2_TCSR = (1<<25); //Reset
	I2S2_TCR1 = I2S_TCR1_RFW(1);
	I2S2_TCR2 = I2S_TCR2_SYNC(tsync) | I2S_TCR2_BCP // sync=0; tx is async;
		| (I2S_TCR2_BCD | I2S_TCR2_DIV((1)) | I2S_TCR2_MSEL(1));
	I2S2_TCR3 = I2S_TCR3_TCE;
	I2S2_TCR4 = I2S_TCR4_FRSZ((2-1)) | I2S_TCR4_SYWD((32-1)) | I2S_TCR4_MF
		| I2S_TCR4_FSD | I2S_TCR4_FSE | I2S_TCR4_FSP;
	I2S2_TCR5 = I2S_TCR5_WNW((32-1)) | I2S_TCR5_W0W((32-1)) | I2S_TCR5_FBT((32-1));

	I2S2_RMR = 0;
	//I2S2_RCSR = (1<<25); //Reset
	I2S2_RCR1 = I2S_RCR1_RFW(1);
	I2S2_RCR2 = I2S_RCR2_SYNC(rsync) | I2S_RCR2_BCP  // sync=0; rx is async;
		| (I2S_RCR2_BCD | I2S_RCR2_DIV((1)) | I2S_RCR2_MSEL(1));
	I2S2_RCR3 = I2S_RCR3_RCE;
	I2S2_RCR4 = I2S_RCR4_FRSZ((2-1)) | I2S_RCR4_SYWD((32-1)) | I2S_RCR4_MF
		| I2S_RCR4_FSE | I2S_RCR4_FSP | I2S_RCR4_FSD;
	I2S2_RCR5 = I2S_RCR5_WNW((32-1)) | I2S_RCR5_W0W((32-1)) | I2S_RCR5_FBT((32-1));

}

/******************************************************************/
#if 0
void AudioOutputI2S2slave::begin(void)
{
	dma.begin(true); // Allocate the DMA channel first

	//pinMode(2, OUTPUT);
	block_left_1st = NULL;
	block_right_1st = NULL;

	AudioOutputI2S2slave::config_i2s();

	CORE_PIN2_CONFIG  = 2;  //2:TX_DATA0
	//CORE_PIN33_CONFIG = 2;  //2:RX_DATA0

	dma.TCD->SADDR = i2s2_tx_buffer;
	dma.TCD->SOFF = 2;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	dma.TCD->NBYTES_MLNO = 2;
	dma.TCD->SLAST = -sizeof(i2s2_tx_buffer);
	dma.TCD->DADDR = (void *)((uint32_t)&I2S2_TDR0 + 2);
	dma.TCD->DOFF = 0;
	dma.TCD->CITER_ELINKNO = sizeof(i2s2_tx_buffer) / 2;
	dma.TCD->DLASTSGA = 0;
	dma.TCD->BITER_ELINKNO = sizeof(i2s2_tx_buffer) / 2;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI2_TX);

	update_responsibility = update_setup();
	dma.enable();
	dma.attachInterrupt(isr);
}

void AudioOutputI2S2slave::config_i2s(void)
{


	if (I2S2_TCSR & I2S_TCSR_TE) return;
	if (I2S2_TCSR & I2S_RCSR_RE) return;

	CCM_CCGR5 |= CCM_CCGR5_SAI2(CCM_CCGR_ON);
/*
	CCM_CSCMR1 = (CCM_CSCMR1 & ~(CCM_CSCMR1_SAI2_CLK_SEL_MASK))
		   | CCM_CSCMR1_SAI2_CLK_SEL(2); // &0x03 // (0,1,2): PLL3PFD0, PLL5, PLL4,
	CCM_CS2CDR = (CCM_CS2CDR & ~(CCM_CS2CDR_SAI2_CLK_PRED_MASK | CCM_CS2CDR_SAI2_CLK_PODF_MASK))
		   | CCM_CS2CDR_SAI2_CLK_PRED(n1-1) | CCM_CS2CDR_SAI2_CLK_PODF(n2-1);
*/
//	IOMUXC_GPR_GPR1 = (IOMUXC_GPR_GPR1 & ~(IOMUXC_GPR_GPR1_SAI2_MCLK3_SEL_MASK | ((uint32_t)(1<<19)) ))
//			/*| (IOMUXC_GPR_GPR1_SAI2_MCLK_DIR*/ | IOMUXC_GPR_GPR1_SAI2_MCLK3_SEL(0);	//Select MCLK

	CORE_PIN5_CONFIG  = 2;  //2:MCLK
	CORE_PIN4_CONFIG  = 2;  //2:TX_BCLK
	CORE_PIN3_CONFIG  = 2;  //2:TX_SYNC
	int rsync = 1;
	int tsync = 0;

	// configure transmitter
	I2S2_TMR = 0;
	I2S2_TCR1 = I2S_TCR1_RFW(1);  // watermark at half fifo size
	I2S2_TCR2 = I2S_TCR2_SYNC(tsync) | I2S_TCR2_BCP;
	I2S2_TCR3 = I2S_TCR3_TCE;
	I2S2_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(31) | I2S_TCR4_MF
		    | I2S_TCR4_FSE | I2S_TCR4_FSP;
	I2S2_TCR5 = I2S_TCR5_WNW(31) | I2S_TCR5_W0W(31) | I2S_TCR5_FBT(31);

	// configure receiver
	I2S2_TMR = 0;
	I2S2_TCR1 = I2S_RCR1_RFW(1);
	I2S2_TCR2 = I2S_RCR2_SYNC(rsync) | I2S_TCR2_BCP;
	I2S2_TCR3 = I2S_RCR3_RCE;
	I2S2_TCR4 = I2S_RCR4_FRSZ(1) | I2S_RCR4_SYWD(31) | I2S_RCR4_MF
		    | I2S_RCR4_FSE | I2S_RCR4_FSP | I2S_RCR4_FSD;
	I2S2_TCR5 = I2S_RCR5_WNW(31) | I2S_RCR5_W0W(31) | I2S_RCR5_FBT(31);
	
}
#endif
#endif //defined(__IMXRT1062__)
