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

 // 2015/08/23: (FB) added mute_PCM() - sets or unsets VALID in VUCP (and adjusts PARITY)

#include <Arduino.h>
#include "output_spdif.h"
#include "utility/imxrt_hw.h"

audio_block_t * AudioOutputSPDIF::block_left_1st = NULL;
audio_block_t * AudioOutputSPDIF::block_right_1st = NULL;
audio_block_t * AudioOutputSPDIF::block_left_2nd = NULL;
audio_block_t * AudioOutputSPDIF::block_right_2nd = NULL;
uint16_t  AudioOutputSPDIF::block_left_offset = 0;
uint16_t  AudioOutputSPDIF::block_right_offset = 0;
bool AudioOutputSPDIF::update_responsibility = false;
DMAChannel AudioOutputSPDIF::dma(false);
extern uint16_t spdif_bmclookup[256];
DMAMEM __attribute__((aligned(32)))
static uint32_t SPDIF_tx_buffer[AUDIO_BLOCK_SAMPLES * 4]; //2 KB


#if defined(KINETISK) || defined(__IMXRT1052__) || defined(__IMXRT1062__)

#define PREAMBLE_B  (0xE8) //11101000
#define PREAMBLE_M  (0xE2) //11100010
#define PREAMBLE_W  (0xE4) //11100100

#define VUCP_VALID   ((0xCC) << 24)
#define VUCP_INVALID ((0xD4) << 24)// To mute PCM, set VUCP = invalid.

uint32_t  AudioOutputSPDIF::vucp = VUCP_VALID;

#endif

PROGMEM
void AudioOutputSPDIF::begin(void)
{

	dma.begin(true); // Allocate the DMA channel first

	block_left_1st = NULL;
	block_right_1st = NULL;

	// TODO: should we set & clear the I2S_TCSR_SR bit here?
	config_SPDIF();
#if defined(KINETISK)
	CORE_PIN22_CONFIG = PORT_PCR_MUX(6); // pin 22, PTC1, I2S0_TXD0

	const int nbytes_mlno = 2 * 4; // 8 Bytes per minor loop

	dma.TCD->SADDR = SPDIF_tx_buffer;
	dma.TCD->SOFF = 4;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
	dma.TCD->NBYTES_MLNO = nbytes_mlno;
	dma.TCD->SLAST = -sizeof(SPDIF_tx_buffer);
	dma.TCD->DADDR = &I2S0_TDR0;
	dma.TCD->DOFF = 0;
	dma.TCD->CITER_ELINKNO = sizeof(SPDIF_tx_buffer) / nbytes_mlno;
	dma.TCD->DLASTSGA = 0;
	dma.TCD->BITER_ELINKNO = sizeof(SPDIF_tx_buffer) / nbytes_mlno;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_TX);
	update_responsibility = update_setup();
	dma.enable();

	I2S0_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE | I2S_TCSR_FR;
	dma.attachInterrupt(isr);
#elif defined(__IMXRT1052__) || defined(__IMXRT1062__)

#if defined(__IMXRT1052__)
	CORE_PIN6_CONFIG  = 3;  //1:TX_DATA0
#elif defined(__IMXRT1062__)
	CORE_PIN7_CONFIG  = 3;  //1:TX_DATA0	
#endif		
	
	const int nbytes_mlno = 2 * 4; // 8 Bytes per minor loop

	dma.TCD->SADDR = SPDIF_tx_buffer;
	dma.TCD->SOFF = 4;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
	dma.TCD->NBYTES_MLNO = nbytes_mlno;
	dma.TCD->SLAST = -sizeof(SPDIF_tx_buffer);
	dma.TCD->DADDR = &I2S1_TDR0;
	dma.TCD->DOFF = 0;
	dma.TCD->CITER_ELINKNO = sizeof(SPDIF_tx_buffer) / nbytes_mlno;
	dma.TCD->DLASTSGA = 0;
	dma.TCD->BITER_ELINKNO = sizeof(SPDIF_tx_buffer) / nbytes_mlno;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI1_TX);
	update_responsibility = update_setup();
	dma.enable();

	I2S1_RCSR |= I2S_RCSR_RE;
	I2S1_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE | I2S_TCSR_FR;
	dma.attachInterrupt(isr);
#endif
}

/*

 http://www.hardwarebook.info/S/PDIF

 1. To make it easier and a bit faster, the parity-bit is always the same.
	- With a alternating parity we had to adjust the next subframe. Instead, use a bit from the aux-info as parity.

 2. The buffer is filled with an offset of 1 byte, so the last parity (which is always 0 now (see 1.) ) is written as first byte.
	-> A bit easier and faster to construct both subframes.

*/

void AudioOutputSPDIF::isr(void)
{
	static uint16_t frame = 0;
	const int16_t *src;
	int32_t *end, *dest;
	audio_block_t *block;
	uint32_t saddr, offset;
	uint16_t sample, lo, hi, aux;

	saddr = (uint32_t)(dma.TCD->SADDR);
	dma.clearInterrupt();
	if (saddr < (uint32_t)SPDIF_tx_buffer + sizeof(SPDIF_tx_buffer) / 2) {
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		dest = (int32_t *)&SPDIF_tx_buffer[AUDIO_BLOCK_SAMPLES * 4/2];
		end = (int32_t *)&SPDIF_tx_buffer[AUDIO_BLOCK_SAMPLES * 4];
		if (AudioOutputSPDIF::update_responsibility) AudioStream::update_all();
	} else {
		// DMA is transmitting the second half of the buffer
		// so we must fill the first half
		dest = (int32_t *)SPDIF_tx_buffer;
		end = (int32_t *)&SPDIF_tx_buffer[AUDIO_BLOCK_SAMPLES * 4/2];
	}


	block = AudioOutputSPDIF::block_left_1st;
	if (block) {
		offset = AudioOutputSPDIF::block_left_offset;
		src = &block->data[offset];
		do {

			sample = *src++;

			//Subframe Channel 1
			hi  = spdif_bmclookup[(uint8_t)(sample >> 8)];
			lo  = spdif_bmclookup[(uint8_t) sample];
			lo ^= (~((int16_t)hi) >> 16);
			// 16 Bit sample:
			*(dest+1) = ((uint32_t)lo << 16) | hi;
			// 4 Bit Auxillary-audio-databits, the first used as parity
			aux = (0xB333 ^ (((uint32_t)((int16_t)lo)) >> 17));

			if (++frame > 191) {
				// VUCP-Bits ("Valid, Subcode, Channelstatus, Parity) = 0 (0xcc) | Preamble (depends on Framno.) | Auxillary
				*(dest+0) =  vucp | (PREAMBLE_B << 16 ) | aux; //special preamble for one of 192 frames
				frame = 0;
			} else {
				*(dest+0) = vucp | (PREAMBLE_M << 16 ) | aux;
			}
			dest += 4;

		} while (dest < end);
		offset += AUDIO_BLOCK_SAMPLES/2;
		if (offset < AUDIO_BLOCK_SAMPLES) {
			AudioOutputSPDIF::block_left_offset = offset;
		} else {
			AudioOutputSPDIF::block_left_offset = 0;
			AudioStream::release(block);
			AudioOutputSPDIF::block_left_1st = AudioOutputSPDIF::block_left_2nd;
			AudioOutputSPDIF::block_left_2nd = NULL;
		}
	} else {
		do {
			if ( ++frame > 191 ) {
				*(dest+0) = vucp | 0x00e8cccc;
				frame = 0;
			} else {
				*(dest+0) = vucp | 0x00e2cccc;
			}
			*(dest+1) = 0xccccccccUL;

			dest +=4;
		} while (dest < end);
	}


	dest -= AUDIO_BLOCK_SAMPLES * 4/2 - 4/2;
	block = AudioOutputSPDIF::block_right_1st;
	if (block) {
		offset = AudioOutputSPDIF::block_right_offset;
		src = &block->data[offset];

		do {
			sample = *src++;

			//Subframe Channel 2
			hi  = spdif_bmclookup[(uint8_t)(sample >> 8)];
			lo  = spdif_bmclookup[(uint8_t)sample];
			lo ^= (~((int16_t)hi) >> 16);

			*(dest+1) = ( ((uint32_t)lo << 16) | hi );

			aux = (0xB333 ^ (((uint32_t)((int16_t)lo)) >> 17));
			*(dest+0)  =  vucp | (PREAMBLE_W << 16 ) | aux;

			dest += 4;
		} while (dest < end);

		offset += AUDIO_BLOCK_SAMPLES/2;
		if (offset < AUDIO_BLOCK_SAMPLES) {
			AudioOutputSPDIF::block_right_offset = offset;
		} else {
			AudioOutputSPDIF::block_right_offset = 0;
			AudioStream::release(block);
			AudioOutputSPDIF::block_right_1st = AudioOutputSPDIF::block_right_2nd;
			AudioOutputSPDIF::block_right_2nd = NULL;
		}
	} else {
		do {
			*dest 	= vucp | 0x00e4ccccUL;
			*(dest+1) = 0xccccccccUL;
			dest += 4 ;
		} while (dest < end);
	}

	#if IMXRT_CACHE_ENABLED >= 2
	dest -= AUDIO_BLOCK_SAMPLES * 4/2 + 4/2;
	arm_dcache_flush_delete(dest, sizeof(SPDIF_tx_buffer) / 2 );
	#endif	
	
}

void AudioOutputSPDIF::mute_PCM(const bool mute)
{
	vucp = mute?VUCP_INVALID:VUCP_VALID;
}

void AudioOutputSPDIF::update(void)
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

PROGMEM
void AudioOutputSPDIF::config_SPDIF(void)
{
#if defined(KINETISK)
	SIM_SCGC6 |= SIM_SCGC6_I2S;
	SIM_SCGC7 |= SIM_SCGC7_DMA;
	SIM_SCGC6 |= SIM_SCGC6_DMAMUX;

	// enable MCLK output
	I2S0_MCR = I2S_MCR_MICS(MCLK_SRC) | I2S_MCR_MOE;
	while (I2S0_MCR & I2S_MCR_DUF) ;
	I2S0_MDR = I2S_MDR_FRACT((MCLK_MULT-1)) | I2S_MDR_DIVIDE((MCLK_DIV-1));

	// configure transmitter
	I2S0_TMR = 0;
	I2S0_TCR1 = I2S_TCR1_TFW(1);  // watermark
	I2S0_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_MSEL(1) | I2S_TCR2_BCD | I2S_TCR2_DIV(0);
	I2S0_TCR3 = I2S_TCR3_TCE;

	//4 Words per Frame 32 Bit Word-Length -> 128 Bit Frame-Length, MSB First:
	I2S0_TCR4 = I2S_TCR4_FRSZ(3) | I2S_TCR4_SYWD(0) | I2S_TCR4_MF | I2S_TCR4_FSP | I2S_TCR4_FSD;
	I2S0_TCR5 = I2S_TCR5_WNW(31) | I2S_TCR5_W0W(31) | I2S_TCR5_FBT(31);

	I2S0_RCSR = 0;

#if 0
	// configure pin mux for 3 clock signals (debug only)
	CORE_PIN23_CONFIG = PORT_PCR_MUX(6); // pin 23, PTC2, I2S0_TX_FS (LRCLK)	44.1kHz
	CORE_PIN9_CONFIG  = PORT_PCR_MUX(6); // pin  9, PTC3, I2S0_TX_BCLK		5.6 MHz
	CORE_PIN11_CONFIG = PORT_PCR_MUX(6); // pin 11, PTC6, I2S0_MCLK			11.43MHz
#endif

#elif defined(__IMXRT1052__) || defined(__IMXRT1062__)

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

	CCM_CSCMR1 = (CCM_CSCMR1 & ~(CCM_CSCMR1_SAI1_CLK_SEL_MASK))
		   | CCM_CSCMR1_SAI1_CLK_SEL(2); // &0x03 // (0,1,2): PLL3PFD0, PLL5, PLL4
	CCM_CS1CDR = (CCM_CS1CDR & ~(CCM_CS1CDR_SAI1_CLK_PRED_MASK | CCM_CS1CDR_SAI1_CLK_PODF_MASK))
		   | CCM_CS1CDR_SAI1_CLK_PRED(n1-1) // &0x07
		   | CCM_CS1CDR_SAI1_CLK_PODF(n2-1); // &0x3f

	IOMUXC_GPR_GPR1 = (IOMUXC_GPR_GPR1 & ~(IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL_MASK))
			| (IOMUXC_GPR_GPR1_SAI1_MCLK_DIR | IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL(0));	//Select MCLK

	int rsync = 0;
	int tsync = 1;
	// configure transmitter
	I2S1_TMR = 0;
	I2S1_TCR1 = I2S_TCR1_RFW(0);  // watermark
	I2S1_TCR2 = I2S_TCR2_SYNC(tsync) | I2S_TCR2_MSEL(1) | I2S_TCR2_BCD | I2S_TCR2_DIV(0);
	I2S1_TCR3 = I2S_TCR3_TCE;

	//4 Words per Frame 32 Bit Word-Length -> 128 Bit Frame-Length, MSB First:
	I2S1_TCR4 = I2S_TCR4_FRSZ(3) | I2S_TCR4_SYWD(0) | I2S_TCR4_MF | I2S_TCR4_FSP | I2S_TCR4_FSD;
	I2S1_TCR5 = I2S_TCR5_WNW(31) | I2S_TCR5_W0W(31) | I2S_TCR5_FBT(31);

	//I2S1_RCSR = 0;
	I2S1_RMR = 0;
	//I2S1_RCSR = (1<<25); //Reset
	I2S1_RCR1 = I2S_RCR1_RFW(0);
	I2S1_RCR2 = I2S_RCR2_SYNC(rsync) | I2S_TCR2_MSEL(1) | I2S_TCR2_BCD | I2S_TCR2_DIV(0);
	I2S1_RCR3 = I2S_RCR3_RCE;
	I2S1_RCR4 = I2S_TCR4_FRSZ(3) | I2S_TCR4_SYWD(0) | I2S_TCR4_MF | I2S_TCR4_FSP | I2S_TCR4_FSD;
	I2S1_RCR5 = I2S_TCR5_WNW(31) | I2S_TCR5_W0W(31) | I2S_TCR5_FBT(31);

#if 0
	//debug only:
	CORE_PIN23_CONFIG = 3;  //1:MCLK	11.43MHz
	CORE_PIN21_CONFIG = 3;  //1:RX_BCLK	5.6 MHz
	CORE_PIN20_CONFIG = 3;  //1:RX_SYNC	44.1 KHz
//	CORE_PIN6_CONFIG  = 3;  //1:TX_DATA0
//	CORE_PIN7_CONFIG  = 3;  //1:RX_DATA0
#endif

#endif
}


#if defined(KINETISL)

void AudioOutputSPDIF::update(void)
{

	audio_block_t *block;
	block = receiveReadOnly(0); // input 0 = left channel
	if (block) release(block);
	block = receiveReadOnly(1); // input 1 = right channel
	if (block) release(block);
}

#endif
