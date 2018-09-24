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

#if defined(KINETISK)

#define PREAMBLE_B  (0xE8) //11101000
#define PREAMBLE_M  (0xE2) //11100010
#define PREAMBLE_W  (0xE4) //11100100

#define VUCP_VALID   ((0xCC) << 24)
#define VUCP_INVALID ((0xD4) << 24)// To mute PCM, set VUCP = invalid.

audio_block_t * AudioOutputSPDIF::block_left_1st = NULL;
audio_block_t * AudioOutputSPDIF::block_right_1st = NULL;
audio_block_t * AudioOutputSPDIF::block_left_2nd = NULL;
audio_block_t * AudioOutputSPDIF::block_right_2nd = NULL;
uint16_t  AudioOutputSPDIF::block_left_offset = 0;
uint16_t  AudioOutputSPDIF::block_right_offset = 0;
bool AudioOutputSPDIF::update_responsibility = false;
uint32_t  AudioOutputSPDIF::vucp = VUCP_VALID;

DMAMEM static uint32_t SPDIF_tx_buffer[AUDIO_BLOCK_SAMPLES * 4]; //2 KB

DMAChannel AudioOutputSPDIF::dma(false);

static const
uint16_t bmclookup[256] = { //biphase mark encoded values (least significant bit first)
	0xcccc, 0x4ccc, 0x2ccc, 0xaccc, 0x34cc, 0xb4cc, 0xd4cc, 0x54cc,
	0x32cc, 0xb2cc, 0xd2cc, 0x52cc, 0xcacc, 0x4acc, 0x2acc, 0xaacc,
	0x334c, 0xb34c, 0xd34c, 0x534c, 0xcb4c, 0x4b4c, 0x2b4c, 0xab4c,
	0xcd4c, 0x4d4c, 0x2d4c, 0xad4c, 0x354c, 0xb54c, 0xd54c, 0x554c,
	0x332c, 0xb32c, 0xd32c, 0x532c, 0xcb2c, 0x4b2c, 0x2b2c, 0xab2c,
	0xcd2c, 0x4d2c, 0x2d2c, 0xad2c, 0x352c, 0xb52c, 0xd52c, 0x552c,
	0xccac, 0x4cac, 0x2cac, 0xacac, 0x34ac, 0xb4ac, 0xd4ac, 0x54ac,
	0x32ac, 0xb2ac, 0xd2ac, 0x52ac, 0xcaac, 0x4aac, 0x2aac, 0xaaac,
	0x3334, 0xb334, 0xd334, 0x5334, 0xcb34, 0x4b34, 0x2b34, 0xab34,
	0xcd34, 0x4d34, 0x2d34, 0xad34, 0x3534, 0xb534, 0xd534, 0x5534,
	0xccb4, 0x4cb4, 0x2cb4, 0xacb4, 0x34b4, 0xb4b4, 0xd4b4, 0x54b4,
	0x32b4, 0xb2b4, 0xd2b4, 0x52b4, 0xcab4, 0x4ab4, 0x2ab4, 0xaab4,
	0xccd4, 0x4cd4, 0x2cd4, 0xacd4, 0x34d4, 0xb4d4, 0xd4d4, 0x54d4,
	0x32d4, 0xb2d4, 0xd2d4, 0x52d4, 0xcad4, 0x4ad4, 0x2ad4, 0xaad4,
	0x3354, 0xb354, 0xd354, 0x5354, 0xcb54, 0x4b54, 0x2b54, 0xab54,
	0xcd54, 0x4d54, 0x2d54, 0xad54, 0x3554, 0xb554, 0xd554, 0x5554,
	0x3332, 0xb332, 0xd332, 0x5332, 0xcb32, 0x4b32, 0x2b32, 0xab32,
	0xcd32, 0x4d32, 0x2d32, 0xad32, 0x3532, 0xb532, 0xd532, 0x5532,
	0xccb2, 0x4cb2, 0x2cb2, 0xacb2, 0x34b2, 0xb4b2, 0xd4b2, 0x54b2,
	0x32b2, 0xb2b2, 0xd2b2, 0x52b2, 0xcab2, 0x4ab2, 0x2ab2, 0xaab2,
	0xccd2, 0x4cd2, 0x2cd2, 0xacd2, 0x34d2, 0xb4d2, 0xd4d2, 0x54d2,
	0x32d2, 0xb2d2, 0xd2d2, 0x52d2, 0xcad2, 0x4ad2, 0x2ad2, 0xaad2,
	0x3352, 0xb352, 0xd352, 0x5352, 0xcb52, 0x4b52, 0x2b52, 0xab52,
	0xcd52, 0x4d52, 0x2d52, 0xad52, 0x3552, 0xb552, 0xd552, 0x5552,
	0xccca, 0x4cca, 0x2cca, 0xacca, 0x34ca, 0xb4ca, 0xd4ca, 0x54ca,
	0x32ca, 0xb2ca, 0xd2ca, 0x52ca, 0xcaca, 0x4aca, 0x2aca, 0xaaca,
	0x334a, 0xb34a, 0xd34a, 0x534a, 0xcb4a, 0x4b4a, 0x2b4a, 0xab4a,
	0xcd4a, 0x4d4a, 0x2d4a, 0xad4a, 0x354a, 0xb54a, 0xd54a, 0x554a,
	0x332a, 0xb32a, 0xd32a, 0x532a, 0xcb2a, 0x4b2a, 0x2b2a, 0xab2a,
	0xcd2a, 0x4d2a, 0x2d2a, 0xad2a, 0x352a, 0xb52a, 0xd52a, 0x552a,
	0xccaa, 0x4caa, 0x2caa, 0xacaa, 0x34aa, 0xb4aa, 0xd4aa, 0x54aa,
	0x32aa, 0xb2aa, 0xd2aa, 0x52aa, 0xcaaa, 0x4aaa, 0x2aaa, 0xaaaa
};


void AudioOutputSPDIF::begin(void)
{

	dma.begin(true); // Allocate the DMA channel first

	block_left_1st = NULL;
	block_right_1st = NULL;

	// TODO: should we set & clear the I2S_TCSR_SR bit here?
	config_SPDIF();
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
			hi  = bmclookup[(uint8_t)(sample >> 8)];
			lo  = bmclookup[(uint8_t) sample];
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
			hi  = bmclookup[(uint8_t)(sample >> 8)];
			lo  = bmclookup[(uint8_t)sample];
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
  #define MCLK_MULT 8
  #define MCLK_DIV  153
  #define MCLK_SRC  0
#elif F_CPU == 240000000
  #define MCLK_MULT 4
  #define MCLK_DIV  85
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


void AudioOutputSPDIF::config_SPDIF(void)
{
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
	CORE_PIN23_CONFIG = PORT_PCR_MUX(6); // pin 23, PTC2, I2S0_TX_FS (LRCLK)
	CORE_PIN9_CONFIG  = PORT_PCR_MUX(6); // pin  9, PTC3, I2S0_TX_BCLK
//	CORE_PIN11_CONFIG = PORT_PCR_MUX(6); // pin 11, PTC6, I2S0_MCLK
#endif
}


#elif defined(KINETISL)

void AudioOutputSPDIF::update(void)
{

	audio_block_t *block;
	block = receiveReadOnly(0); // input 0 = left channel
	if (block) release(block);
	block = receiveReadOnly(1); // input 1 = right channel
	if (block) release(block);
}

#endif
