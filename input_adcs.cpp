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
#include "input_adcs.h"
#include "utility/pdb.h"
#include "utility/dspinst.h"

#if defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)

#define COEF_HPF_DCBLOCK    (1048300<<10)  // DC Removal filter coefficient in S1.30

DMAMEM static uint16_t left_buffer[AUDIO_BLOCK_SAMPLES];
DMAMEM static uint16_t right_buffer[AUDIO_BLOCK_SAMPLES];
audio_block_t * AudioInputAnalogStereo::block_left = NULL;
audio_block_t * AudioInputAnalogStereo::block_right = NULL;
uint16_t AudioInputAnalogStereo::offset_left = 0;
uint16_t AudioInputAnalogStereo::offset_right = 0;
int32_t AudioInputAnalogStereo::hpf_y1[2] = { 0, 0 };
int32_t AudioInputAnalogStereo::hpf_x1[2] = { 0, 0 };
bool AudioInputAnalogStereo::update_responsibility = false;
DMAChannel AudioInputAnalogStereo::dma0(false);
DMAChannel AudioInputAnalogStereo::dma1(false);

static int analogReadADC1(uint8_t pin);

void AudioInputAnalogStereo::init(uint8_t pin0, uint8_t pin1)
{
	uint32_t tmp;

	//pinMode(32, OUTPUT);
	//pinMode(33, OUTPUT);

	// Configure the ADC and run at least one software-triggered
	// conversion.  This completes the self calibration stuff and
	// leaves the ADC in a state that's mostly ready to use
	analogReadRes(16);
	analogReference(INTERNAL); // range 0 to 1.2 volts
#if F_BUS == 96000000 || F_BUS == 48000000 || F_BUS == 24000000
	analogReadAveraging(8);
	ADC1_SC3 = ADC_SC3_AVGE + ADC_SC3_AVGS(1);
#else
	analogReadAveraging(4);
	ADC1_SC3 = ADC_SC3_AVGE + ADC_SC3_AVGS(0);
#endif

    // Note for review:
    // Probably not useful to spin cycles here stabilizing
    // since DC blocking is similar to te external analog filters
    tmp = (uint16_t) analogRead(pin0);
    tmp = ( ((int32_t) tmp) << 14);
    hpf_x1[0] = tmp;   // With constant DC level x1 would be x0
    hpf_y1[0] = 0;     // Output will settle here when stable

    tmp = (uint16_t) analogReadADC1(pin1);
    tmp = ( ((int32_t) tmp) << 14);
    hpf_x1[1] = tmp;   // With constant DC level x1 would be x0
    hpf_y1[1] = 0;     // Output will settle here when stable


	// set the programmable delay block to trigger the ADC at 44.1 kHz
	//if (!(SIM_SCGC6 & SIM_SCGC6_PDB)
	  //|| (PDB0_SC & PDB_CONFIG) != PDB_CONFIG
	  //|| PDB0_MOD != PDB_PERIOD
	  //|| PDB0_IDLY != 1
	  //|| PDB0_CH0C1 != 0x0101) {
		SIM_SCGC6 |= SIM_SCGC6_PDB;
		PDB0_IDLY = 1;
		PDB0_MOD = PDB_PERIOD;
		PDB0_SC = PDB_CONFIG | PDB_SC_LDOK;
		PDB0_SC = PDB_CONFIG | PDB_SC_SWTRIG;
		PDB0_CH0C1 = 0x0101;
		PDB0_CH1C1 = 0x0101;
	//}

	// enable the ADC for hardware trigger and DMA
	ADC0_SC2 |= ADC_SC2_ADTRG | ADC_SC2_DMAEN;
	ADC1_SC2 |= ADC_SC2_ADTRG | ADC_SC2_DMAEN;

	// set up a DMA channel to store the ADC data
	dma0.begin(true);
	dma1.begin(true);
	// ADC0_RA = 0x4003B010
	// ADC1_RA = 0x400BB010
	dma0.TCD->SADDR = &ADC0_RA;
	dma0.TCD->SOFF = 0;
	dma0.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	dma0.TCD->NBYTES_MLNO = 2;
	dma0.TCD->SLAST = 0;
	dma0.TCD->DADDR = left_buffer;
	dma0.TCD->DOFF = 2;
	dma0.TCD->CITER_ELINKNO = sizeof(left_buffer) / 2;
	dma0.TCD->DLASTSGA = -sizeof(left_buffer);
	dma0.TCD->BITER_ELINKNO = sizeof(left_buffer) / 2;
	dma0.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;

	dma1.TCD->SADDR = &ADC1_RA;
	dma1.TCD->SOFF = 0;
	dma1.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	dma1.TCD->NBYTES_MLNO = 2;
	dma1.TCD->SLAST = 0;
	dma1.TCD->DADDR = right_buffer;
	dma1.TCD->DOFF = 2;
	dma1.TCD->CITER_ELINKNO = sizeof(right_buffer) / 2;
	dma1.TCD->DLASTSGA = -sizeof(right_buffer);
	dma1.TCD->BITER_ELINKNO = sizeof(right_buffer) / 2;
	dma1.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;

	dma0.triggerAtHardwareEvent(DMAMUX_SOURCE_ADC0);
	//dma1.triggerAtHardwareEvent(DMAMUX_SOURCE_ADC1);
	dma1.triggerAtTransfersOf(dma0);
	dma1.triggerAtCompletionOf(dma0);
	update_responsibility = update_setup();
	dma0.enable();
	dma1.enable();
	dma0.attachInterrupt(isr0);
	dma1.attachInterrupt(isr1);
}


void AudioInputAnalogStereo::isr0(void)
{
	uint32_t daddr, offset;
	const uint16_t *src, *end;
	uint16_t *dest;

	daddr = (uint32_t)(dma0.TCD->DADDR);
	dma0.clearInterrupt();

	//digitalWriteFast(32, HIGH);
	if (daddr < (uint32_t)left_buffer + sizeof(left_buffer) / 2) {
		// DMA is receiving to the first half of the buffer
		// need to remove data from the second half
		src = (uint16_t *)&left_buffer[AUDIO_BLOCK_SAMPLES/2];
		end = (uint16_t *)&left_buffer[AUDIO_BLOCK_SAMPLES];
	} else {
		// DMA is receiving to the second half of the buffer
		// need to remove data from the first half
		src = (uint16_t *)&left_buffer[0];
		end = (uint16_t *)&left_buffer[AUDIO_BLOCK_SAMPLES/2];
		//if (update_responsibility) AudioStream::update_all();
	}
	if (block_left != NULL) {
		offset = offset_left;
		if (offset > AUDIO_BLOCK_SAMPLES/2) offset = AUDIO_BLOCK_SAMPLES/2;
		offset_left = offset + AUDIO_BLOCK_SAMPLES/2;
		dest = (uint16_t *)&(block_left->data[offset]);
		do {
			*dest++ = *src++;
		} while (src < end);
	}
	//digitalWriteFast(32, LOW);
}

void AudioInputAnalogStereo::isr1(void)
{
	uint32_t daddr, offset;
	const uint16_t *src, *end;
	uint16_t *dest;

	daddr = (uint32_t)(dma1.TCD->DADDR);
	dma1.clearInterrupt();

	//digitalWriteFast(33, HIGH);
	if (daddr < (uint32_t)right_buffer + sizeof(right_buffer) / 2) {
		// DMA is receiving to the first half of the buffer
		// need to remove data from the second half
		src = (uint16_t *)&right_buffer[AUDIO_BLOCK_SAMPLES/2];
		end = (uint16_t *)&right_buffer[AUDIO_BLOCK_SAMPLES];
		if (update_responsibility) AudioStream::update_all();
	} else {
		// DMA is receiving to the second half of the buffer
		// need to remove data from the first half
		src = (uint16_t *)&right_buffer[0];
		end = (uint16_t *)&right_buffer[AUDIO_BLOCK_SAMPLES/2];
	}
	if (block_right != NULL) {
		offset = offset_right;
		if (offset > AUDIO_BLOCK_SAMPLES/2) offset = AUDIO_BLOCK_SAMPLES/2;
		offset_right = offset + AUDIO_BLOCK_SAMPLES/2;
		dest = (uint16_t *)&(block_right->data[offset]);
		do {
			*dest++ = *src++;
		} while (src < end);
	}
	//digitalWriteFast(33, LOW);
}


void AudioInputAnalogStereo::update(void)
{
	audio_block_t *new_left=NULL, *out_left=NULL;
	audio_block_t *new_right=NULL, *out_right=NULL;
	int32_t tmp;
	int16_t s, *p, *end;

	//Serial.println("update");

	// allocate new block (ok if both NULL)
	new_left = allocate();
	if (new_left == NULL) {
		new_right = NULL;
	} else {
		new_right = allocate();
		if (new_right == NULL) {
			release(new_left);
			new_left = NULL;
		}
	}
	__disable_irq();
	if (offset_left < AUDIO_BLOCK_SAMPLES || offset_right < AUDIO_BLOCK_SAMPLES) {
		// the DMA hasn't filled up both blocks
		if (block_left == NULL) {
			block_left = new_left;
			offset_left = 0;
			new_left = NULL;
		}
		if (block_right == NULL) {
			block_right = new_right;
			offset_right = 0;
			new_right = NULL;
		}
		__enable_irq();
		if (new_left) release(new_left);
		if (new_right) release(new_right);
		return;
	}
	// the DMA filled blocks, so grab them and get the
	// new blocks to the DMA, as quickly as possible
	out_left = block_left;
	out_right = block_right;
	block_left = new_left;
	block_right = new_right;
	offset_left = 0;
	offset_right = 0;
	__enable_irq();

    //
	// DC Offset Removal Filter
    // 1-pole digital high-pass filter implementation
    //   y = a*(x[n] - x[n-1] + y[n-1])
    // The coefficient "a" is as follows:
    //  a = UNITY*e^(-2*pi*fc/fs)
    //  fc = 2 @ fs = 44100
    //

    // DC removal, LEFT
    p = out_left->data;
    end = p + AUDIO_BLOCK_SAMPLES;
    do {
        tmp = (uint16_t)(*p);
        tmp = ( ((int32_t) tmp) << 14);
        int32_t acc = hpf_y1[0] - hpf_x1[0];
        acc += tmp;
        hpf_y1[0] = FRACMUL_SHL(acc, COEF_HPF_DCBLOCK, 1);
        hpf_x1[0] = tmp;
        s = signed_saturate_rshift(hpf_y1[0], 16, 14);
        *p++ = s;
    } while (p < end);

    // DC removal, RIGHT
    p = out_right->data;
    end = p + AUDIO_BLOCK_SAMPLES;
    do {
        tmp = (uint16_t)(*p);
        tmp = ( ((int32_t) tmp) << 14);
        int32_t acc = hpf_y1[1] - hpf_x1[1];
        acc += tmp;
        hpf_y1[1]= FRACMUL_SHL(acc, COEF_HPF_DCBLOCK, 1);
        hpf_x1[1] = tmp;
        s = signed_saturate_rshift(hpf_y1[1], 16, 14);
        *p++ = s;
    } while (p < end);

	// then transmit the AC data
	transmit(out_left, 0);
	release(out_left);
	transmit(out_right, 1);
	release(out_right);
}


#if defined(__MK20DX256__)
static const uint8_t pin2sc1a[] = {
        5, 14, 8+128, 9+128, 13, 12, 6, 7, 15, 4, 0, 19, 3, 19+128, // 0-13 -> A0-A13
        5, 14, 8+128, 9+128, 13, 12, 6, 7, 15, 4, // 14-23 are A0-A9
        255, 255, // 24-25 are digital only
        5+192, 5+128, 4+128, 6+128, 7+128, 4+192, // 26-31 are A15-A20
        255, 255, // 32-33 are digital only
        0, 19, 3, 19+128, // 34-37 are A10-A13
        26,     // 38 is temp sensor,
        18+128, // 39 is vref
        23      // 40 is A14
};
#elif defined(__MK64FX512__) || defined(__MK66FX1M0__)
static const uint8_t pin2sc1a[] = {
        5, 14, 8+128, 9+128, 13, 12, 6, 7, 15, 4, 3, 19+128, 14+128, 15+128, // 0-13 -> A0-A13
        5, 14, 8+128, 9+128, 13, 12, 6, 7, 15, 4, // 14-23 are A0-A9
        255, 255, 255, 255, 255, 255, 255, // 24-30 are digital only
        14+128, 15+128, 17, 18, 4+128, 5+128, 6+128, 7+128, 17+128,  // 31-39 are A12-A20
        255, 255, 255, 255, 255, 255, 255, 255, 255,  // 40-48 are digital only
        10+128, 11+128, // 49-50 are A23-A24
        255, 255, 255, 255, 255, 255, 255, // 51-57 are digital only
        255, 255, 255, 255, 255, 255, // 58-63 (sd card pins) are digital only
        3, 19+128, // 64-65 are A10-A11
        23, 23+128,// 66-67 are A21-A22 (DAC pins)
        1, 1+128,  // 68-69 are A25-A26 (unused USB host port on Teensy 3.5)
        26,        // 70 is Temperature Sensor
        18+128     // 71 is Vref
};
#endif


static int analogReadADC1(uint8_t pin)
{
        ADC1_SC1A = 9;
        while (1) {
                if ((ADC1_SC1A & ADC_SC1_COCO)) {
                        return ADC1_RA;
                }
        }

        if (pin >= sizeof(pin2sc1a)) return 0;
        uint8_t channel = pin2sc1a[pin];
	if ((channel & 0x80) == 0) return 0;
        if (channel == 255) return 0;
        if (channel & 0x40) {
                ADC1_CFG2 &= ~ADC_CFG2_MUXSEL;
        } else {
                ADC1_CFG2 |= ADC_CFG2_MUXSEL;
        }
        ADC1_SC1A = channel & 0x3F;
        while (1) {
                if ((ADC1_SC1A & ADC_SC1_COCO)) {
                        return ADC1_RA;
                }
        }
}

#else

void AudioInputAnalogStereo::init(uint8_t pin0, uint8_t pin1)
{
}

void AudioInputAnalogStereo::update(void)
{
}


#endif
