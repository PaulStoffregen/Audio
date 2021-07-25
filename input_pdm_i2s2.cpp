/* Audio Library for Teensy 3.X
 * Copyright (c) 2018, Paul Stoffregen, paul@pjrc.com
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

#if defined(__IMXRT1052__) || defined(__IMXRT1062__)

#include <Arduino.h>
#include "input_pdm_i2s2.h"
#include "utility/dspinst.h"
#include "utility/imxrt_hw.h"


// Decrease this for more mic gain, increase for range to accommodate loud sounds
#define RSHIFT  2

// Pulse Density Modulation (PDM) is a tech trade-off of questionable value.
// The only advantage is your delta-sigma based ADC can be less expensive,
// since it can omit the digital low-pass filter.  But it limits the ADC
// to a single bit modulator, and it imposes the filtering requirement onto
// your microcontroller.  Generally digital filtering is much less expensive
// to implement with dedicated digital logic than firmware in a general
// purpose microcontroller.  PDM probably makes more sense with an ASIC or
// highly integrated SoC, or maybe even with a "real" DSP chip.  Using a
// microcontroller, maybe not so much?
//
// This code imposes considerable costs.  It consumes 39% of the CPU time
// when running at 96 MHz, and uses 2104 bytes of RAM for buffering and 32768
// bytes of flash for a large table lookup to optimize the filter computation.
//
// On the plus side, this filter is a 512 tap FIR with approximately +/- 1 dB
// gain flatness to 10 kHz bandwidth.  That won't impress any audio enthusiasts,
// but its performance should be *much* better than the rapid passband rolloff
// of Cascaded Integrator Comb (CIC) or moving average filters.

DMAMEM __attribute__((aligned(32))) static uint32_t pdm_buffer[AUDIO_BLOCK_SAMPLES*4];
static uint32_t leftover[14];
audio_block_t * AudioInputPDM2::block_left = NULL;
bool AudioInputPDM2::update_responsibility = false;
DMAChannel AudioInputPDM2::dma(false);




// T4.x version
void AudioInputPDM2::begin(void)
{
  dma.begin(true); // Allocate the DMA channel first

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

  int rsync = 0;
  int tsync = 1;
  // clear SAI2_CLK register locations
  CCM_CSCMR1 = (CCM_CSCMR1 & ~(CCM_CSCMR1_SAI2_CLK_SEL_MASK))
    | CCM_CSCMR1_SAI2_CLK_SEL(2); // &0x03 // (0,1,2): PLL3PFD0, PLL5, PLL4
  CCM_CS2CDR = (CCM_CS2CDR & ~(CCM_CS2CDR_SAI2_CLK_PRED_MASK | CCM_CS2CDR_SAI2_CLK_PODF_MASK))
    | CCM_CS2CDR_SAI2_CLK_PRED(n1-1) // &0x07
    | CCM_CS2CDR_SAI2_CLK_PODF(n2-1); // &0x3f
  
  // Select MCLK - SAI2 doesn't seem to have write access to MCLK2, so use MCLK3 (not enabled as a pin anyway)
  IOMUXC_GPR_GPR1 = (IOMUXC_GPR_GPR1 & ~(IOMUXC_GPR_GPR1_SAI2_MCLK3_SEL_MASK))
    | (IOMUXC_GPR_GPR1_SAI2_MCLK_DIR | IOMUXC_GPR_GPR1_SAI2_MCLK3_SEL(0));

  // all the pins for SAI2 seem to be ALT2 io-muxed
  //// CORE_PIN33_CONFIG = 2;  //2:MCLK
  CORE_PIN4_CONFIG = 2;  //2:TX_BCLK
  //// CORE_PIN3_CONFIG = 2;  //2:RX_SYNC  // LRCLK

  I2S2_TMR = 0;
  //I2S2_TCSR = (1<<25); //Reset
  I2S2_TCR1 = I2S_TCR1_RFW(1);
  I2S2_TCR2 = I2S_TCR2_SYNC(tsync) | I2S_TCR2_BCP | (I2S_TCR2_BCD | I2S_TCR2_DIV((1)) | I2S_TCR2_MSEL(1)); // sync=0; tx is async;
  I2S2_TCR3 = I2S_TCR3_TCE;
  I2S2_TCR4 = I2S_TCR4_FRSZ((2-1)) | I2S_TCR4_SYWD((32-1)) | I2S_TCR4_MF | I2S_TCR4_FSD | I2S_TCR4_FSE | I2S_TCR4_FSP;
  I2S2_TCR5 = I2S_TCR5_WNW((32-1)) | I2S_TCR5_W0W((32-1)) | I2S_TCR5_FBT((32-1));

  I2S2_RMR = 0;
  //I2S2_RCSR = (1<<25); //Reset
  I2S2_RCR1 = I2S_RCR1_RFW(2);
  I2S2_RCR2 = I2S_RCR2_SYNC(rsync) | I2S_RCR2_BCP | (I2S_RCR2_BCD | I2S_RCR2_DIV((1)) | I2S_RCR2_MSEL(1));  // sync=0; rx is async;
  I2S2_RCR3 = I2S_RCR3_RCE;
  I2S2_RCR4 = I2S_RCR4_FRSZ((2-1)) | I2S_RCR4_SYWD((32-1)) | I2S_RCR4_MF /* | I2S_RCR4_FSE */ | I2S_RCR4_FSP | I2S_RCR4_FSD;
  I2S2_RCR5 = I2S_RCR5_WNW((32-1)) | I2S_RCR5_W0W((32-1)) | I2S_RCR5_FBT((32-1));

  CORE_PIN5_CONFIG  = 2;  //2:RX_DATA0
  IOMUXC_SAI2_RX_DATA0_SELECT_INPUT = 0;
  
  dma.TCD->SADDR = &I2S2_RDR0;
  dma.TCD->SOFF = 0;
  dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
  dma.TCD->NBYTES_MLNO = 4;
  dma.TCD->SLAST = 0;
  dma.TCD->DADDR = pdm_buffer;
  dma.TCD->DOFF = 4;
  dma.TCD->CITER_ELINKNO = sizeof(pdm_buffer) / 4;
  dma.TCD->DLASTSGA = -sizeof(pdm_buffer);
  dma.TCD->BITER_ELINKNO = sizeof(pdm_buffer) / 4;
  dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;

  dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI2_RX);
  update_responsibility = update_setup();
  dma.enable();

  I2S2_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE | I2S_RCSR_FRDE | I2S_RCSR_FR;

  dma.attachInterrupt(isr);
}

extern const int16_t enormous_pdm_filter_table[];

extern int pdm_filter(const uint32_t *buf);
extern int pdm_filter(const uint32_t *buf1, unsigned int n, const uint32_t *buf2);

void AudioInputPDM2::isr(void)
{
	uint32_t daddr;
	const uint32_t *src;
	audio_block_t *left;

	//digitalWriteFast(14, HIGH);
	daddr = (uint32_t)(dma.TCD->DADDR);
	dma.clearInterrupt();

	if (daddr < (uint32_t)pdm_buffer + sizeof(pdm_buffer) / 2) {
		// DMA is receiving to the first half of the buffer
		// need to remove data from the second half
		src = pdm_buffer + AUDIO_BLOCK_SAMPLES*2;
	} else {
		// DMA is receiving to the second half of the buffer
		// need to remove data from the first half
		src = pdm_buffer;
	}
	if (update_responsibility) AudioStream::update_all();
	left = block_left;
	if (left != NULL) {
		// TODO: should find a way to pass the unfiltered data to
		// the lower priority update.  This burns ~40% of the CPU
		// time in a high priority interrupt.  Not ideal.  :(
		int16_t *dest = left->data;
		arm_dcache_delete ((void*) src, sizeof (pdm_buffer) >> 1);
		for (unsigned int i=0; i < 14; i += 2) {
			*dest++ = pdm_filter(leftover + i, 7 - (i >> 1), src);
		}
		for (unsigned int i=0; i < AUDIO_BLOCK_SAMPLES*2-14; i += 2) {
			*dest++ = pdm_filter(src + i);
		}
		for (unsigned int i=0; i < 14; i++) {
			leftover[i] = src[AUDIO_BLOCK_SAMPLES*2 - 14 + i];
		}
		//left->data[0] = 0x7FFF;
	}
	//digitalWriteFast(14, LOW);
}

void AudioInputPDM2::update(void)
{
	audio_block_t *new_left, *out_left;
	new_left = allocate();
	__disable_irq();
	out_left = block_left;
	block_left = new_left;
	__enable_irq();
	if (out_left) {
		transmit(out_left, 0);
		release(out_left);
	}
}

#endif
