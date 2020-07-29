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
// Frank B

#if defined(__IMXRT1052__) || defined(__IMXRT1062__)
#include <Arduino.h>
#include "output_mqs.h"
#include "memcpy_audio.h"
#include "utility/imxrt_hw.h"

audio_block_t * AudioOutputMQS::block_left_1st = NULL;
audio_block_t * AudioOutputMQS::block_right_1st = NULL;
audio_block_t * AudioOutputMQS::block_left_2nd = NULL;
audio_block_t * AudioOutputMQS::block_right_2nd = NULL;
uint16_t  AudioOutputMQS::block_left_offset = 0;
uint16_t  AudioOutputMQS::block_right_offset = 0;
bool AudioOutputMQS::update_responsibility = false;
DMAChannel AudioOutputMQS::dma(false);
DMAMEM __attribute__((aligned(32)))
static uint32_t I2S3_tx_buffer[AUDIO_BLOCK_SAMPLES];


void AudioOutputMQS::begin(void)
{
	dma.begin(true); // Allocate the DMA channel first

	block_left_1st = NULL;
	block_right_1st = NULL;

	config_i2s();

	CORE_PIN10_CONFIG = 2;//B0_00 MQS_RIGHT
	CORE_PIN12_CONFIG = 2;//B0_01 MQS_LEFT

	dma.TCD->SADDR = I2S3_tx_buffer;
	dma.TCD->SOFF = 2;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	dma.TCD->NBYTES_MLNO = 2;
	dma.TCD->SLAST = -sizeof(I2S3_tx_buffer);
	dma.TCD->DOFF = 0;
	dma.TCD->CITER_ELINKNO = sizeof(I2S3_tx_buffer) / 2;
	dma.TCD->DLASTSGA = 0;
	dma.TCD->BITER_ELINKNO = sizeof(I2S3_tx_buffer) / 2;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.TCD->DADDR = (void *)((uint32_t)&I2S3_TDR0 + 0);
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI3_TX);

	I2S3_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;
	update_responsibility = update_setup();
	dma.attachInterrupt(isr);
	dma.enable();
}

void AudioOutputMQS::isr(void)
{
	int16_t *dest;
	audio_block_t *blockL, *blockR;
	uint32_t saddr, offsetL, offsetR;

	saddr = (uint32_t)(dma.TCD->SADDR);
	dma.clearInterrupt();
	if (saddr < (uint32_t)I2S3_tx_buffer + sizeof(I2S3_tx_buffer) / 2) {
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		dest = (int16_t *)&I2S3_tx_buffer[AUDIO_BLOCK_SAMPLES/2];
		if (AudioOutputMQS::update_responsibility) AudioStream::update_all();
	} else {
		// DMA is transmitting the second half of the buffer
		// so we must fill the first half
		dest = (int16_t *)I2S3_tx_buffer;
	}

	blockL = AudioOutputMQS::block_left_1st;
	blockR = AudioOutputMQS::block_right_1st;
	offsetL = AudioOutputMQS::block_left_offset;
	offsetR = AudioOutputMQS::block_right_offset;

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
		memset(dest,0, sizeof(I2S3_tx_buffer) / 2);		
	}
	
	#if IMXRT_CACHE_ENABLED >= 2		
	arm_dcache_flush_delete(dest, sizeof(I2S3_tx_buffer) / 2 );
	#endif
	
	if (offsetL < AUDIO_BLOCK_SAMPLES) {
		AudioOutputMQS::block_left_offset = offsetL;
	} else {
		AudioOutputMQS::block_left_offset = 0;
		AudioStream::release(blockL);
		AudioOutputMQS::block_left_1st = AudioOutputMQS::block_left_2nd;
		AudioOutputMQS::block_left_2nd = NULL;
	}
	if (offsetR < AUDIO_BLOCK_SAMPLES) {
		AudioOutputMQS::block_right_offset = offsetR;
	} else {
		AudioOutputMQS::block_right_offset = 0;
		AudioStream::release(blockR);
		AudioOutputMQS::block_right_1st = AudioOutputMQS::block_right_2nd;
		AudioOutputMQS::block_right_2nd = NULL;
	}

}




void AudioOutputMQS::update(void)
{
	// null audio device: discard all incoming data
	//if (!active) return;
	//audio_block_t *block = receiveReadOnly();
	//if (block) release(block);
	//digitalWriteFast(13, LOW);
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

void AudioOutputMQS::config_i2s(void)
{
	CCM_CCGR5 |= CCM_CCGR5_SAI3(CCM_CCGR_ON);
	CCM_CCGR0 |= CCM_CCGR0_MQS_HMCLK(CCM_CCGR_ON);

//PLL:
//TODO: Check if frequencies are correct!

	int fs = AUDIO_SAMPLE_RATE_EXACT;
	int oversample = 64*8;
	// PLL between 27*24 = 648MHz und 54*24=1296MHz
	int n1 = 4; //SAI prescaler 4 => (n1*n2) = multiple of 4
	int n2 = 1 + (24000000 * 27) / (fs * oversample * n1);

	double C = ((double)fs * oversample * n1 * n2) / 24000000;
	int c0 = C;
	int c2 = 10000;
	int c1 = C * c2 - (c0 * c2);

	set_audioClock(c0, c1, c2);

	CCM_CSCMR1 = (CCM_CSCMR1 & ~(CCM_CSCMR1_SAI3_CLK_SEL_MASK))
		   | CCM_CSCMR1_SAI3_CLK_SEL(2); // &0x03 // (0,1,2): PLL3PFD0, PLL5, PLL4,
	CCM_CS1CDR = (CCM_CS1CDR & ~(CCM_CS1CDR_SAI3_CLK_PRED_MASK | CCM_CS1CDR_SAI3_CLK_PODF_MASK))
		   | CCM_CS1CDR_SAI3_CLK_PRED(n1-1)
		   | CCM_CS1CDR_SAI3_CLK_PODF(n2-1);
	IOMUXC_GPR_GPR1 = (IOMUXC_GPR_GPR1 & ~(IOMUXC_GPR_GPR1_SAI3_MCLK3_SEL_MASK))
			| (IOMUXC_GPR_GPR1_SAI3_MCLK_DIR | IOMUXC_GPR_GPR1_SAI3_MCLK3_SEL(0));	//Select MCLK

	IOMUXC_GPR_GPR2 = (IOMUXC_GPR_GPR2 & ~(IOMUXC_GPR_GPR2_MQS_OVERSAMPLE | IOMUXC_GPR_GPR2_MQS_CLK_DIV_MASK))
			| IOMUXC_GPR_GPR2_MQS_EN | IOMUXC_GPR_GPR2_MQS_OVERSAMPLE | IOMUXC_GPR_GPR2_MQS_CLK_DIV(0);

	if (I2S3_TCSR & I2S_TCSR_TE) return;

	I2S3_TMR = 0;
//	I2S3_TCSR = (1<<25); //Reset
	I2S3_TCR1 = I2S_TCR1_RFW(1);
	I2S3_TCR2 = I2S_TCR2_SYNC(0) /*| I2S_TCR2_BCP*/ // sync=0; tx is async;
		    | (I2S_TCR2_BCD | I2S_TCR2_DIV((7)) | I2S_TCR2_MSEL(1));
	I2S3_TCR3 = I2S_TCR3_TCE;
	I2S3_TCR4 = I2S_TCR4_FRSZ((2-1)) | I2S_TCR4_SYWD((16-1)) | I2S_TCR4_MF | I2S_TCR4_FSD /*| I2S_TCR4_FSE*/ /* | I2S_TCR4_FSP */;
	I2S3_TCR5 = I2S_TCR5_WNW((16-1)) | I2S_TCR5_W0W((16-1)) | I2S_TCR5_FBT((16-1));
}

#endif //defined(__IMXRT1062__)
