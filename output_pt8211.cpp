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


#include <Arduino.h>
#include "output_pt8211.h"
#include "memcpy_audio.h"
#include "utility/imxrt_hw.h"

audio_block_t * AudioOutputPT8211::block_left_1st = NULL;
audio_block_t * AudioOutputPT8211::block_right_1st = NULL;
audio_block_t * AudioOutputPT8211::block_left_2nd = NULL;
audio_block_t * AudioOutputPT8211::block_right_2nd = NULL;
uint16_t  AudioOutputPT8211::block_left_offset = 0;
uint16_t  AudioOutputPT8211::block_right_offset = 0;
bool AudioOutputPT8211::update_responsibility = false;
#if defined(AUDIO_PT8211_OVERSAMPLING)
DMAMEM __attribute__((aligned(32))) static uint32_t i2s_tx_buffer[AUDIO_BLOCK_SAMPLES*4];
#else
DMAMEM __attribute__((aligned(32))) static uint32_t i2s_tx_buffer[AUDIO_BLOCK_SAMPLES];
#endif
DMAChannel AudioOutputPT8211::dma(false);

void AudioOutputPT8211::begin(void)
{
	dma.begin(true); // Allocate the DMA channel first

	block_left_1st = NULL;
	block_right_1st = NULL;

	// TODO: should we set & clear the I2S_TCSR_SR bit here?
	config_i2s();
#if defined(KINETISK)
	CORE_PIN22_CONFIG = PORT_PCR_MUX(6); // pin 22, PTC1, I2S0_TXD0

	dma.TCD->SADDR = i2s_tx_buffer;
	dma.TCD->SOFF = 2;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	dma.TCD->NBYTES_MLNO = 2;
	dma.TCD->SLAST = -sizeof(i2s_tx_buffer);
	dma.TCD->DADDR = &I2S0_TDR0;
	dma.TCD->DOFF = 0;
	dma.TCD->CITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;
	dma.TCD->DLASTSGA = 0;
	dma.TCD->BITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;

	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_TX);
	update_responsibility = update_setup();
	dma.attachInterrupt(isr);
	dma.enable();
	I2S0_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE | I2S_TCSR_FR;
	return;
#elif defined(__IMXRT1052__) || defined(__IMXRT1062__)

#if defined(__IMXRT1052__)
	CORE_PIN6_CONFIG  = 3;  //1:TX_DATA0
#elif defined(__IMXRT1062__)
	CORE_PIN7_CONFIG  = 3;  //1:TX_DATA0	
#endif

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
	dma.TCD->DADDR = (void *)((uint32_t)&I2S1_TDR0);
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI1_TX);

	I2S1_RCSR |= I2S_RCSR_RE;
	I2S1_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;

	update_responsibility = update_setup();
	dma.attachInterrupt(isr);
	dma.enable();
	return;
#endif	
}

void AudioOutputPT8211::isr(void)
{
	int16_t *dest, *dest_copy;
	audio_block_t *blockL, *blockR;
	uint32_t saddr, offsetL, offsetR;

#if defined(KINETISK) || defined(__IMXRT1062__)
	saddr = (uint32_t)(dma.TCD->SADDR);
#endif
	dma.clearInterrupt();
	if (saddr < (uint32_t)i2s_tx_buffer + sizeof(i2s_tx_buffer) / 2) {
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		#if defined(AUDIO_PT8211_OVERSAMPLING)
			dest = (int16_t *)&i2s_tx_buffer[(AUDIO_BLOCK_SAMPLES/2)*4];
		#else
			dest = (int16_t *)&i2s_tx_buffer[AUDIO_BLOCK_SAMPLES/2];
		#endif
		if (AudioOutputPT8211::update_responsibility) AudioStream::update_all();
	} else {
		// DMA is transmitting the second half of the buffer
		// so we must fill the first half
		dest = (int16_t *)i2s_tx_buffer;
	}
	dest_copy = dest;

	blockL = AudioOutputPT8211::block_left_1st;
	blockR = AudioOutputPT8211::block_right_1st;
	offsetL = AudioOutputPT8211::block_left_offset;
	offsetR = AudioOutputPT8211::block_right_offset;

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

	arm_dcache_flush_delete(dest_copy, sizeof(i2s_tx_buffer) / 2);

	if (offsetL < AUDIO_BLOCK_SAMPLES) {
		AudioOutputPT8211::block_left_offset = offsetL;
	} else {
		AudioOutputPT8211::block_left_offset = 0;
		AudioStream::release(blockL);
		AudioOutputPT8211::block_left_1st = AudioOutputPT8211::block_left_2nd;
		AudioOutputPT8211::block_left_2nd = NULL;
	}
	if (offsetR < AUDIO_BLOCK_SAMPLES) {
		AudioOutputPT8211::block_right_offset = offsetR;
	} else {
		AudioOutputPT8211::block_right_offset = 0;
		AudioStream::release(blockR);
		AudioOutputPT8211::block_right_1st = AudioOutputPT8211::block_right_2nd;
		AudioOutputPT8211::block_right_2nd = NULL;
	}
}



void AudioOutputPT8211::update(void)
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

#if defined(KINETISK)
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
  #define MCLK_MULT 12
  #define MCLK_DIV  17
  #define MCLK_SRC  1
#elif F_CPU == 240000000
  #define MCLK_MULT 2
  #define MCLK_DIV  85
  #define MCLK_SRC  0
#elif F_CPU == 256000000
  #define MCLK_MULT 12
  #define MCLK_DIV  17
  #define MCLK_SRC  1
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
#endif

void AudioOutputPT8211::config_i2s(void)
{
#if defined(KINETISK)
	SIM_SCGC6 |= SIM_SCGC6_I2S;
	SIM_SCGC7 |= SIM_SCGC7_DMA;
	SIM_SCGC6 |= SIM_SCGC6_DMAMUX;

	// if transmitter is enabled, do nothing
	if (I2S0_TCSR & I2S_TCSR_TE) return;


	// enable MCLK output
	I2S0_MCR = I2S_MCR_MICS(MCLK_SRC) | I2S_MCR_MOE;
	while (I2S0_MCR & I2S_MCR_DUF) ;
	I2S0_MDR = I2S_MDR_FRACT((MCLK_MULT-1)) | I2S_MDR_DIVIDE((MCLK_DIV-1));

	// configure transmitter
	I2S0_TMR = 0;
	I2S0_TCR1 = I2S_TCR1_TFW(1);  // watermark at half fifo size
	#if defined(AUDIO_PT8211_OVERSAMPLING)
		I2S0_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_BCP | I2S_TCR2_MSEL(1) | I2S_TCR2_BCD | I2S_TCR2_DIV(0);
	#else
		I2S0_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_BCP | I2S_TCR2_MSEL(1) | I2S_TCR2_BCD | I2S_TCR2_DIV(3);
	#endif
	I2S0_TCR3 = I2S_TCR3_TCE;
//	I2S0_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(15) | I2S_TCR4_MF | I2S_TCR4_FSE | I2S_TCR4_FSP | I2S_TCR4_FSD; //TDA1543
	I2S0_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(15) | I2S_TCR4_MF /*| I2S_TCR4_FSE*/ | I2S_TCR4_FSP | I2S_TCR4_FSD; //PT8211
	I2S0_TCR5 = I2S_TCR5_WNW(15) | I2S_TCR5_W0W(15) | I2S_TCR5_FBT(15);

	// configure pin mux for 3 clock signals
	CORE_PIN23_CONFIG = PORT_PCR_MUX(6); // pin 23, PTC2, I2S0_TX_FS (LRCLK)
	CORE_PIN9_CONFIG  = PORT_PCR_MUX(6); // pin  9, PTC3, I2S0_TX_BCLK
	//CORE_PIN11_CONFIG = PORT_PCR_MUX(6); // pin 11, PTC6, I2S0_MCLK
	
#elif ( defined(__IMXRT1052__) || defined(__IMXRT1062__) )

	CCM_CCGR5 |= CCM_CCGR5_SAI1(CCM_CCGR_ON);
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

	// clear SAI1_CLK register locations
	CCM_CSCMR1 = (CCM_CSCMR1 & ~(CCM_CSCMR1_SAI1_CLK_SEL_MASK))
		   | CCM_CSCMR1_SAI1_CLK_SEL(2); // &0x03 // (0,1,2): PLL3PFD0, PLL5, PLL4
	CCM_CS1CDR = (CCM_CS1CDR & ~(CCM_CS1CDR_SAI1_CLK_PRED_MASK | CCM_CS1CDR_SAI1_CLK_PODF_MASK))
		   | CCM_CS1CDR_SAI1_CLK_PRED(n1-1) // &0x07
		   | CCM_CS1CDR_SAI1_CLK_PODF(n2-1); // &0x3f

	IOMUXC_GPR_GPR1 = (IOMUXC_GPR_GPR1 & ~(IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL_MASK))
			| (IOMUXC_GPR_GPR1_SAI1_MCLK_DIR | IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL(0));	//Select MCLK

	if (I2S1_TCSR & I2S_TCSR_TE) return;

//	CORE_PIN23_CONFIG = 3;  //1:MCLK
	CORE_PIN21_CONFIG = 3;  //1:RX_BCLK
	CORE_PIN20_CONFIG = 3;  //1:RX_SYNC
//	CORE_PIN6_CONFIG  = 3;  //1:TX_DATA0
//	CORE_PIN7_CONFIG  = 3;  //1:RX_DATA0

	int rsync = 0;
	int tsync = 1;
	#if defined(AUDIO_PT8211_OVERSAMPLING)
	int div = 0;
	#else
	int div = 3;
	#endif
	// configure transmitter
	I2S1_TMR = 0;
	I2S1_TCR1 = I2S_TCR1_RFW(0);
	I2S1_TCR2 = I2S_TCR2_SYNC(tsync) | I2S_TCR2_BCP | I2S_TCR2_MSEL(1) | I2S_TCR2_BCD | I2S_TCR2_DIV(div);
	I2S1_TCR3 = I2S_TCR3_TCE;
//	I2S1_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(15) | I2S_TCR4_MF | I2S_TCR4_FSE | I2S_TCR4_FSP | I2S_TCR4_FSD; //TDA1543
	I2S1_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(15) | I2S_TCR4_MF /*| I2S_TCR4_FSE*/ | I2S_TCR4_FSP | I2S_TCR4_FSD; //PT8211
	I2S1_TCR5 = I2S_TCR5_WNW(15) | I2S_TCR5_W0W(15) | I2S_TCR5_FBT(15);

	I2S1_RMR = 0;
	//I2S1_RCSR = (1<<25); //Reset
	I2S1_RCR1 = I2S_RCR1_RFW(0);
	I2S1_RCR2 = I2S_RCR2_SYNC(rsync) | I2S_RCR2_BCP | I2S_RCR2_MSEL(1) | I2S_TCR2_BCD | I2S_TCR2_DIV(div);
	I2S1_RCR3 = I2S_RCR3_RCE;
//	I2S1_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(15) | I2S_TCR4_MF | I2S_TCR4_FSE | I2S_TCR4_FSP | I2S_TCR4_FSD; //TDA1543	
	I2S1_RCR4 = I2S_RCR4_FRSZ(1) | I2S_RCR4_SYWD(15) | I2S_RCR4_MF /*| I2S_RCR4_FSE*/ | I2S_RCR4_FSP | I2S_RCR4_FSD; //PT8211
	I2S1_RCR5 = I2S_RCR5_WNW(15) | I2S_RCR5_W0W(15) | I2S_RCR5_FBT(15);

#endif
}
