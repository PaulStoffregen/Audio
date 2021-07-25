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
#include "input_adc.h"
#include "utility/dspinst.h"

#if defined(KINETISK)

#include "utility/pdb.h"

#define COEF_HPF_DCBLOCK    (1048300<<10)  // DC Removal filter coefficient in S1.30

DMAMEM __attribute__((aligned(32))) static uint16_t analog_rx_buffer[AUDIO_BLOCK_SAMPLES];
audio_block_t * AudioInputAnalog::block_left = NULL;
uint16_t AudioInputAnalog::block_offset = 0;
int32_t AudioInputAnalog::hpf_y1 = 0;
int32_t AudioInputAnalog::hpf_x1 = 0;

bool AudioInputAnalog::update_responsibility = false;
DMAChannel AudioInputAnalog::dma(false);

void AudioInputAnalog::init(uint8_t pin)
{
	int32_t tmp;

	// Configure the ADC and run at least one software-triggered
	// conversion.  This completes the self calibration stuff and
	// leaves the ADC in a state that's mostly ready to use
	analogReadRes(16);
	analogReference(INTERNAL); // range 0 to 1.2 volts
#if F_BUS == 96000000 || F_BUS == 48000000 || F_BUS == 24000000
	analogReadAveraging(8);
#else
	analogReadAveraging(4);
#endif
	// Note for review:
	// Probably not useful to spin cycles here stabilizing
	// since DC blocking is similar to te external analog filters
	tmp = (uint16_t) analogRead(pin);
	tmp = ( ((int32_t) tmp) << 14);
	hpf_x1 = tmp;   // With constant DC level x1 would be x0
	hpf_y1 = 0;     // Output will settle here when stable

	// set the programmable delay block to trigger the ADC at 44.1 kHz
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
	// enable the ADC for hardware trigger and DMA
	ADC0_SC2 |= ADC_SC2_ADTRG | ADC_SC2_DMAEN;

	// set up a DMA channel to store the ADC data
	dma.begin(true);
	dma.TCD->SADDR = &ADC0_RA;
	dma.TCD->SOFF = 0;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	dma.TCD->NBYTES_MLNO = 2;
	dma.TCD->SLAST = 0;
	dma.TCD->DADDR = analog_rx_buffer;
	dma.TCD->DOFF = 2;
	dma.TCD->CITER_ELINKNO = sizeof(analog_rx_buffer) / 2;
	dma.TCD->DLASTSGA = -sizeof(analog_rx_buffer);
	dma.TCD->BITER_ELINKNO = sizeof(analog_rx_buffer) / 2;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_ADC0);
	update_responsibility = update_setup();
	dma.enable();
	dma.attachInterrupt(isr);
}


void AudioInputAnalog::isr(void)
{
	uint32_t daddr, offset;
	const uint16_t *src, *end;
	uint16_t *dest_left;
	audio_block_t *left;

	daddr = (uint32_t)(dma.TCD->DADDR);
	dma.clearInterrupt();

	if (daddr < (uint32_t)analog_rx_buffer + sizeof(analog_rx_buffer) / 2) {
		// DMA is receiving to the first half of the buffer
		// need to remove data from the second half
		src = (uint16_t *)&analog_rx_buffer[AUDIO_BLOCK_SAMPLES/2];
		end = (uint16_t *)&analog_rx_buffer[AUDIO_BLOCK_SAMPLES];
		if (update_responsibility) AudioStream::update_all();
	} else {
		// DMA is receiving to the second half of the buffer
		// need to remove data from the first half
		src = (uint16_t *)&analog_rx_buffer[0];
		end = (uint16_t *)&analog_rx_buffer[AUDIO_BLOCK_SAMPLES/2];
	}
	left = block_left;
	if (left != NULL) {
		offset = block_offset;
		if (offset > AUDIO_BLOCK_SAMPLES/2) offset = AUDIO_BLOCK_SAMPLES/2;
		dest_left = (uint16_t *)&(left->data[offset]);
		block_offset = offset + AUDIO_BLOCK_SAMPLES/2;
		do {
			*dest_left++ = *src++;
		} while (src < end);
	}
}

void AudioInputAnalog::update(void)
{
	audio_block_t *new_left=NULL, *out_left=NULL;
	uint32_t offset;
	int32_t tmp;
	int16_t s, *p, *end;

	//Serial.println("update");

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

    //
	// DC Offset Removal Filter
    // 1-pole digital high-pass filter implementation
    //   y = a*(x[n] - x[n-1] + y[n-1])
    // The coefficient "a" is as follows:
    //  a = UNITY*e^(-2*pi*fc/fs)
    //  fc = 2 @ fs = 44100
    //
	p = out_left->data;
	end = p + AUDIO_BLOCK_SAMPLES;
	do {
		tmp = (uint16_t)(*p);
        tmp = ( ((int32_t) tmp) << 14);
        int32_t acc = hpf_y1 - hpf_x1;
        acc += tmp;
        hpf_y1 = FRACMUL_SHL(acc, COEF_HPF_DCBLOCK, 1);
        hpf_x1 = tmp;
		s = signed_saturate_rshift(hpf_y1, 16, 14);
		*p++ = s;
	} while (p < end);

	// then transmit the AC data
	transmit(out_left);
	release(out_left);
}
#endif



#if defined(__IMXRT1062__)

#include <Arduino.h>
#include "input_adc.h"

extern "C" void xbar_connect(unsigned int input, unsigned int output);

#define FILTERLEN 15

DMAChannel AudioInputAnalog::dma(false);
// TODO: how much extra space is needed to avoid wrap-around timing?  200 seems a safe guess
static __attribute__((aligned(32))) uint16_t adc_buffer[AUDIO_BLOCK_SAMPLES*4+200];
static int16_t capture_buffer[AUDIO_BLOCK_SAMPLES*4+FILTERLEN];
// TODO: these big buffers should be in DMAMEM, rather than consuming precious DTCM

PROGMEM static const uint8_t adc2_pin_to_channel[] = {
	7,      // 0/A0  AD_B1_02
	8,      // 1/A1  AD_B1_03
	12,     // 2/A2  AD_B1_07
	11,     // 3/A3  AD_B1_06
	6,      // 4/A4  AD_B1_01
	5,      // 5/A5  AD_B1_00
	15,     // 6/A6  AD_B1_10
	0,      // 7/A7  AD_B1_11
	13,     // 8/A8  AD_B1_08
	14,     // 9/A9  AD_B1_09
	255,	// 10/A10 AD_B0_12 - only on ADC1, 1 - can't use for audio
	255,	// 11/A11 AD_B0_13 - only on ADC1, 2 - can't use for audio
	3,      // 12/A12 AD_B1_14
	4,      // 13/A13 AD_B1_15
	7,      // 14/A0  AD_B1_02
	8,      // 15/A1  AD_B1_03
	12,     // 16/A2  AD_B1_07
	11,     // 17/A3  AD_B1_06
	6,      // 18/A4  AD_B1_01
	5,      // 19/A5  AD_B1_00
	15,     // 20/A6  AD_B1_10
	0,      // 21/A7  AD_B1_11
	13,     // 22/A8  AD_B1_08
	14,     // 23/A9  AD_B1_09
	255,    // 24/A10 AD_B0_12 - only on ADC1, 1 - can't use for audio
	255,    // 25/A11 AD_B0_13 - only on ADC1, 2 - can't use for audio
	3,      // 26/A12 AD_B1_14 - only on ADC2, do not use analogRead()
	4,      // 27/A13 AD_B1_15 - only on ADC2, do not use analogRead()
#ifdef ARDUINO_TEENSY41
	255,    // 28
	255,    // 29
	255,    // 30
	255,    // 31
	255,    // 32
	255,    // 33
	255,    // 34
	255,    // 35
	255,    // 36
	255,    // 37
	1,      // 38/A14 AD_B1_12 - only on ADC2, do not use analogRead()
	2,      // 39/A15 AD_B1_13 - only on ADC2, do not use analogRead()
	9,      // 40/A16 AD_B1_04
	10,     // 41/A17 AD_B1_05
#endif
};

static const int16_t filter[FILTERLEN] = {
	1449,
	3676,
	6137,
	9966,
	13387,
	16896,
	18951,
	19957,
	18951,
	16896,
	13387,
	9966,
	6137,
	3676,
	1449
};


void AudioInputAnalog::init(uint8_t pin)
{
	if (pin >= sizeof(adc2_pin_to_channel)) return;
	const uint8_t adc_channel = adc2_pin_to_channel[pin];
	if (adc_channel == 255) return;

	// configure a timer to trigger ADC
	// TODO: sample rate should be slightly lower than 4X AUDIO_SAMPLE_RATE_EXACT
	//       linear interpolation is supposed to resample it to exactly 4X
	//       the sample rate, so we avoid artifacts boundaries between captures
	const int comp1 = ((float)F_BUS_ACTUAL) / (AUDIO_SAMPLE_RATE_EXACT * 4.0f) / 2.0f + 0.5f;
	TMR4_ENBL &= ~(1<<3);
	TMR4_SCTRL3 = TMR_SCTRL_OEN | TMR_SCTRL_FORCE;
	TMR4_CSCTRL3 = TMR_CSCTRL_CL1(1) | TMR_CSCTRL_TCF1EN;
	TMR4_CNTR3 = 0;
	TMR4_LOAD3 = 0;
	TMR4_COMP13 = comp1;
	TMR4_CMPLD13 = comp1;
	TMR4_CTRL3 = TMR_CTRL_CM(1) | TMR_CTRL_PCS(8) | TMR_CTRL_LENGTH | TMR_CTRL_OUTMODE(3);
	TMR4_DMA3 = TMR_DMA_CMPLD1DE;
	TMR4_CNTR3 = 0;
	TMR4_ENBL |= (1<<3);

	// connect the timer output the ADC_ETC input
	const int trigger = 4; // 0-3 for ADC1, 4-7 for ADC2
	CCM_CCGR2 |= CCM_CCGR2_XBAR1(CCM_CCGR_ON);
	xbar_connect(XBARA1_IN_QTIMER4_TIMER3, XBARA1_OUT_ADC_ETC_TRIG00 + trigger);

	// turn on ADC_ETC and configure to receive trigger
	if (ADC_ETC_CTRL & (ADC_ETC_CTRL_SOFTRST | ADC_ETC_CTRL_TSC_BYPASS)) {
		ADC_ETC_CTRL = 0; // clears SOFTRST only
		ADC_ETC_CTRL = 0; // clears TSC_BYPASS
	}
	ADC_ETC_CTRL |= ADC_ETC_CTRL_TRIG_ENABLE(1 << trigger) | ADC_ETC_CTRL_DMA_MODE_SEL;
	ADC_ETC_DMA_CTRL |= ADC_ETC_DMA_CTRL_TRIQ_ENABLE(trigger);

	// configure ADC_ETC trigger4 to make one ADC2 measurement on pin A2
	const int len = 1;
	IMXRT_ADC_ETC.TRIG[trigger].CTRL = ADC_ETC_TRIG_CTRL_TRIG_CHAIN(len - 1) |
		ADC_ETC_TRIG_CTRL_TRIG_PRIORITY(7);
	IMXRT_ADC_ETC.TRIG[trigger].CHAIN_1_0 = ADC_ETC_TRIG_CHAIN_HWTS0(1) |
		ADC_ETC_TRIG_CHAIN_CSEL0(adc2_pin_to_channel[pin]) | ADC_ETC_TRIG_CHAIN_B2B0;

	// set up ADC2 for 12 bit mode, hardware trigger
	Serial.printf("ADC2_CFG = %08X\n", ADC2_CFG);
	ADC2_CFG |= ADC_CFG_ADTRG;
	ADC2_CFG = ADC_CFG_MODE(2) | ADC_CFG_ADSTS(3) | ADC_CFG_ADLSMP | ADC_CFG_ADTRG |
		ADC_CFG_ADICLK(1) | ADC_CFG_ADIV(0) /*| ADC_CFG_ADHSC*/;
	ADC2_GC &= ~ADC_GC_AVGE; // single sample, no averaging
	ADC2_HC0 = ADC_HC_ADCH(16); // 16 = controlled by ADC_ETC

	// use a DMA channel to capture ADC_ETC output
	dma.begin();
	dma.TCD->SADDR = &(IMXRT_ADC_ETC.TRIG[4].RESULT_1_0);
	dma.TCD->SOFF = 0;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	dma.TCD->NBYTES_MLNO = 2;
	dma.TCD->SLAST = 0;
	dma.TCD->DADDR = adc_buffer;
	dma.TCD->DOFF = 2;
	dma.TCD->CITER_ELINKNO = sizeof(adc_buffer) / 2;
	dma.TCD->DLASTSGA = -sizeof(adc_buffer);
	dma.TCD->BITER_ELINKNO = sizeof(adc_buffer) / 2;
	dma.TCD->CSR = 0;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_ADC_ETC);
	dma.enable();

	// TODO: configure I2S1 to interrupt every 128 audio samples
}

static int16_t fir(const int16_t *data, const int16_t *impulse, int len)
{
	int64_t sum=0;

	while (len > 0) {
		sum += *data++ * *impulse++; // TODO: optimize with DSP inst and filter symmetry
		len --;
	}
	sum = sum >> 15; // TODO: adjust filter coefficients for proper gain, 12 to 16 bits
	if (sum > 32767) return 32767;
	if (sum < -32768) return -32768;
	return sum;
}

void AudioInputAnalog::update(void)
{
	audio_block_t *output=NULL;
	output = allocate();
	if (output == NULL) return;

	uint16_t *p = (uint16_t *)dma.TCD->DADDR;
	//int offset = p - adc_buffer;
	//if (--offset < 0) offset = sizeof(adc_buffer) / 2 - 1;
	//Serial.printf("offset = %4d, val = %4d\n", offset + 1, adc_buffer[offset]);

	// copy adc buffer to capture buffer
	//  FIXME: this should be done from the I2S interrupt, for precise capture timing
	const unsigned int capture_len = sizeof(capture_buffer) / 2;
	for (unsigned int i=0; i < capture_len; i++) {
		// TODO: linear interpolate to exactly 4X sample rate
		if (--p < adc_buffer) p = adc_buffer + (sizeof(adc_buffer) / 2 - 1);

		// remove DC offset
		// TODO: very slow low pass filter for DC offset
		int dc_offset = 550; // FIXME: quick kludge for testing!!

		int n = (int)*p - dc_offset;
		if (n > 4095) n = 4095;
		if (n < -4095) n = -4095;

		capture_buffer[i] = n;
	}
	//printbuf(capture_buffer, 8);

	// low pass filter and subsample (this part belongs here)
	int16_t *dest = output->data + AUDIO_BLOCK_SAMPLES - 1;
	for (int i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
#if 1
		// proper low-pass filter sounds pretty good
		*dest-- = fir(capture_buffer + i * 4, filter, sizeof(filter)/2);
#else
		// just averge 4 samples together, lower quality but much faster
		*dest-- = capture_buffer[i * 4] + capture_buffer[i * 4 + 1]
			+ capture_buffer[i * 4 + 2] + capture_buffer[i * 4 + 3];
#endif
	}
	transmit(output);
	release(output);
}



#endif
