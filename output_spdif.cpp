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
DMAMEM __attribute__((aligned(32)))
static int32_t SPDIF_tx_buffer[AUDIO_BLOCK_SAMPLES * 4]; //2 KB

#if defined(KINETISK) || defined(__IMXRT1062__)

#define PREAMBLE_B  ((0xE8) << 16) //11101000
#define PREAMBLE_M  ((0xE2) << 16) //11100010
#define PREAMBLE_W  ((0xE4) << 16) //11100100

// 1 bit of aux is included here, since it gets used as the "real" parity
#define VUCP_VALID    (0xCC008000) // V=0,U=0,C=0,P=0
#define VUCP_INVALID  (0xAC008000) // V=1,U=1,C=0,P=0. To mute PCM, set VUCP = invalid.
#define VUCP_C_TOGGLE (0x18000000) // XOR with VUCP to flip U and C (keep parity even)

int32_t  AudioOutputSPDIF::vucp = VUCP_VALID;

static const union {
	struct {
		// byte 0
		uint32_t pro:1; // 0=consumer format, 1=professional format
		uint32_t audio:1; // 0=linear PCM, 1=non-PCM
		uint32_t copy_permitted:1; // 0=copy inhibited, 1=copy permitted
		uint32_t pre_emphasis:3; // 0=none(2ch), 1=50/15us(2ch), 2/3=reserved(2ch), others=reserved(4ch)
		uint32_t mode:2; // 0=mode 0 (defines next 24 bits)
		// byte 1
		uint32_t category:7; // 0=general
		uint32_t generation:1; // 0=none/1st generation, 1=original/commercial
		// byte 2
		uint32_t source:4; // 0=unspecified
		uint32_t channel:4; // channel number, 0=unspecified
		// byte 3
		uint32_t Fs:4; // 0=44.1kHz, 2=48, 3=32, others=reserved
		uint32_t clock_accuracy:2; // 0=+/- 1000ppm, 1=+/- 50ppm, 2=variable, 3=reserved
		uint32_t :2; // reserved
	};
	uint32_t bits;

	bool operator[] (int index) const {
		if (index >= 0 && index < 32)
			return (bits >> index) & 1;
		return false;
	}
} consumer_channel_status = { // fields not initialized here will be zero
	.copy_permitted = 1,
};

FLASHMEM
void AudioOutputSPDIF::begin(void)
{

	memset(SPDIF_tx_buffer,0,sizeof SPDIF_tx_buffer); // ensure we start with silence
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

#elif defined(__IMXRT1062__)
	CORE_PIN7_CONFIG  = 3;  //1:TX_DATA0	
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

// pulls a 16-bit sample from the given channel, returns encoded 32-bit value
static int32_t next_encoded_sample(const int16_t* &src) {
	if (src) {
		uint16_t sample = (uint16_t)*src++;

		int32_t hi = spdif_bmclookup[sample >> 8];
		int32_t lo = ~spdif_bmclookup[sample & 0xFF];
		return (lo << 16) ^ hi;
	}

	// silence: return encoded 0
	return 0xCCCCCCCC;
}

void AudioOutputSPDIF::isr(void)
{
	static int frame = 0;
	int32_t *end, *dest;
	uint32_t saddr;

#if defined(KINETISK) || defined(__IMXRT1062__)
	saddr = (uint32_t)(dma.TCD->SADDR);
#endif
	dma.clearInterrupt();
	if (saddr < (uint32_t)SPDIF_tx_buffer + sizeof(SPDIF_tx_buffer) / 2) {
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		dest = &SPDIF_tx_buffer[AUDIO_BLOCK_SAMPLES * 4/2];
		end = &SPDIF_tx_buffer[AUDIO_BLOCK_SAMPLES * 4];
		if (AudioOutputSPDIF::update_responsibility) AudioStream::update_all();
	} else {
		// DMA is transmitting the second half of the buffer
		// so we must fill the first half
		dest = SPDIF_tx_buffer;
		end = &SPDIF_tx_buffer[AUDIO_BLOCK_SAMPLES * 4/2];
	}

	const int16_t* leftsrc = block_left_1st != NULL ? &block_left_1st->data[block_left_offset] : NULL;
	const int16_t* rightsrc = block_right_1st != NULL ? &block_right_1st->data[block_right_offset] : NULL;

	do {
		// each output sample is 8 bytes, generate 2 stereo samples (2x2x8=32) to fill a cacheline
		for (int i=0; i < 2; i++) {
			int32_t left  = next_encoded_sample(leftsrc);
			int32_t right = next_encoded_sample(rightsrc);

			uint16_t laux = 0x3333 ^ (left >> 31);
			uint16_t raux = 0x3333 ^ (right >> 31);

			if (++frame > 191) frame = 0;

			// 1 byte of previous subframe 1, 3 bytes subframe 0 (channel 1)
			dest[0] = vucp | (frame==0 ? PREAMBLE_B : PREAMBLE_M) | laux;
			dest[1] = left;
			// 1 byte of subframe 0, 3 bytes of subframe 1 (channel 2)
			dest[2] = vucp | PREAMBLE_W | raux;
			dest[3] = right;

			/* channel status is identical for both channels, but
			 * the status for channel 2 is one frame delayed due to
			 * the one byte offset in our output.
			 */
			if (frame <= 32) {
				// set status for previous subframe 1/channel 2
				if (consumer_channel_status[frame-1]) dest[0] ^= VUCP_C_TOGGLE;
				// set status for subframe 0/channel 1
				if (consumer_channel_status[frame]) dest[2] ^= VUCP_C_TOGGLE;
			}

			dest += 4;
		}

		#if IMXRT_CACHE_ENABLED >= 2
		// flush
		SCB_CACHE_DCCMVAC = (uint32_t)(dest-8);
		#endif
	} while (dest < end);

	if (leftsrc) {
		if (block_left_offset >= AUDIO_BLOCK_SAMPLES/2) {
			block_left_offset = 0;
			release(block_left_1st);
			block_left_1st = block_left_2nd;
			block_left_2nd = NULL;
		}
		else block_left_offset += AUDIO_BLOCK_SAMPLES/2;
	}
	if (rightsrc) {
		if (block_right_offset >= AUDIO_BLOCK_SAMPLES/2) {
			block_right_offset = 0;
			release(block_right_1st);
			block_right_1st = block_right_2nd;
			block_right_2nd = NULL;
		}
		else block_right_offset += AUDIO_BLOCK_SAMPLES/2;
	}

	#if IMXRT_CACHE_ENABLED >= 2
	// ensure cache operations are complete
	asm volatile("dsb");
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

FLASHMEM
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

#elif defined(__IMXRT1062__)

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

#endif // KINETISK || __IMXRT1062__

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
