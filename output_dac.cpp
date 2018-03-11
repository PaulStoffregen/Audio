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

#include <Arduino.h>
#include "output_dac.h"
#include "utility/pdb.h"

#if defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)

DMAMEM static uint16_t dac_buffer[AUDIO_BLOCK_SAMPLES*2];
audio_block_t * AudioOutputAnalog::block_left_1st = NULL;
audio_block_t * AudioOutputAnalog::block_left_2nd = NULL;
bool AudioOutputAnalog::update_responsibility = false;
DMAChannel AudioOutputAnalog::dma(false);

void AudioOutputAnalog::begin(void)
{
	dma.begin(true); // Allocate the DMA channel first

	SIM_SCGC2 |= SIM_SCGC2_DAC0;
	DAC0_C0 = DAC_C0_DACEN;                   // 1.2V VDDA is DACREF_2
	// slowly ramp up to DC voltage, approx 1/4 second
	for (int16_t i=0; i<=2048; i+=8) {
		*(int16_t *)&(DAC0_DAT0L) = i;
		delay(1);
	}

	// set the programmable delay block to trigger DMA requests
	if (!(SIM_SCGC6 & SIM_SCGC6_PDB)
	  || (PDB0_SC & PDB_CONFIG) != PDB_CONFIG
	  || PDB0_MOD != PDB_PERIOD
	  || PDB0_IDLY != 1
	  || PDB0_CH0C1 != 0x0101) {
		SIM_SCGC6 |= SIM_SCGC6_PDB;
		PDB0_IDLY = 1;
		PDB0_MOD = PDB_PERIOD;
		PDB0_SC = PDB_CONFIG | PDB_SC_LDOK;
		PDB0_SC = PDB_CONFIG | PDB_SC_SWTRIG;
		PDB0_CH0C1 = 0x0101;
	}

	dma.TCD->SADDR = dac_buffer;
	dma.TCD->SOFF = 2;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	dma.TCD->NBYTES_MLNO = 2;
	dma.TCD->SLAST = -sizeof(dac_buffer);
	dma.TCD->DADDR = &DAC0_DAT0L;
	dma.TCD->DOFF = 0;
	dma.TCD->CITER_ELINKNO = sizeof(dac_buffer) / 2;
	dma.TCD->DLASTSGA = 0;
	dma.TCD->BITER_ELINKNO = sizeof(dac_buffer) / 2;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_PDB);
	update_responsibility = update_setup();
	dma.enable();
	dma.attachInterrupt(isr);
}

void AudioOutputAnalog::analogReference(int ref)
{
	// TODO: this should ramp gradually to the new DC level
	if (ref == INTERNAL) {
		DAC0_C0 &= ~DAC_C0_DACRFS; // 1.2V
	} else {
		DAC0_C0 |= DAC_C0_DACRFS;  // 3.3V
	}
}


void AudioOutputAnalog::update(void)
{
	audio_block_t *block;
	block = receiveReadOnly(0); // input 0
	if (block) {
		__disable_irq();
		if (block_left_1st == NULL) {
			block_left_1st = block;
			__enable_irq();
		} else if (block_left_2nd == NULL) {
			block_left_2nd = block;
			__enable_irq();
		} else {
			audio_block_t *tmp = block_left_1st;
			block_left_1st = block_left_2nd;
			block_left_2nd = block;
			__enable_irq();
			release(tmp);
		}
	}
}

// TODO: the DAC has much higher bandwidth than the datasheet says
// can we output a 2X oversampled output, for easier filtering?

void AudioOutputAnalog::isr(void)
{
	const int16_t *src, *end;
	int16_t *dest;
	audio_block_t *block;
	uint32_t saddr;

	saddr = (uint32_t)(dma.TCD->SADDR);
	dma.clearInterrupt();
	if (saddr < (uint32_t)dac_buffer + sizeof(dac_buffer) / 2) {
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		dest = (int16_t *)&dac_buffer[AUDIO_BLOCK_SAMPLES];
		end = (int16_t *)&dac_buffer[AUDIO_BLOCK_SAMPLES*2];
	} else {
		// DMA is transmitting the second half of the buffer
		// so we must fill the first half
		dest = (int16_t *)dac_buffer;
		end = (int16_t *)&dac_buffer[AUDIO_BLOCK_SAMPLES];
	}
	block = AudioOutputAnalog::block_left_1st;
	if (block) {
		src = block->data;
		do {
			// TODO: this should probably dither
			*dest++ = ((*src++) + 32768) >> 4;
		} while (dest < end);
		AudioStream::release(block);
		AudioOutputAnalog::block_left_1st = AudioOutputAnalog::block_left_2nd;
		AudioOutputAnalog::block_left_2nd = NULL;
	} else {
		do {
			*dest++ = 2048;
		} while (dest < end);
	}
	if (AudioOutputAnalog::update_responsibility) AudioStream::update_all();
}



#elif defined (__MKL26Z64__)

DMAMEM static uint16_t dac_buffer1[AUDIO_BLOCK_SAMPLES];
DMAMEM static uint16_t dac_buffer2[AUDIO_BLOCK_SAMPLES];
audio_block_t * AudioOutputAnalog::block_left_1st = NULL;
bool AudioOutputAnalog::update_responsibility = false;
DMAChannel AudioOutputAnalog::dma1(false);
DMAChannel AudioOutputAnalog::dma2(false);

void AudioOutputAnalog::begin(void)
{
	dma1.begin(true); // Allocate the DMA channels first
	dma2.begin(true); // Allocate the DMA channels first

	delay(2500);
	Serial.println("AudioOutputAnalog begin");
	delay(10);

	SIM_SCGC6 |= SIM_SCGC6_DAC0;
	DAC0_C0 = DAC_C0_DACEN | DAC_C0_DACRFS;		// VDDA (3.3V) ref
	// slowly ramp up to DC voltage, approx 1/4 second
	for (int16_t i=0; i<2048; i+=8) {
		*(int16_t *)&(DAC0_DAT0L) = i;
		delay(1);
	}

	// commandeer FTM1 for timing (PWM on pin 3 & 4 will become 22 kHz)
	FTM1_SC = 0;
	FTM1_CNT = 0;
	FTM1_MOD = (uint32_t)((F_PLL/2) / AUDIO_SAMPLE_RATE_EXACT + 0.5);
	FTM1_SC = FTM_SC_CLKS(1);

	dma1.sourceBuffer(dac_buffer1, sizeof(dac_buffer1));
	dma1.destination(*(int16_t *)&DAC0_DAT0L);
	dma1.interruptAtCompletion();
	dma1.disableOnCompletion();
	dma1.triggerAtCompletionOf(dma2);
	dma1.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM1_OV);
	dma1.attachInterrupt(isr1);

	dma2.sourceBuffer(dac_buffer2, sizeof(dac_buffer2));
	dma2.destination(*(int16_t *)&DAC0_DAT0L);
	dma2.interruptAtCompletion();
	dma2.disableOnCompletion();
	dma2.triggerAtCompletionOf(dma1);
	dma2.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM1_OV);
	dma2.attachInterrupt(isr2);

	update_responsibility = update_setup();
/*
	dma.TCD->SADDR = dac_buffer;
	dma.TCD->SOFF = 2;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	dma.TCD->NBYTES_MLNO = 2;
	dma.TCD->SLAST = -sizeof(dac_buffer);
	dma.TCD->DADDR = &DAC0_DAT0L;
	dma.TCD->DOFF = 0;
	dma.TCD->CITER_ELINKNO = sizeof(dac_buffer) / 2;
	dma.TCD->DLASTSGA = 0;
	dma.TCD->BITER_ELINKNO = sizeof(dac_buffer) / 2;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_PDB);
	update_responsibility = update_setup();
	dma.enable();
	dma.attachInterrupt(isr);
*/
}

void AudioOutputAnalog::isr1(void)
{
	dma1.clearInterrupt();

}

void AudioOutputAnalog::isr2(void)
{
	dma2.clearInterrupt();


}


void AudioOutputAnalog::update(void)
{
	audio_block_t *block;
	block = receiveReadOnly();
	if (block) {
		__disable_irq();
		if (block_left_1st == NULL) {
			block_left_1st = block;
			__enable_irq();
		} else {
			audio_block_t *tmp = block_left_1st;
			block_left_1st = block;
			__enable_irq();
			release(tmp);
		}
	}
}




#else

void AudioOutputAnalog::begin(void)
{
}

void AudioOutputAnalog::update(void)
{
	audio_block_t *block;
	block = receiveReadOnly(0); // input 0
	if (block) release(block);
}

#endif // defined(__MK20DX256__)



