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

//Adapted to PT8211, Frank Bösing, Ben-Rheinland

#if defined(__IMXRT1052__) || defined(__IMXRT1062__)
#include <Arduino.h>
#include "output_pt8211_2.h"
#include "memcpy_audio.h"
#include "utility/imxrt_hw.h"

audio_block_t * AudioOutputPT8211_2::block_left_1st = NULL;
audio_block_t * AudioOutputPT8211_2::block_right_1st = NULL;
audio_block_t * AudioOutputPT8211_2::block_left_2nd = NULL;
audio_block_t * AudioOutputPT8211_2::block_right_2nd = NULL;
uint16_t  AudioOutputPT8211_2::block_left_offset = 0;
uint16_t  AudioOutputPT8211_2::block_right_offset = 0;
bool AudioOutputPT8211_2::update_responsibility = false;
#if defined(AUDIO_PT8211_OVERSAMPLING)
	static uint32_t i2s_tx_buffer[AUDIO_BLOCK_SAMPLES*4];
#else
	static uint32_t i2s_tx_buffer[AUDIO_BLOCK_SAMPLES];
#endif
DMAChannel AudioOutputPT8211_2::dma(false);

PROGMEM
void AudioOutputPT8211_2::begin(void)
{
	dma.begin(true); // Allocate the DMA channel first

	block_left_1st = NULL;
	block_right_1st = NULL;

	// TODO: should we set & clear the I2S_TCSR_SR bit here?
	config_i2s();
	CORE_PIN2_CONFIG  = 2;  //2:TX_DATA0
	
	dma.TCD->SADDR = i2s_tx_buffer;
	dma.TCD->SOFF = 2;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	dma.TCD->NBYTES_MLNO = 2;
	dma.TCD->SLAST = -sizeof(i2s_tx_buffer);
	dma.TCD->DOFF = 0;
	dma.TCD->CITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;
	dma.TCD->DLASTSGA = 0;
	dma.TCD->BITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.TCD->DADDR = (void *)((uint32_t)&I2S2_TDR0);
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI2_TX);

	I2S2_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;

	update_responsibility = update_setup();
	dma.attachInterrupt(isr);
	dma.enable();
}

void AudioOutputPT8211_2::isr(void)
{
	int16_t *dest;
	audio_block_t *blockL, *blockR;
	uint32_t saddr, offsetL, offsetR;

	saddr = (uint32_t)(dma.TCD->SADDR);
	dma.clearInterrupt();
	if (saddr < (uint32_t)i2s_tx_buffer + sizeof(i2s_tx_buffer) / 2) {
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		#if defined(AUDIO_PT8211_OVERSAMPLING)
			dest = (int16_t *)&i2s_tx_buffer[(AUDIO_BLOCK_SAMPLES/2)*4];
		#else
			dest = (int16_t *)&i2s_tx_buffer[AUDIO_BLOCK_SAMPLES/2];
		#endif
		if (AudioOutputPT8211_2::update_responsibility) AudioStream::update_all();
	} else {
		// DMA is transmitting the second half of the buffer
		// so we must fill the first half
		dest = (int16_t *)i2s_tx_buffer;
	}

	blockL = AudioOutputPT8211_2::block_left_1st;
	blockR = AudioOutputPT8211_2::block_right_1st;
	offsetL = AudioOutputPT8211_2::block_left_offset;
	offsetR = AudioOutputPT8211_2::block_right_offset;

	#if defined(AUDIO_PT8211_OVERSAMPLING)
		static int32_t oldL = 0;
		static int32_t oldR = 0;
	#endif
	if (blockL && blockR) {
		#if defined(AUDIO_PT8211_OVERSAMPLING)
			#if defined(AUDIO_PT8211_INTERPOLATION_LINEAR)
				for (int i=0; i< AUDIO_BLOCK_SAMPLES / 2; i++, offsetL++, offsetR++) {
					int32_t valL = blockL->data[offsetL];
					int32_t valR = blockR->data[offsetR];
					int32_t nL = (oldL+valL) >> 1;
					int32_t nR = (oldR+valR) >> 1;
					*(dest+0) = (oldL+nL) >> 1;
					*(dest+1) = (oldR+nR) >> 1;
					*(dest+2) = nL;
					*(dest+3) = nR;
					*(dest+4) = (nL+valL) >> 1;
					*(dest+5) = (nR+valR) >> 1;
					*(dest+6) = valL;
					*(dest+7) = valR;
					dest+=8;
					oldL = valL;
					oldR = valR;
				}
			#elif defined(AUDIO_PT8211_INTERPOLATION_CIC)
				for (int i=0; i< AUDIO_BLOCK_SAMPLES / 2; i++, offsetL++, offsetR++) {
					int32_t valL = blockL->data[offsetL];
					int32_t valR = blockR->data[offsetR];

					int32_t combL[3] = {0};
					static int32_t combLOld[2] = {0};
					int32_t combR[3] = {0};
					static int32_t combROld[2] = {0};

					combL[0] = valL - oldL;
					combR[0] = valR - oldR;
					combL[1] = combL[0] - combLOld[0];
					combR[1] = combR[0] - combROld[0];
					combL[2] = combL[1] - combLOld[1];
					combR[2] = combR[1] - combROld[1];
					// combL[2] now holds input val
					// combR[2] now holds input val
					oldL = valL;
					oldR = valR;
					combLOld[0] = combL[0];
					combROld[0] = combR[0];
					combLOld[1] = combL[1];
					combROld[1] = combR[1];
					for (int j = 0; j < 4; j++) {
						int32_t integrateL[3];
						int32_t integrateR[3];
						static int32_t integrateLOld[3] = {0};
						static int32_t integrateROld[3] = {0};
						integrateL[0] = ( (j==0) ? (combL[2]) : (0) ) + integrateLOld[0];
						integrateR[0] = ( (j==0) ? (combR[2]) : (0) ) + integrateROld[0];
						integrateL[1] = integrateL[0] + integrateLOld[1];
						integrateR[1] = integrateR[0] + integrateROld[1];
						integrateL[2] = integrateL[1] + integrateLOld[2];
						integrateR[2] = integrateR[1] + integrateROld[2];
						// integrateL[2] now holds j'th upsampled value
						// integrateR[2] now holds j'th upsampled value
						*(dest+j*2) = integrateL[2] >> 4;
						*(dest+j*2+1) = integrateR[2] >> 4;
						integrateLOld[0] = integrateL[0];
						integrateROld[0] = integrateR[0];
						integrateLOld[1] = integrateL[1];
						integrateROld[1] = integrateR[1];
						integrateLOld[2] = integrateL[2];
						integrateROld[2] = integrateR[2];
					}
					dest+=8;
				}
			#else
				#error no interpolation method defined for oversampling.
			#endif //defined(AUDIO_PT8211_INTERPOLATION_LINEAR)
		#else
			memcpy_tointerleaveLR(dest, blockL->data + offsetL, blockR->data + offsetR);
			offsetL += AUDIO_BLOCK_SAMPLES / 2;
			offsetR += AUDIO_BLOCK_SAMPLES / 2;
		#endif //defined(AUDIO_PT8211_OVERSAMPLING)

	} else if (blockL) {
		#if defined(AUDIO_PT8211_OVERSAMPLING)
			#if defined(AUDIO_PT8211_INTERPOLATION_LINEAR)
				for (int i=0; i< AUDIO_BLOCK_SAMPLES / 2; i++, offsetL++) {
					int32_t val = blockL->data[offsetL];
					int32_t n = (oldL+val) >> 1;
					*(dest+0) = (oldL+n) >> 1;
					*(dest+1) = 0;
					*(dest+2) = n;
					*(dest+3) = 0;
					*(dest+4) = (n+val) >> 1;
					*(dest+5) = 0;
					*(dest+6) = val;
					*(dest+7) = 0;
					dest+=8;
					oldL = val;
				}
			#elif defined(AUDIO_PT8211_INTERPOLATION_CIC)
				for (int i=0; i< AUDIO_BLOCK_SAMPLES / 2; i++, offsetL++, offsetR++) {
					int32_t valL = blockL->data[offsetL];

					int32_t combL[3] = {0};
					static int32_t combLOld[2] = {0};

					combL[0] = valL - oldL;
					combL[1] = combL[0] - combLOld[0];
					combL[2] = combL[1] - combLOld[1];
					// combL[2] now holds input val
					combLOld[0] = combL[0];
					combLOld[1] = combL[1];

					for (int j = 0; j < 4; j++) {
						int32_t integrateL[3];
						static int32_t integrateLOld[3] = {0};
						integrateL[0] = ( (j==0) ? (combL[2]) : (0) ) + integrateLOld[0];
						integrateL[1] = integrateL[0] + integrateLOld[1];
						integrateL[2] = integrateL[1] + integrateLOld[2];
						// integrateL[2] now holds j'th upsampled value
						*(dest+j*2) = integrateL[2] >> 4;
						integrateLOld[0] = integrateL[0];
						integrateLOld[1] = integrateL[1];
						integrateLOld[2] = integrateL[2];
					}

					// fill right channel with zeros:
					*(dest+1) = 0;
					*(dest+3) = 0;
					*(dest+5) = 0;
					*(dest+7) = 0;
					dest+=8;
					oldL = valL;
				}
			#else
				#error no interpolation method defined for oversampling.
			#endif //defined(AUDIO_PT8211_INTERPOLATION_LINEAR)
		#else
			memcpy_tointerleaveL(dest, blockL->data + offsetL);
			offsetL += (AUDIO_BLOCK_SAMPLES / 2);
		#endif //defined(AUDIO_PT8211_OVERSAMPLING)
	} else if (blockR) {
		#if defined(AUDIO_PT8211_OVERSAMPLING)
			#if defined(AUDIO_PT8211_INTERPOLATION_LINEAR)
				for (int i=0; i< AUDIO_BLOCK_SAMPLES / 2; i++, offsetR++) {
					int32_t val = blockR->data[offsetR];
					int32_t n = (oldR+val) >> 1;
					*(dest+0) = 0;
					*(dest+1) = ((oldR+n) >> 1);
					*(dest+2) = 0;
					*(dest+3) = n;
					*(dest+4) = 0;
					*(dest+5) = ((n+val) >> 1);
					*(dest+6) = 0;
					*(dest+7) = val;
					dest+=8;
					oldR = val;
				}
			#elif defined(AUDIO_PT8211_INTERPOLATION_CIC)
				for (int i=0; i< AUDIO_BLOCK_SAMPLES / 2; i++, offsetL++, offsetR++) {
					int32_t valR = blockR->data[offsetR];

					int32_t combR[3] = {0};
					static int32_t combROld[2] = {0};

					combR[0] = valR - oldR;
					combR[1] = combR[0] - combROld[0];
					combR[2] = combR[1] - combROld[1];
					// combR[2] now holds input val
					combROld[0] = combR[0];
					combROld[1] = combR[1];

					for (int j = 0; j < 4; j++) {
						int32_t integrateR[3];
						static int32_t integrateROld[3] = {0};
						integrateR[0] = ( (j==0) ? (combR[2]) : (0) ) + integrateROld[0];
						integrateR[1] = integrateR[0] + integrateROld[1];
						integrateR[2] = integrateR[1] + integrateROld[2];
						// integrateR[2] now holds j'th upsampled value
						*(dest+j*2+1) = integrateR[2] >> 4;
						integrateROld[0] = integrateR[0];
						integrateROld[1] = integrateR[1];
						integrateROld[2] = integrateR[2];
					}

					// fill left channel with zeros:
					*(dest+0) = 0;
					*(dest+2) = 0;
					*(dest+4) = 0;
					*(dest+6) = 0;
					dest+=8;
					oldR = valR;
				}
			#else
				#error no interpolation method defined for oversampling.
			#endif //defined(AUDIO_PT8211_INTERPOLATION_LINEAR)
		#else
			memcpy_tointerleaveR(dest, blockR->data + offsetR);
			offsetR += AUDIO_BLOCK_SAMPLES / 2;
		#endif //defined(AUDIO_PT8211_OVERSAMPLING)
	} else {
		#if defined(AUDIO_PT8211_OVERSAMPLING)
			memset(dest,0,AUDIO_BLOCK_SAMPLES*8);
		#else
			memset(dest,0,AUDIO_BLOCK_SAMPLES*2);
		#endif
		return;
	}

	if (offsetL < AUDIO_BLOCK_SAMPLES) {
		AudioOutputPT8211_2::block_left_offset = offsetL;
	} else {
		AudioOutputPT8211_2::block_left_offset = 0;
		AudioStream::release(blockL);
		AudioOutputPT8211_2::block_left_1st = AudioOutputPT8211_2::block_left_2nd;
		AudioOutputPT8211_2::block_left_2nd = NULL;
	}
	if (offsetR < AUDIO_BLOCK_SAMPLES) {
		AudioOutputPT8211_2::block_right_offset = offsetR;
	} else {
		AudioOutputPT8211_2::block_right_offset = 0;
		AudioStream::release(blockR);
		AudioOutputPT8211_2::block_right_1st = AudioOutputPT8211_2::block_right_2nd;
		AudioOutputPT8211_2::block_right_2nd = NULL;
	}
}



void AudioOutputPT8211_2::update(void)
{

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

PROGMEM
void AudioOutputPT8211_2::config_i2s(void)
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

	if (I2S2_TCSR & I2S_TCSR_TE) return;

	//CORE_PIN5_CONFIG  = 2;  //2:MCLK
	CORE_PIN4_CONFIG  = 2;  //2:TX_BCLK
	CORE_PIN3_CONFIG  = 2;  //2:TX_SYNC

	#if defined(AUDIO_PT8211_OVERSAMPLING)
	int div = 0;
	#else
	int div = 3;
	#endif
	// configure transmitter
	I2S2_TMR = 0;
	I2S2_TCR1 = I2S_TCR1_RFW(0);
	I2S2_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_BCP | I2S_TCR2_MSEL(1) | I2S_TCR2_BCD | I2S_TCR2_DIV(div);
	I2S2_TCR3 = I2S_TCR3_TCE;
//	I2S2_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(15) | I2S_TCR4_MF | I2S_TCR4_FSE | I2S_TCR4_FSP | I2S_TCR4_FSD; //TDA1543
	I2S2_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(15) | I2S_TCR4_MF /*| I2S_TCR4_FSE*/ | I2S_TCR4_FSP | I2S_TCR4_FSD; //PT8211
	I2S2_TCR5 = I2S_TCR5_WNW(15) | I2S_TCR5_W0W(15) | I2S_TCR5_FBT(15);

}
#endif
