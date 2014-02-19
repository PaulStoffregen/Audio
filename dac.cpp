#include "Audio.h"
#include "arm_math.h"



// #define PDB_CONFIG (PDB_SC_TRGSEL(15) | PDB_SC_PDBEN | PDB_SC_CONT)
// #define PDB_PERIOD 1087 // 48e6 / 44100

#if defined(__MK20DX256__)

DMAMEM static uint16_t dac_buffer[AUDIO_BLOCK_SAMPLES*2];
audio_block_t * AudioOutputAnalog::block_left_1st = NULL;
audio_block_t * AudioOutputAnalog::block_left_2nd = NULL;
bool AudioOutputAnalog::update_responsibility = false;

void AudioOutputAnalog::begin(void)
{
	SIM_SCGC2 |= SIM_SCGC2_DAC0;
	DAC0_C0 = DAC_C0_DACEN | DAC_C0_DACRFS; // 3.3V VDDA is DACREF_2
	// slowly ramp up to DC voltage, approx 1/4 second
	for (int16_t i=0; i<128; i++) {
		analogWrite(A14, i);
		delay(2);
	}

	// set the programmable delay block to trigger DMA requests
	SIM_SCGC6 |= SIM_SCGC6_PDB;
	PDB0_IDLY = 1;
	PDB0_MOD = PDB_PERIOD;
	PDB0_SC = PDB_CONFIG | PDB_SC_LDOK;
	PDB0_SC = PDB_CONFIG | PDB_SC_SWTRIG | PDB_SC_PDBIE | PDB_SC_DMAEN;
	SIM_SCGC7 |= SIM_SCGC7_DMA;
	SIM_SCGC6 |= SIM_SCGC6_DMAMUX;
	DMA_CR = 0;
	DMA_TCD4_SADDR = dac_buffer;
	DMA_TCD4_SOFF = 2;
	DMA_TCD4_ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	DMA_TCD4_NBYTES_MLNO = 2;
	DMA_TCD4_SLAST = -sizeof(dac_buffer);
	DMA_TCD4_DADDR = &DAC0_DAT0L;
	DMA_TCD4_DOFF = 0;
	DMA_TCD4_CITER_ELINKNO = sizeof(dac_buffer) / 2;
	DMA_TCD4_DLASTSGA = 0;
	DMA_TCD4_BITER_ELINKNO = sizeof(dac_buffer) / 2;
	DMA_TCD4_CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	DMAMUX0_CHCFG4 = DMAMUX_DISABLE;
	DMAMUX0_CHCFG4 = DMAMUX_SOURCE_PDB | DMAMUX_ENABLE;
	update_responsibility = update_setup();
	DMA_SERQ = 4;
	NVIC_ENABLE_IRQ(IRQ_DMA_CH4);
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

void dma_ch4_isr(void)
{
	const int16_t *src, *end;
	int16_t *dest;
	audio_block_t *block;
	uint32_t saddr;

	saddr = (uint32_t)DMA_TCD4_SADDR;
        DMA_CINT = 4;
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
			*dest++ = ((*src++) + 32767) >> 4;
		} while (dest < end);
		AudioStream::release(block);
		AudioOutputAnalog::block_left_1st = AudioOutputAnalog::block_left_2nd;
		AudioOutputAnalog::block_left_2nd = NULL;
	} else {
		do {
			*dest++ = 2047;
		} while (dest < end);
	}
	if (AudioOutputAnalog::update_responsibility) AudioStream::update_all();
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



