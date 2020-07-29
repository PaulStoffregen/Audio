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
#include "output_pwm.h"

bool AudioOutputPWM::update_responsibility = false;

#if defined(KINETISK)
audio_block_t * AudioOutputPWM::block_1st = NULL;
audio_block_t * AudioOutputPWM::block_2nd = NULL;
uint32_t  AudioOutputPWM::block_offset = 0;
uint8_t AudioOutputPWM::interrupt_count = 0;

DMAMEM uint32_t pwm_dma_buffer[AUDIO_BLOCK_SAMPLES*2];
DMAChannel AudioOutputPWM::dma(false);

// TODO: this code assumes F_BUS is 48 MHz.
// supporting other speeds is not easy, but should be done someday


void AudioOutputPWM::begin(void)
{
	dma.begin(true); // Allocate the DMA channel first

	//Serial.println("AudioPwmOutput constructor");
	block_1st = NULL;
	FTM1_SC = 0;
	FTM1_CNT = 0;
	FTM1_MOD = 543;
	FTM1_C0SC = 0x69;  // send DMA request on match
	FTM1_C1SC = 0x28;
	FTM1_SC = FTM_SC_CLKS(1) | FTM_SC_PS(0);
	CORE_PIN3_CONFIG = PORT_PCR_MUX(3) | PORT_PCR_DSE | PORT_PCR_SRE;
	CORE_PIN4_CONFIG = PORT_PCR_MUX(3) | PORT_PCR_DSE | PORT_PCR_SRE;
	FTM1_C0V = 120; // range 120 to 375
	FTM1_C1V = 0;   // range 0 to 255
	for (int i=0; i<(AUDIO_BLOCK_SAMPLES*2); i+=2) {
		pwm_dma_buffer[i] = 120; // zero must not be used
		pwm_dma_buffer[i+1] = 0;
	}
	dma.TCD->SADDR = pwm_dma_buffer;
	dma.TCD->SOFF = 4;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2)
		| DMA_TCD_ATTR_DSIZE(2) | DMA_TCD_ATTR_DMOD(4);
	dma.TCD->NBYTES_MLNO = 8;
	dma.TCD->SLAST = -sizeof(pwm_dma_buffer);
	dma.TCD->DADDR = &FTM1_C0V;
	dma.TCD->DOFF = 8;
	dma.TCD->CITER_ELINKNO = sizeof(pwm_dma_buffer) / 8;
	dma.TCD->DLASTSGA = 0;
	dma.TCD->BITER_ELINKNO = sizeof(pwm_dma_buffer) / 8;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM1_CH0);
	dma.enable();
	update_responsibility = update_setup();
	dma.attachInterrupt(isr);
}

void AudioOutputPWM::update(void)
{
	audio_block_t *block;
	block = receiveReadOnly();
	if (!block) return;
	__disable_irq();
	if (block_1st == NULL) {
		block_1st = block;
		block_offset = 0;
		__enable_irq();
	} else if (block_2nd == NULL) {
		block_2nd = block;
		__enable_irq();
	} else {
		audio_block_t *tmp = block_1st;
		block_1st = block_2nd;
		block_2nd = block;
		block_offset = 0;
		__enable_irq();
		release(tmp);
	}
}

void AudioOutputPWM::isr(void)
{
	int16_t *src;
	uint32_t *dest;
	audio_block_t *block;
	uint32_t saddr, offset;

	saddr = (uint32_t)(dma.TCD->SADDR);
	dma.clearInterrupt();
	if (saddr < (uint32_t)pwm_dma_buffer + sizeof(pwm_dma_buffer) / 2) {
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		dest = &pwm_dma_buffer[AUDIO_BLOCK_SAMPLES];
	} else {
		// DMA is transmitting the second half of the buffer
		// so we must fill the first half
		dest = pwm_dma_buffer;
	}
	block = AudioOutputPWM::block_1st;
	offset = AudioOutputPWM::block_offset;

	if (block) {
		src = &block->data[offset];
		for (int i=0; i < AUDIO_BLOCK_SAMPLES/4; i++) {
			uint16_t sample = *src++ + 0x8000;
			uint32_t msb = ((sample >> 8) & 255) + 120;
			uint32_t lsb = sample & 255;
			*dest++ = msb;
			*dest++ = lsb;
			*dest++ = msb;
			*dest++ = lsb;
		}
		offset += AUDIO_BLOCK_SAMPLES/4;
		if (offset < AUDIO_BLOCK_SAMPLES) {
			AudioOutputPWM::block_offset = offset;
		} else {
			AudioOutputPWM::block_offset = 0;
			AudioStream::release(block);
			AudioOutputPWM::block_1st = AudioOutputPWM::block_2nd;
			AudioOutputPWM::block_2nd = NULL;
		}
	} else {
		// fill with silence when no data available
		for (int i=0; i < AUDIO_BLOCK_SAMPLES/4; i++) {
			*dest++ = 248;
			*dest++ = 0;
			*dest++ = 248;
			*dest++ = 0;
		}
	}
	if (AudioOutputPWM::update_responsibility) {
		if (++AudioOutputPWM::interrupt_count >= 4) {
			AudioOutputPWM::interrupt_count = 0;
			AudioStream::update_all();
		}
	}
}




// DMA target is: (registers require 32 bit writes)
//  40039010 Channel 0 Value (FTM1_C0V)
//  40039018 Channel 1 Value (FTM1_C1V)

// TCD:
//  source address = buffer address
//  source offset = 4 bytes
//  attr = no src mod, ssize = 32 bit, dest mod = 16 bytes (4), dsize = 32 bit
//  minor loop byte count = 8
//  source last adjust = -sizeof(buffer)
//  dest address = FTM1_C0V
//  dest address offset = 8
//  citer = sizeof(buffer) / 8    (no minor loop linking)
//  dest last adjust = 0          (dest modulo keeps it ready for more)
//  control:
//    throttling = 0
//    major link to same channel
//    done = 0
//    active = 0
//    majorlink = 1
//    scatter/gather = 0
//    disable request = 0
//    inthalf = 1
//    intmajor = 1
//    start = 0
//  biter = sizeof(buffer) / 8   (no minor loop linking)


#elif defined(KINETISL)

void AudioOutputPWM::update(void)
{
	audio_block_t *block;
	block = receiveReadOnly();
	if (block) release(block);
}

#elif defined(__IMXRT1062__)


#if 1

// Frank says this should be disabled for non-beta release
// https://forum.pjrc.com/threads/60532-Teensy-4-1-Beta-Test?p=239244&viewfull=1#post239244

void AudioOutputPWM::begin(void)
{
}

void AudioOutputPWM::update(void)
{
	audio_block_t *block;
	block = receiveReadOnly();
	if (block) release(block);
}

#else
/*
* by Frank B
*/

static const uint8_t silence[2] = {0x80, 0x00};

extern uint8_t analog_write_res;
extern const struct _pwm_pin_info_struct pwm_pin_info[];
audio_block_t * AudioOutputPWM::block = NULL;
DMAMEM __attribute__((aligned(32))) static uint16_t pwm_tx_buffer[2][AUDIO_BLOCK_SAMPLES * 2];
DMAChannel AudioOutputPWM::dma[2];
_audio_info_flexpwm AudioOutputPWM::apins[2];

FLASHMEM
 void AudioOutputPWM::begin(void) { begin(3, 4); }
FLASHMEM
void AudioOutputPWM::begin(uint8_t pin1, uint8_t pin2)
{
  analogWriteResolution(8);
  const uint8_t pins[2] = {pin1, pin2};

  for (unsigned i = 0; i < 2; i++) {

    // use the existing code here:
    analogWriteFrequency(pins[i], AUDIO_SAMPLE_RATE_EXACT);
    analogWrite(pins[i], silence[i]);

    //Fill structure
    apins[i].pin = pins[i];
    apins[i].info = pwm_pin_info[apins[i].pin];

    uint8_t dmamux_source;

    if (apins[i].info.type == 1) { //only for valid flexPWM pin:
      unsigned module = (apins[i].info.module >> 4) & 3;
      unsigned submodule = apins[i].info.module & 3;
      switch (module) {
        case 0: {
            apins[i].flexpwm = &IMXRT_FLEXPWM1;
            switch (submodule) {
              case 0: dmamux_source = DMAMUX_SOURCE_FLEXPWM1_WRITE0; break;
              case 1: dmamux_source = DMAMUX_SOURCE_FLEXPWM1_WRITE1; break;
              case 2: dmamux_source = DMAMUX_SOURCE_FLEXPWM1_WRITE2; break;
              default: dmamux_source = DMAMUX_SOURCE_FLEXPWM1_WRITE3;
            }
            break;
          }
        case 1: {
            apins[i].flexpwm = &IMXRT_FLEXPWM2;
            switch (submodule) {
              case 0: dmamux_source = DMAMUX_SOURCE_FLEXPWM2_WRITE0; break;
              case 1: dmamux_source = DMAMUX_SOURCE_FLEXPWM2_WRITE1; break;
              case 2: dmamux_source = DMAMUX_SOURCE_FLEXPWM2_WRITE2; break;
              default: dmamux_source = DMAMUX_SOURCE_FLEXPWM2_WRITE3;
            }
            break;
          }
        case 2: {
            apins[i].flexpwm = &IMXRT_FLEXPWM3;
            switch (submodule) {
              case 0: dmamux_source = DMAMUX_SOURCE_FLEXPWM3_WRITE0; break;
              case 1: dmamux_source = DMAMUX_SOURCE_FLEXPWM3_WRITE1; break;
              case 2: dmamux_source = DMAMUX_SOURCE_FLEXPWM3_WRITE2; break;
              default: dmamux_source = DMAMUX_SOURCE_FLEXPWM3_WRITE3;
            }
            break;
          }
        default: {
            apins[i].flexpwm = &IMXRT_FLEXPWM4;
            switch (submodule) {
              case 0: dmamux_source = DMAMUX_SOURCE_FLEXPWM4_WRITE0; break;
              case 1: dmamux_source = DMAMUX_SOURCE_FLEXPWM4_WRITE1; break;
              case 2: dmamux_source = DMAMUX_SOURCE_FLEXPWM4_WRITE2; break;
              default: dmamux_source = DMAMUX_SOURCE_FLEXPWM4_WRITE3;
            }
          }
        }

	volatile uint16_t *valReg;
	switch (apins[i].info.channel) {
	    case 0:  valReg = &apins[i].flexpwm->SM[submodule].VAL0; break;
	    case 1:  valReg = &apins[i].flexpwm->SM[submodule].VAL3; break;
	    default:  valReg = &apins[i].flexpwm->SM[submodule].VAL5; break;
	}

	dma[i].begin(true);
	dma[i].TCD->SADDR = &pwm_tx_buffer[i][0];
	dma[i].TCD->SOFF = 2;
	dma[i].TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	dma[i].TCD->NBYTES_MLNO = 2;
	dma[i].TCD->SLAST = -sizeof(pwm_tx_buffer[0]);
	dma[i].TCD->DOFF = 0;
	dma[i].TCD->CITER_ELINKNO = sizeof(pwm_tx_buffer[0]) / 2;
	dma[i].TCD->DLASTSGA = 0;
	dma[i].TCD->BITER_ELINKNO = sizeof(pwm_tx_buffer[0]) / 2;
	dma[i].TCD->DADDR = valReg;
	dma[i].triggerAtHardwareEvent(dmamux_source);
	if (i == 1) { //One interrupt only
		dma[i].TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
		dma[i].attachInterrupt(isr);
	}

	//set PWM-DMA-Enable
	apins[i].flexpwm->SM[submodule].DMAEN = FLEXPWM_SMDMAEN_VALDE;

	//clear inital dma data:
	uint32_t modulo = apins[i].flexpwm->SM[apins[i].info.module & 3].VAL1;
	for (unsigned j=0; j<AUDIO_BLOCK_SAMPLES * 2; j++) {
		uint32_t cval = (silence[i] * (modulo + 1)) >> analog_write_res;
		if (cval > modulo) cval = modulo;
		pwm_tx_buffer[i][j] = cval;
	}
	arm_dcache_flush_delete(&pwm_tx_buffer[i][0], sizeof(pwm_tx_buffer[0]) / 2 );
      }
  }

  dma[0].enable();
  dma[1].enable();
  update_responsibility = update_setup();
  //pinMode(13,OUTPUT);
}

void AudioOutputPWM::isr(void)
{
	dma[1].clearInterrupt();

	uint16_t *dest, *dest1;

	uint32_t saddr = (uint32_t)(dma[0].TCD->SADDR);
	if (saddr < (uint32_t)&pwm_tx_buffer[0][AUDIO_BLOCK_SAMPLES]) {
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		dest = &pwm_tx_buffer[0][AUDIO_BLOCK_SAMPLES];
		dest1 = &pwm_tx_buffer[1][AUDIO_BLOCK_SAMPLES];
	} else {
		// DMA is transmitting the second half of the buffer
		// so we must fill the first half
		dest = &pwm_tx_buffer[0][0];
		dest1 = &pwm_tx_buffer[1][0];
	}

        const uint32_t modulo[2] = { apins[0].flexpwm->SM[apins[0].info.module & 3].VAL1, apins[1].flexpwm->SM[apins[1].info.module & 3].VAL1};

	if (block) {

		for (unsigned i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			uint32_t sample = (uint16_t)block->data[i] + 0x8000;
			
			uint32_t msb = ((sample >> 8) & 255)/* + 120 ???*/;
			uint32_t cval0 = (msb * (modulo[0] + 1)) >> analog_write_res;
			if (cval0 > modulo[0]) cval0 = modulo[0]; // TODO: is this check correct?
			*dest++ = cval0;
			
			uint32_t lsb = sample & 255;
			uint32_t cval1 = (lsb * (modulo[1] + 1)) >> analog_write_res;
			if (cval1 > modulo[1]) cval1 = modulo[1];
			*dest1++ = cval1;
		}
		arm_dcache_flush_delete(dest, sizeof(pwm_tx_buffer[0]) / 2 );
		arm_dcache_flush_delete(dest1, sizeof(pwm_tx_buffer[1]) / 2 );
		
		AudioStream::release(block);
		block = NULL;
	} else {
		//Serial.println(".");

		// fill with silence when no data available
		uint32_t cval0 = (silence[0] * (modulo[0] + 1)) >> analog_write_res;
		if (cval0 > modulo[0]) cval0 = modulo[0];

		uint32_t cval1 = (silence[1] * (modulo[1] + 1)) >> analog_write_res;
		if (cval1 > modulo[1]) cval1 = modulo[1];

		for (unsigned i=0; i < AUDIO_BLOCK_SAMPLES / 2; i++) {
			*dest++ = cval0;
			*dest++ = cval0;
			*dest1++ = cval1;
			*dest1++ = cval1;
		}

		arm_dcache_flush_delete(dest, sizeof(pwm_tx_buffer[0]) / 2 );
		arm_dcache_flush_delete(dest1, sizeof(pwm_tx_buffer[1]) / 2 );
	}

        AudioStream::update_all();
	//digitalWriteFast(13, !digitalRead(13));
}

void AudioOutputPWM::update(void)
{
	audio_block_t *tblock;
	tblock = receiveReadOnly();
	if (!tblock) return;
	__disable_irq();
	block = tblock;
	__enable_irq();
}
#endif

#endif // __IMXRT1062__
