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

#include "input_i2s.h"
#include "output_i2s.h"

DMAMEM static uint32_t i2s_rx_buffer[AUDIO_BLOCK_SAMPLES];
audio_block_t * AudioInputI2S::block_left = NULL;
audio_block_t * AudioInputI2S::block_right = NULL;
uint16_t AudioInputI2S::block_offset = 0;
bool AudioInputI2S::update_responsibility = false;


void AudioInputI2S::begin(void)
{
	//block_left_1st = NULL;
	//block_right_1st = NULL;

	//pinMode(3, OUTPUT);
	//digitalWriteFast(3, HIGH);
	//delayMicroseconds(500);
	//digitalWriteFast(3, LOW);

	// TODO: should we set & clear the I2S_RCSR_SR bit here?
	AudioOutputI2S::config_i2s();

	CORE_PIN13_CONFIG = PORT_PCR_MUX(4); // pin 13, PTC5, I2S0_RXD0

	DMA_CR = 0;
	DMA_TCD1_SADDR = &I2S0_RDR0;
	DMA_TCD1_SOFF = 0;
	DMA_TCD1_ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	DMA_TCD1_NBYTES_MLNO = 2;
	DMA_TCD1_SLAST = 0;
	DMA_TCD1_DADDR = i2s_rx_buffer;
	DMA_TCD1_DOFF = 2;
	DMA_TCD1_CITER_ELINKNO = sizeof(i2s_rx_buffer) / 2;
	DMA_TCD1_DLASTSGA = -sizeof(i2s_rx_buffer);
	DMA_TCD1_BITER_ELINKNO = sizeof(i2s_rx_buffer) / 2;
	DMA_TCD1_CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;

	DMAMUX0_CHCFG1 = DMAMUX_DISABLE;
	DMAMUX0_CHCFG1 = DMAMUX_SOURCE_I2S0_RX | DMAMUX_ENABLE;
	update_responsibility = update_setup();
	DMA_SERQ = 1;

	// TODO: is I2S_RCSR_BCE appropriate if sync'd to transmitter clock?
	//I2S0_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE | I2S_RCSR_FRDE | I2S_RCSR_FR;
	I2S0_RCSR |= I2S_RCSR_RE | I2S_RCSR_FRDE | I2S_RCSR_FR;
	NVIC_ENABLE_IRQ(IRQ_DMA_CH1);
}

void dma_ch1_isr(void)
{
	uint32_t daddr, offset;
	const int16_t *src, *end;
	int16_t *dest_left, *dest_right;
	audio_block_t *left, *right;

	//digitalWriteFast(3, HIGH);
	daddr = (uint32_t)DMA_TCD1_DADDR;
        DMA_CINT = 1;

	if (daddr < (uint32_t)i2s_rx_buffer + sizeof(i2s_rx_buffer) / 2) {
		// DMA is receiving to the first half of the buffer
		// need to remove data from the second half
		src = (int16_t *)&i2s_rx_buffer[AUDIO_BLOCK_SAMPLES/2];
		end = (int16_t *)&i2s_rx_buffer[AUDIO_BLOCK_SAMPLES];
		if (AudioInputI2S::update_responsibility) AudioStream::update_all();
	} else {
		// DMA is receiving to the second half of the buffer
		// need to remove data from the first half
		src = (int16_t *)&i2s_rx_buffer[0];
		end = (int16_t *)&i2s_rx_buffer[AUDIO_BLOCK_SAMPLES/2];
	}
	left = AudioInputI2S::block_left;
	right = AudioInputI2S::block_right;
	if (left != NULL && right != NULL) {
		offset = AudioInputI2S::block_offset;
		if (offset <= AUDIO_BLOCK_SAMPLES/2) {
			dest_left = &(left->data[offset]);
			dest_right = &(right->data[offset]);
			AudioInputI2S::block_offset = offset + AUDIO_BLOCK_SAMPLES/2;
			do {
				//n = *src++;
				//*dest_left++ = (int16_t)n;
				//*dest_right++ = (int16_t)(n >> 16);
				*dest_left++ = *src++;
				*dest_right++ = *src++;
			} while (src < end);
		}
	}
	//digitalWriteFast(3, LOW);
}



void AudioInputI2S::update(void)
{
	audio_block_t *new_left=NULL, *new_right=NULL, *out_left=NULL, *out_right=NULL;

	// allocate 2 new blocks, but if one fails, allocate neither
	new_left = allocate();
	if (new_left != NULL) {
		new_right = allocate();
		if (new_right == NULL) {
			release(new_left);
			new_left = NULL;
		}
	}
	__disable_irq();
	if (block_offset >= AUDIO_BLOCK_SAMPLES) {
		// the DMA filled 2 blocks, so grab them and get the
		// 2 new blocks to the DMA, as quickly as possible
		out_left = block_left;
		block_left = new_left;
		out_right = block_right;
		block_right = new_right;
		block_offset = 0;
		__enable_irq();
		// then transmit the DMA's former blocks
		transmit(out_left, 0);
		release(out_left);
		transmit(out_right, 1);
		release(out_right);
		//Serial.print(".");
	} else if (new_left != NULL) {
		// the DMA didn't fill blocks, but we allocated blocks
		if (block_left == NULL) {
			// the DMA doesn't have any blocks to fill, so
			// give it the ones we just allocated
			block_left = new_left;
			block_right = new_right;
			block_offset = 0;
			__enable_irq();
		} else {
			// the DMA already has blocks, doesn't need these
			__enable_irq();
			release(new_left);
			release(new_right);
		}
	} else {
		// The DMA didn't fill blocks, and we could not allocate
		// memory... the system is likely starving for memory!
		// Sadly, there's nothing we can do.
		__enable_irq();
	}
}


/******************************************************************/


void AudioInputI2Sslave::begin(void)
{
	//block_left_1st = NULL;
	//block_right_1st = NULL;

	//pinMode(3, OUTPUT);
	//digitalWriteFast(3, HIGH);
	//delayMicroseconds(500);
	//digitalWriteFast(3, LOW);

	AudioOutputI2Sslave::config_i2s();

	CORE_PIN13_CONFIG = PORT_PCR_MUX(4); // pin 13, PTC5, I2S0_RXD0

	DMA_CR = 0;
	DMA_TCD1_SADDR = &I2S0_RDR0;
	DMA_TCD1_SOFF = 0;
	DMA_TCD1_ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	DMA_TCD1_NBYTES_MLNO = 2;
	DMA_TCD1_SLAST = 0;
	DMA_TCD1_DADDR = i2s_rx_buffer;
	DMA_TCD1_DOFF = 2;
	DMA_TCD1_CITER_ELINKNO = sizeof(i2s_rx_buffer) / 2;
	DMA_TCD1_DLASTSGA = -sizeof(i2s_rx_buffer);
	DMA_TCD1_BITER_ELINKNO = sizeof(i2s_rx_buffer) / 2;
	DMA_TCD1_CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;

	DMAMUX0_CHCFG1 = DMAMUX_DISABLE;
	DMAMUX0_CHCFG1 = DMAMUX_SOURCE_I2S0_RX | DMAMUX_ENABLE;
	update_responsibility = update_setup();
	DMA_SERQ = 1;

	// TODO: is I2S_RCSR_BCE appropriate if sync'd to transmitter clock?
	//I2S0_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE | I2S_RCSR_FRDE | I2S_RCSR_FR;
	I2S0_RCSR |= I2S_RCSR_RE | I2S_RCSR_FRDE | I2S_RCSR_FR;
	NVIC_ENABLE_IRQ(IRQ_DMA_CH1);
}




