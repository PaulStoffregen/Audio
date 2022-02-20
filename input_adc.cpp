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
#endif  // KINETISK





#if defined(__IMXRT1062__) // Teensy 4.0, 4.1, MicroMod

#include <Arduino.h>
#include "input_adc.h"

extern "C" void xbar_connect(unsigned int input, unsigned int output);

DMAChannel AudioInputAnalog::dma(false);
// need at least FILTERLEN extra samples, but add a safety margin so the DMA won't
// overwrite the oldest samples if we have some latency before calling update()
static __attribute__((aligned(32))) uint16_t adc_buffer[AUDIO_BLOCK_SAMPLES*4+200];

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


// http://t-filter.engineerjs.com/  (use 176400 sample freq, int 18 bit output)
static const int16_t filter[] = {
#if 1
33, 125, 299, 591, 979, 1420, 1798, 1971, 1784, 1136, 26, -1391, -2811,
-3802, -3906, -2743, -142, 3778, 8586, 13593, 17981, 20983, 22050, 20983,
17981, 13593, 8586, 3778, -142, -2743, -3906, -3802, -2811, -1391, 26,
1136, 1784, 1971, 1798, 1420, 979, 591, 299, 125, 33
#else
-12, -40, -95, -179, -282, -382, -443, -424, -298, -65, 239, 537, 736,
755, 561, 192, -240, -577, -671, -445, 61, 686, 1189, 1337, 999, 226,
-739, -1528, -1776, -1276, -92, 1410, 2657, 3062, 2259, 299, -2280,
-4549, -5441, -4098, -190, 5901, 13108, 19919, 24783, 26547, 24783, 19919,
13108, 5901, -190, -4098, -5441, -4549, -2280, 299, 2259, 3062, 2657,
1410, -92, -1276, -1776, -1528, -739, 226, 999, 1337, 1189, 686, 61, -445,
-671, -577, -240, 192, 561, 755, 736, 537, 239, -65, -298, -424, -443,
-382, -282, -179, -95, -40, -12
#endif
};

#define FILTERLEN (sizeof(filter)/2)

static int16_t capture_buffer[AUDIO_BLOCK_SAMPLES*4+FILTERLEN];

void AudioInputAnalog::init(uint8_t pin)
{
	if (pin >= sizeof(adc2_pin_to_channel)) return;
	const uint8_t adc_channel = adc2_pin_to_channel[pin];
	if (adc_channel == 255) return;

	//analogReadResolution(12);
	//while (!Serial);
	//for (int i=0; i < 16; i++) Serial.println(analogRead(pin));

	// configure a timer to trigger ADC
	// sample rate should be very close to 4X AUDIO_SAMPLE_RATE_EXACT
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
	//  ADLPC=0, ADHSC=1, 12 bit mode, 40 MHz max ADC clock
	//  ADLPC=0, ADHSC=0, 12 bit mode, 30 MHz max ADC clock
	//  ADLPC=1, ADHSC=0, 12 bit mode, 20 MHz max ADC clock

	uint32_t cfg = ADC_CFG_ADTRG;
	cfg |= ADC_CFG_MODE(2);  // 2 = 12 bits
	cfg |= ADC_CFG_AVGS(0);  // number of samples to average
	cfg |= ADC_CFG_ADSTS(3); // sampling time, 0-3
	//cfg |= ADC_CFG_ADLSMP;   // long sample time
	cfg |= ADC_CFG_ADHSC;    // high speed conversion
	//cfg |= ADC_CFG_ADLPC;    // low power
	cfg |= ADC_CFG_ADICLK(0);// 0:ipg, 1=ipg/2, 3=adack (10 or 20 MHz)
	cfg |= ADC_CFG_ADIV(2);  // 0:div1, 1=div2, 2=div4, 3=div8
	ADC2_CFG = cfg;
	//ADC2_GC &= ~ADC_GC_AVGE; // single sample, no averaging
	ADC2_GC |= ADC_GC_AVGE; // use averaging
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

	// TODO: configure I2S1 to interrupt every 128 audio samples, run 1st half of update
}

static int16_t fir(const int16_t *data, const int16_t *impulse, int len)
{
	int64_t sum=0;

	while (len > 0) {
		sum += *data++ * *impulse++; // TODO: optimize with DSP inst and filter symmetry
		len --;
	}
	return signed_saturate_rshift(sum, 16, 13);
}

// simple stats for troubleshooting
//volatile int capture_min=65535;
//volatile int capture_max=0;
//volatile int ac_only_min=65535;
//volatile int ac_only_max=0;
//volatile int filter_min=65535;
//volatile int filter_max=0;
//volatile int samples_count=0;

int32_t AudioInputAnalog::hpf_y1 = 0;
int32_t AudioInputAnalog::hpf_x1 = 0;

void AudioInputAnalog::update(void)
{
	audio_block_t *output=NULL;
	output = allocate();
	if (output == NULL) return;

	const int adc_buffer_len = sizeof(adc_buffer)/2;
	static uint16_t *prior_p = adc_buffer;
	uint16_t *p = (uint16_t *)dma.TCD->DADDR;
	// TODO: check if DADDR points to most recently written (as used here)
	// or if DADDR really points to next place to write, and we would need to
	// back up 1 location to avoid reusing stale oldest data
	if (--p < adc_buffer) p = adc_buffer + adc_buffer_len - 1;

	// First, copy raw samples from adc_buffer[] to capture_buffer[].
	// Perhaps a future version could avoid this memory-to-memory copy and
	// the extra memory used by capture_buffer[] by redesigning the DC offset
	// removal and FIR filter to be able to work within the wrap-around
	// adc_buffer[].
	const int capture_buffer_len = sizeof(capture_buffer)/2;
	int new_samples, recycle_samples;
	if (p >= prior_p) {
		// new raw ADC samples are contiguous within adc_buffer[]
		new_samples = p - prior_p; // must be close to AUDIO_BLOCK_SAMPLES*4
		if (new_samples > capture_buffer_len) {
			new_samples = capture_buffer_len;
		}
		recycle_samples = capture_buffer_len - new_samples;
		if (recycle_samples > 0) {
			memmove(capture_buffer, capture_buffer + new_samples, recycle_samples * 2);
		}
		memcpy(capture_buffer + recycle_samples, prior_p, new_samples * 2);
		//Serial.printf("recycle = %d, num = %d\n", recycle_samples, new_samples);
	} else {
		// new raw ADC samples wrap around from end to start of adc_buffer[]
		int new_samples1 = (adc_buffer + adc_buffer_len) - prior_p;
		int new_samples2 = p - adc_buffer;
		new_samples = new_samples1 + new_samples2; // must be ~AUDIO_BLOCK_SAMPLES*4
		if (new_samples > capture_buffer_len) {
			if (new_samples1 >= capture_buffer_len) {
				new_samples1 = capture_buffer_len;
				new_samples2 = 0;
			} else {
				new_samples2 = capture_buffer_len - new_samples1;
			}
			new_samples = capture_buffer_len;
		}
		recycle_samples = capture_buffer_len - new_samples;
		if (recycle_samples > 0) {
			memmove(capture_buffer, capture_buffer + new_samples, recycle_samples * 2);
		}
		memcpy(capture_buffer + recycle_samples, prior_p, new_samples1 * 2);
		memcpy(capture_buffer + recycle_samples + new_samples1,
			adc_buffer, new_samples2 * 2);
		//Serial.printf("recycle = %d, num = %d (%d + %d)\n", recycle_samples,
			//new_samples, new_samples1, new_samples2);
	}
	//samples_count = new_samples;
	if (++p >= adc_buffer + adc_buffer_len) p = adc_buffer;
	prior_p = p;
	if (new_samples < AUDIO_BLOCK_SAMPLES*4 - 3) {
		// if the ADC didn't collect enough raw samples, give up
		// now rather than creating horribly choppy sounds.
		// Normally this shouldn't happen, but wrong ADC settings
		// can cause it to run too slow.
		release(output);
		//Serial.println("AudioInputAnalog, ADC running too slow");
		return;
	}

	// Remove DC offset from newly added samples
	int16_t *s = capture_buffer + recycle_samples;
	const int16_t *end = capture_buffer + capture_buffer_len;
	while (s < end) {
		//if (*s > capture_max) capture_max = *s;
		//if (*s < capture_min) capture_min = *s;
#if 0
		// just subtract a constant, for testing only!!
		int dc_offset = 1950;
		int n = (int)*s - dc_offset;
		if (n > 4095) n = 4095;
		if (n < -4095) n = -4095;
#endif
#if 0
		// https://forum.pjrc.com/threads/69542
		#define pole ((int16_t)32767*0.995)
		#define Q15Mul(a,b) ((int16_t)((int32_t)a*b)>>15)
		static int xm1=0, ym1=0;
		ym1 = *s - xm1 + Q15Mul(pole,ym1);
		xm1 = *s;
		int n = ym1;
#endif
#if 1
		#define COEF_HPF_DCBLOCK    (1048300<<10)
		int32_t tmp = *s;
		tmp = ( ((int32_t) tmp) << 14);
		int32_t acc = hpf_y1 - hpf_x1;
		acc += tmp;
		hpf_y1 = FRACMUL_SHL(acc, COEF_HPF_DCBLOCK, 1);
		hpf_x1 = tmp;
		int n = signed_saturate_rshift(hpf_y1, 16, 14);
#endif
		// TODO: try this? https://www.dsprelated.com/showarticle/58.php
		//if (n > ac_only_max) ac_only_max = n;
		//if (n < ac_only_min) ac_only_min = n;
		*s++ = n;
	}

	// Low pass filter and subsample
	int16_t *dest = output->data;
	for (int i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
#if 1
		// proper low-pass filter sounds pretty good
		*dest++ = fir(capture_buffer + i * 4, filter, sizeof(filter)/2);
#else
		// just averge 4 samples together, lower quality but less math
		*dest++ = (capture_buffer[i * 4] + capture_buffer[i * 4 + 1]
			+ capture_buffer[i * 4 + 2] + capture_buffer[i * 4 + 3]) << 2;
#endif
		//int x = output->data[i];
		//if (x > filter_max) filter_max = x;
		//if (x < filter_min) filter_min = x;
	}
	transmit(output);
	release(output);
}

#endif // __IMXRT1062__


#if defined(KINETISL)

// TODO: ADC implementation needed for Teensy LC

void AudioInputAnalog::init(uint8_t pin)
{
}

void AudioInputAnalog::update(void)
{
}

#endif // KINETISL
