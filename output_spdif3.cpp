/* Hardware-SPDIF for Teensy 4
 * Copyright (c) 2019, Frank Bösing, f.boesing@gmx.de
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
/*
 http://www.hardwarebook.info/S/PDIF
 https://www.mikrocontroller.net/articles/S/PDIF
 https://en.wikipedia.org/wiki/S/PDIF
*/

#if defined(__IMXRT1052__) || defined(__IMXRT1062__)

#include <Arduino.h>
#include "output_spdif3.h"
#include "utility/imxrt_hw.h"
#include "memcpy_audio.h"
#include <math.h>

audio_block_t * AudioOutputSPDIF3::block_left_1st = nullptr;
audio_block_t * AudioOutputSPDIF3::block_right_1st = nullptr;
audio_block_t * AudioOutputSPDIF3::block_left_2nd = nullptr;
audio_block_t * AudioOutputSPDIF3::block_right_2nd = nullptr;
bool AudioOutputSPDIF3::update_responsibility = false;
DMAChannel AudioOutputSPDIF3::dma(false);

DMAMEM  __attribute__((aligned(32)))
static int32_t SPDIF_tx_buffer[AUDIO_BLOCK_SAMPLES * 4];
DMAMEM  __attribute__((aligned(32)))
audio_block_t AudioOutputSPDIF3::block_silent;

PROGMEM
void AudioOutputSPDIF3::begin(void)
{

	dma.begin(true); // Allocate the DMA channel first

	block_left_1st = nullptr;
	block_right_1st = nullptr;
	memset(&block_silent, 0, sizeof(block_silent));
	uint32_t fs = AUDIO_SAMPLE_RATE_EXACT;
	// PLL between 27*24 = 648MHz und 54*24=1296MHz
	// n1, n2 choosen for compatibility with I2S (same PLL frequency) :
	int n1 = 4; //SAI prescaler 4 => (n1*n2) = multiple of 4
	int n2 = 1 + (24000000 * 27) / (fs * 256 * n1);
	double C = ((double)fs * 256 * n1 * n2) / 24000000;
	int c0 = C;
	int c2 = 10000;
	int c1 = C * c2 - (c0 * c2);
	set_audioClock(c0, c1, c2);

	double pllclock = (c0 + (double)c1 / c2) * 24000000; //677376000 Hz

	//use new pred/podf values
	n1 = 7; //0: divide by 1 (do not use with high input frequencies), 1:/2, 2: /3, 7:/8
	n2 = 0; //0: divide by 1, 7: divide by 8

	uint32_t clock = pllclock / (1 + n1) / (1 + n2);
	uint32_t clkdiv = clock / (fs * 64); // 1 .. 128
	uint32_t mod = clock % (fs * 64);
	if (mod > ((fs * 64) / 2)) clkdiv += 1; //nearest divider

	CCM_CCGR5 &= ~CCM_CCGR5_SPDIF(CCM_CCGR_ON); //Clock gate off

	CCM_CDCDR = (CCM_CDCDR & ~(CCM_CDCDR_SPDIF0_CLK_SEL_MASK | CCM_CDCDR_SPDIF0_CLK_PRED_MASK | CCM_CDCDR_SPDIF0_CLK_PODF_MASK))
		| CCM_CDCDR_SPDIF0_CLK_SEL(0) // 0 PLL4, 1 PLL3 PFD2, 2 PLL5, 3 pll3_sw_clk
		| CCM_CDCDR_SPDIF0_CLK_PRED(n1)
		| CCM_CDCDR_SPDIF0_CLK_PODF(n2);

	CCM_CCGR5 |= CCM_CCGR5_SPDIF(CCM_CCGR_ON); //Clock gate on

	if ((SPDIF_SCR & 0x400) != 0) {			//If default value:
		SPDIF_SCR = SPDIF_SCR_SOFT_RESET;		//Reset SPDIF
		while (SPDIF_SCR & SPDIF_SCR_SOFT_RESET) {;}	//Wait for Reset (takes 8 cycles)
		SPDIF_SCR = 0;
	}

	SPDIF_SCR |=
		SPDIF_SCR_RXFIFOFULL_SEL(0) |// Full interrupt if at least 1 sample in Rx left and right FIFOs
		SPDIF_SCR_RXAUTOSYNC |
		SPDIF_SCR_TXAUTOSYNC |
		SPDIF_SCR_TXFIFOEMPTY_SEL(3) | // Empty interrupt if at most 4 sample in Tx left and right FIFOs
		SPDIF_SCR_TXFIFO_CTRL(1) | // 0:Send zeros 1: normal operation
		SPDIF_SCR_DMA_TX_EN |
		//SPDIF_SCR_VALCTRL | // Outgoing Validity always clear
		SPDIF_SCR_TXSEL(5) | //0:off and output 0, 1:Feed-though SPDIFIN, 5:Tx Normal operation
		SPDIF_SCR_USRC_SEL(3); // No embedded U channel
		SPDIF_SRCD = 0;
		SPDIF_STCSCH = 0;
		SPDIF_STCSCL = 0;
	
	SPDIF_SRCD = 0;

	SPDIF_STC = SPDIF_STC_TXCLK_SOURCE(1) //tx_clk input (from SPDIF0_CLK_ROOT)
		| SPDIF_STC_SYSCLK_DF(0)
		| SPDIF_STC_TXCLK_DF(clkdiv - 1);

	const int nbytes_mlno = 2 * 4; // 8 Bytes per minor loop

	dma.TCD->SADDR = SPDIF_tx_buffer;
	dma.TCD->SOFF = 4;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);	
	dma.TCD->NBYTES_MLNO = DMA_TCD_NBYTES_MLOFFYES_NBYTES(nbytes_mlno) | DMA_TCD_NBYTES_DMLOE | 
                         DMA_TCD_NBYTES_MLOFFYES_MLOFF(-8);
	dma.TCD->SLAST = -sizeof(SPDIF_tx_buffer);
	dma.TCD->DADDR = &SPDIF_STL;
	dma.TCD->DOFF = 4;
	dma.TCD->DLASTSGA = -8;
	//dma.TCD->ATTR_DST = ((31 - __builtin_clz(8)) << 3);
	dma.TCD->CITER_ELINKNO = sizeof(SPDIF_tx_buffer) / nbytes_mlno;	
	dma.TCD->BITER_ELINKNO = sizeof(SPDIF_tx_buffer) / nbytes_mlno;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;

	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SPDIF_TX);

	update_responsibility = update_setup();	
	dma.enable();
	dma.attachInterrupt(isr);	

	CORE_PIN14_CONFIG = 3;  //3:SPDIF_OUT
	SPDIF_STC |= SPDIF_STC_TX_ALL_CLK_EN;
//	pinMode(13, OUTPUT);
}

void AudioOutputSPDIF3::isr(void)
{

	const int16_t *src_left, *src_right;
	const int32_t *end;
	int32_t *dest;
	audio_block_t *block_left, *block_right;
	uint32_t saddr;

	saddr = (uint32_t)(dma.TCD->SADDR);
	dma.clearInterrupt();
	if (saddr < (uint32_t)SPDIF_tx_buffer + sizeof(SPDIF_tx_buffer) / 2) {
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		dest = SPDIF_tx_buffer + AUDIO_BLOCK_SAMPLES*2;
		end = SPDIF_tx_buffer + AUDIO_BLOCK_SAMPLES*4;
	} else {
		// DMA is transmitting the second half of the buffer
		// so we must fill the first half
		dest = SPDIF_tx_buffer;
		end = SPDIF_tx_buffer + AUDIO_BLOCK_SAMPLES*2;
	}
	block_left = block_left_1st;
	if (!block_left) block_left = &block_silent;
	block_right = block_right_1st;
	if (!block_right) block_right = &block_silent;

	src_left = (const int16_t *)(block_left->data);
	src_right = (const int16_t *)(block_right->data);
		
	do {
		
		*dest++ = (*src_left++) << 8;
		*dest++ = (*src_right++) << 8;

		*dest++ = (*src_left++) << 8;
		*dest++ = (*src_right++) << 8;
		
		*dest++ = (*src_left++) << 8;
		*dest++ = (*src_right++) << 8;

		*dest++ = (*src_left++) << 8;
		*dest++ = (*src_right++) << 8;
		
		#if IMXRT_CACHE_ENABLED >= 2
		SCB_CACHE_DCCIMVAC = (uint32_t) dest - 32;
		#endif
		
	} while (dest < end);
	
	if (block_left != &block_silent) {
		release(block_left);	
		block_left_1st = block_left_2nd;
		block_left_2nd = nullptr;
	}
	if (block_right != &block_silent) {
		release(block_right);	
		block_right_1st = block_right_2nd;
		block_right_2nd = nullptr;
	}

	if (update_responsibility) update_all();
	//digitalWriteFast(13,!digitalReadFast(13));	
}

void AudioOutputSPDIF3::update(void)
{
	audio_block_t *block_left, *block_right;

	block_left = receiveReadOnly(0);  // input 0
	block_right = receiveReadOnly(1); // input 1
	__disable_irq();
	if (block_left) {
		if (block_left_1st == nullptr) {
			block_left_1st = block_left;
			block_left = nullptr;
		} else if (block_left_2nd == nullptr) {
			block_left_2nd = block_left;
			block_left = nullptr;
		} else {
			audio_block_t *tmp = block_left_1st;
			block_left_1st = block_left_2nd;
			block_left_2nd = block_left;
			block_left = tmp;
		}
	}
	if (block_right) {
		if (block_right_1st == nullptr) {
			block_right_1st = block_right;
			block_right = nullptr;
		} else if (block_right_2nd == nullptr) {
			block_right_2nd = block_right;
			block_right = nullptr;
		} else {
			audio_block_t *tmp = block_right_1st;
			block_right_1st = block_right_2nd;
			block_right_2nd = block_right;
			block_right = tmp;
		}
	}
	__enable_irq();
	if (block_left) {
		release(block_left);	
	}
	if (block_right) {			
		release(block_right);		
	}

}

void AudioOutputSPDIF3::mute_PCM(const bool mute)
{
	if (mute)
		SPDIF_SCR |= SPDIF_SCR_VALCTRL;		
	else
		SPDIF_SCR &= ~SPDIF_SCR_VALCTRL;
}
#endif
