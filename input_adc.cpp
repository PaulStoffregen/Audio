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

#include "input_adc.h"
#include "utility/pdb.h"

DMAMEM static uint16_t analog_rx_buffer[AUDIO_BLOCK_SAMPLES];
audio_block_t * AudioInputAnalog::block_left = NULL;
uint16_t AudioInputAnalog::block_offset = 0;
bool AudioInputAnalog::update_responsibility = false;

// #define PDB_CONFIG (PDB_SC_TRGSEL(15) | PDB_SC_PDBEN | PDB_SC_CONT)
// #define PDB_PERIOD 1087 // 48e6 / 44100

void AudioInputAnalog::begin(unsigned int pin)
{
	uint32_t i, sum=0;

	// pin specified in user sketches should be A0 to A13
	// numbers can be used, but the recommended usage is
	// with the named constants A0 to A13
	//   constants A0-A9 are actually 14 to 23
	//   constants A10-A13 are actually 34 to 37
	if (pin > 23 && !(pin >= 34 && pin <= 37)) return;

	//pinMode(2, OUTPUT);
	//pinMode(3, OUTPUT);
	//digitalWriteFast(3, HIGH);
	//delayMicroseconds(500);
	//digitalWriteFast(3, LOW);

	// Configure the ADC and run at least one software-triggered
	// conversion.  This completes the self calibration stuff and
	// leaves the ADC in a state that's mostly ready to use
	analogReadRes(16);
	analogReference(INTERNAL); // range 0 to 1.2 volts
	//analogReference(DEFAULT); // range 0 to 3.3 volts
	analogReadAveraging(8);
	// Actually, do many normal reads, to start with a nice DC level
	for (i=0; i < 1024; i++) {
		sum += analogRead(pin);
	}
	dc_average = sum >> 10;

	// testing only, enable adc interrupt
	//ADC0_SC1A |= ADC_SC1_AIEN;
	//while ((ADC0_SC1A & ADC_SC1_COCO) == 0) ; // wait
	//NVIC_ENABLE_IRQ(IRQ_ADC0);

	// set the programmable delay block to trigger the ADC at 44.1 kHz
	SIM_SCGC6 |= SIM_SCGC6_PDB;
	PDB0_MOD = PDB_PERIOD;
	PDB0_SC = PDB_CONFIG | PDB_SC_LDOK;
	PDB0_SC = PDB_CONFIG | PDB_SC_SWTRIG;
	PDB0_CH0C1 = 0x0101;

	// enable the ADC for hardware trigger and DMA
	ADC0_SC2 |= ADC_SC2_ADTRG | ADC_SC2_DMAEN;

	// set up a DMA channel to store the ADC data
	SIM_SCGC7 |= SIM_SCGC7_DMA;
	SIM_SCGC6 |= SIM_SCGC6_DMAMUX;
	DMA_CR = 0;
	DMA_TCD2_SADDR = &ADC0_RA;
	DMA_TCD2_SOFF = 0;
	DMA_TCD2_ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	DMA_TCD2_NBYTES_MLNO = 2;
	DMA_TCD2_SLAST = 0;
	DMA_TCD2_DADDR = analog_rx_buffer;
	DMA_TCD2_DOFF = 2;
	DMA_TCD2_CITER_ELINKNO = sizeof(analog_rx_buffer) / 2;
	DMA_TCD2_DLASTSGA = -sizeof(analog_rx_buffer);
	DMA_TCD2_BITER_ELINKNO = sizeof(analog_rx_buffer) / 2;
	DMA_TCD2_CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	DMAMUX0_CHCFG2 = DMAMUX_DISABLE;
	DMAMUX0_CHCFG2 = DMAMUX_SOURCE_ADC0 | DMAMUX_ENABLE;
	update_responsibility = update_setup();
	DMA_SERQ = 2;
	NVIC_ENABLE_IRQ(IRQ_DMA_CH2);
}

void dma_ch2_isr(void)
{
	uint32_t daddr, offset;
	const uint16_t *src, *end;
	uint16_t *dest_left;
	audio_block_t *left;

	//digitalWriteFast(3, HIGH);
	daddr = (uint32_t)DMA_TCD2_DADDR;
	DMA_CINT = 2;

	if (daddr < (uint32_t)analog_rx_buffer + sizeof(analog_rx_buffer) / 2) {
		// DMA is receiving to the first half of the buffer
		// need to remove data from the second half
		src = (uint16_t *)&analog_rx_buffer[AUDIO_BLOCK_SAMPLES/2];
		end = (uint16_t *)&analog_rx_buffer[AUDIO_BLOCK_SAMPLES];
		if (AudioInputAnalog::update_responsibility) AudioStream::update_all();
	} else {
		// DMA is receiving to the second half of the buffer
		// need to remove data from the first half
		src = (uint16_t *)&analog_rx_buffer[0];
		end = (uint16_t *)&analog_rx_buffer[AUDIO_BLOCK_SAMPLES/2];
	}
	left = AudioInputAnalog::block_left;
	if (left != NULL) {
		offset = AudioInputAnalog::block_offset;
		if (offset > AUDIO_BLOCK_SAMPLES/2) offset = AUDIO_BLOCK_SAMPLES/2;
		//if (offset <= AUDIO_BLOCK_SAMPLES/2) {
			dest_left = (uint16_t *)&(left->data[offset]);
			AudioInputAnalog::block_offset = offset + AUDIO_BLOCK_SAMPLES/2;
			do {
				*dest_left++ = *src++;
			} while (src < end);
		//}
	}
	//digitalWriteFast(3, LOW);
}


#if 0
void adc0_isr(void)
{
	uint32_t tmp = ADC0_RA; // read ADC result to clear interrupt
	digitalWriteFast(3, HIGH);
	delayMicroseconds(1);
	digitalWriteFast(3, LOW);
}
#endif


void AudioInputAnalog::update(void)
{
	audio_block_t *new_left=NULL, *out_left=NULL;
	unsigned int dc, offset;
	int16_t s, *p, *end;

	// allocate new block (ok if NULL)
	new_left = allocate();

	__disable_irq();
	offset = block_offset;
	if (offset < AUDIO_BLOCK_SAMPLES) {
		// the DMA didn't fill a block
		if (new_left != NULL) {
			// but we allocated a block
			if (block_left == NULL) {
				// the DMA doesn't have any blocks to fill, so
				// give it the one we just allocated
				block_left = new_left;
				block_offset = 0;
				__enable_irq();
	 			 //Serial.println("fail1");
			} else {
				// the DMA already has blocks, doesn't need this
				__enable_irq();
				release(new_left);
	 			 //Serial.print("fail2, offset=");
	 			 //Serial.println(offset);
			}
		} else {
			// The DMA didn't fill a block, and we could not allocate
			// memory... the system is likely starving for memory!
			// Sadly, there's nothing we can do.
			__enable_irq();
	 		 //Serial.println("fail3");
		}
		return;
	}
	// the DMA filled a block, so grab it and get the
	// new block to the DMA, as quickly as possible
	out_left = block_left;
	block_left = new_left;
	block_offset = 0;
	__enable_irq();

	// find and subtract DC offset....
	// TODO: this may not be correct, needs testing with more types of signals
	dc = dc_average;
	p = out_left->data;
	end = p + AUDIO_BLOCK_SAMPLES;
	do {
		s = (uint16_t)(*p) - dc; // TODO: should be saturating subtract
		*p++ = s;
		dc += s >> 13; // approx 5.38 Hz high pass filter
	} while (p < end);
	dc_average = dc;

	// then transmit the AC data
	transmit(out_left);
	release(out_left);
}



