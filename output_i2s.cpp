#include "Audio.h"
#include "arm_math.h"


// MCLK needs to be 48e6 / 1088 * 256 = 11.29411765 MHz -> 44.117647 kHz sample rate
// Possible to create using fractional divider for all USB-compatible Kinetis:
// MCLK = 16e6 * 12 / 17
// MCLK = 24e6 * 8 / 17
// MCLK = 48e6 * 4 / 17
// MCLK = 72e6 * 8 / 51
// MCLK = 96e6 * 2 / 17
// MCLK = 120e6 * 8 / 85

// TODO: instigate using I2S0_MCR to select the crystal directly instead of the system
// clock, which has audio band jitter from the PLL



audio_block_t * AudioOutputI2S::block_left_1st = NULL;
audio_block_t * AudioOutputI2S::block_right_1st = NULL;
audio_block_t * AudioOutputI2S::block_left_2nd = NULL;
audio_block_t * AudioOutputI2S::block_right_2nd = NULL;
uint16_t  AudioOutputI2S::block_left_offset = 0;
uint16_t  AudioOutputI2S::block_right_offset = 0;
bool AudioOutputI2S::update_responsibility = false;
DMAMEM static uint32_t i2s_tx_buffer[AUDIO_BLOCK_SAMPLES];

void AudioOutputI2S::begin(void)
{
	//pinMode(2, OUTPUT);
	block_left_1st = NULL;
	block_right_1st = NULL;

	config_i2s();
	CORE_PIN22_CONFIG = PORT_PCR_MUX(6); // pin 22, PTC1, I2S0_TXD0

	DMA_CR = 0;
	DMA_TCD0_SADDR = i2s_tx_buffer;
	DMA_TCD0_SOFF = 2;
	DMA_TCD0_ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	DMA_TCD0_NBYTES_MLNO = 2;
	DMA_TCD0_SLAST = -sizeof(i2s_tx_buffer);
	DMA_TCD0_DADDR = &I2S0_TDR0;
	DMA_TCD0_DOFF = 0;
	DMA_TCD0_CITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;
	DMA_TCD0_DLASTSGA = 0;
	DMA_TCD0_BITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;
	DMA_TCD0_CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;

	DMAMUX0_CHCFG0 = DMAMUX_DISABLE;
	DMAMUX0_CHCFG0 = DMAMUX_SOURCE_I2S0_TX | DMAMUX_ENABLE;
	update_responsibility = update_setup();
	DMA_SERQ = 0;

	I2S0_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE | I2S_TCSR_FR;
	NVIC_ENABLE_IRQ(IRQ_DMA_CH0);
}




void dma_ch0_isr(void)
{
	const int16_t *src, *end;
	int16_t *dest;
	audio_block_t *block;
	uint32_t saddr, offset;

	saddr = (uint32_t)DMA_TCD0_SADDR;
        DMA_CINT = 0;
	if (saddr < (uint32_t)i2s_tx_buffer + sizeof(i2s_tx_buffer) / 2) {
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		dest = (int16_t *)&i2s_tx_buffer[AUDIO_BLOCK_SAMPLES/2];
		end = (int16_t *)&i2s_tx_buffer[AUDIO_BLOCK_SAMPLES];
		if (AudioOutputI2S::update_responsibility) AudioStream::update_all();
	} else {
		// DMA is transmitting the second half of the buffer
		// so we must fill the first half
		dest = (int16_t *)i2s_tx_buffer;
		end = (int16_t *)&i2s_tx_buffer[AUDIO_BLOCK_SAMPLES/2];
	}

	// TODO: these copy routines could be merged and optimized, maybe in assembly?
	block = AudioOutputI2S::block_left_1st;
	if (block) {
		offset = AudioOutputI2S::block_left_offset;
		src = &block->data[offset];
		do {
			*dest = *src++;
			dest += 2;
		} while (dest < end);
		offset += AUDIO_BLOCK_SAMPLES/2;
		if (offset < AUDIO_BLOCK_SAMPLES) {
			AudioOutputI2S::block_left_offset = offset;
		} else {
			AudioOutputI2S::block_left_offset = 0;
			AudioStream::release(block);
			AudioOutputI2S::block_left_1st = AudioOutputI2S::block_left_2nd;
			AudioOutputI2S::block_left_2nd = NULL;
		}
	} else {
		do {
			*dest = 0;
			dest += 2;
		} while (dest < end);
	}
	dest -= AUDIO_BLOCK_SAMPLES - 1;
	block = AudioOutputI2S::block_right_1st;
	if (block) {
		offset = AudioOutputI2S::block_right_offset;
		src = &block->data[offset];
		do {
			*dest = *src++;
			dest += 2;
		} while (dest < end);
		offset += AUDIO_BLOCK_SAMPLES/2;
		if (offset < AUDIO_BLOCK_SAMPLES) {
			AudioOutputI2S::block_right_offset = offset;
		} else {
			AudioOutputI2S::block_right_offset = 0;
			AudioStream::release(block);
			AudioOutputI2S::block_right_1st = AudioOutputI2S::block_right_2nd;
			AudioOutputI2S::block_right_2nd = NULL;
		}
	} else {
		do {
			*dest = 0;
			dest += 2;
		} while (dest < end);
	}
}




void AudioOutputI2S::update(void)
{
	// null audio device: discard all incoming data
	//if (!active) return;
	//audio_block_t *block = receiveReadOnly();
	//if (block) release(block);

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



void AudioOutputI2S::config_i2s(void)
{
	SIM_SCGC6 |= SIM_SCGC6_I2S;
	SIM_SCGC7 |= SIM_SCGC7_DMA;
	SIM_SCGC6 |= SIM_SCGC6_DMAMUX;

	// if either transmitter or receiver is enabled, do nothing
	if (I2S0_TCSR & I2S_TCSR_TE) return;
	if (I2S0_RCSR & I2S_RCSR_RE) return;

	// enable MCLK output
	I2S0_MCR = I2S_MCR_MICS(3) | I2S_MCR_MOE;
	I2S0_MDR = I2S_MDR_FRACT(1) | I2S_MDR_DIVIDE(16);

	// configure transmitter
	I2S0_TMR = 0;
	I2S0_TCR1 = I2S_TCR1_TFW(1);  // watermark at half fifo size
	I2S0_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_BCP | I2S_TCR2_MSEL(1)
		| I2S_TCR2_BCD | I2S_TCR2_DIV(3);
	I2S0_TCR3 = I2S_TCR3_TCE;
	I2S0_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(15) | I2S_TCR4_MF
		| I2S_TCR4_FSE | I2S_TCR4_FSP | I2S_TCR4_FSD;
	I2S0_TCR5 = I2S_TCR5_WNW(15) | I2S_TCR5_W0W(15) | I2S_TCR5_FBT(15);

	// configure receiver (sync'd to transmitter clocks)
	I2S0_RMR = 0;
	I2S0_RCR1 = I2S_RCR1_RFW(1);
	I2S0_RCR2 = I2S_RCR2_SYNC(1) | I2S_TCR2_BCP | I2S_RCR2_MSEL(1)
		| I2S_RCR2_BCD | I2S_RCR2_DIV(3);
	I2S0_RCR3 = I2S_RCR3_RCE;
	I2S0_RCR4 = I2S_RCR4_FRSZ(1) | I2S_RCR4_SYWD(15) | I2S_RCR4_MF
		| I2S_RCR4_FSE | I2S_RCR4_FSP | I2S_RCR4_FSD;
	I2S0_RCR5 = I2S_RCR5_WNW(15) | I2S_RCR5_W0W(15) | I2S_RCR5_FBT(15);

	// configure pin mux for 3 clock signals
	CORE_PIN23_CONFIG = PORT_PCR_MUX(6); // pin 23, PTC2, I2S0_TX_FS (LRCLK)
	CORE_PIN9_CONFIG  = PORT_PCR_MUX(6); // pin  9, PTC3, I2S0_TX_BCLK
	CORE_PIN11_CONFIG = PORT_PCR_MUX(6); // pin 11, PTC6, I2S0_MCLK
}



/******************************************************************/

void AudioOutputI2Sslave::begin(void)
{
	//pinMode(2, OUTPUT);
	block_left_1st = NULL;
	block_right_1st = NULL;

	AudioOutputI2Sslave::config_i2s();
	CORE_PIN22_CONFIG = PORT_PCR_MUX(6); // pin 22, PTC1, I2S0_TXD0

	DMA_CR = 0;
	DMA_TCD0_SADDR = i2s_tx_buffer;
	DMA_TCD0_SOFF = 2;
	DMA_TCD0_ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	DMA_TCD0_NBYTES_MLNO = 2;
	DMA_TCD0_SLAST = -sizeof(i2s_tx_buffer);
	DMA_TCD0_DADDR = &I2S0_TDR0;
	DMA_TCD0_DOFF = 0;
	DMA_TCD0_CITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;
	DMA_TCD0_DLASTSGA = 0;
	DMA_TCD0_BITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;
	DMA_TCD0_CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;

	DMAMUX0_CHCFG0 = DMAMUX_DISABLE;
	DMAMUX0_CHCFG0 = DMAMUX_SOURCE_I2S0_TX | DMAMUX_ENABLE;
	update_responsibility = update_setup();
	DMA_SERQ = 0;

	I2S0_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE | I2S_TCSR_FR;
	NVIC_ENABLE_IRQ(IRQ_DMA_CH0);
}

void AudioOutputI2Sslave::config_i2s(void)
{
	SIM_SCGC6 |= SIM_SCGC6_I2S;
	SIM_SCGC7 |= SIM_SCGC7_DMA;
	SIM_SCGC6 |= SIM_SCGC6_DMAMUX;

	// if either transmitter or receiver is enabled, do nothing
	if (I2S0_TCSR & I2S_TCSR_TE) return;
	if (I2S0_RCSR & I2S_RCSR_RE) return;

	// Select input clock 0
	// Configure to input the bit-clock from pin, bypasses the MCLK divider
	I2S0_MCR = I2S_MCR_MICS(0);
	I2S0_MDR = 0;

	// configure transmitter
	I2S0_TMR = 0;
	I2S0_TCR1 = I2S_TCR1_TFW(1);  // watermark at half fifo size
	I2S0_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_BCP;

	I2S0_TCR3 = I2S_TCR3_TCE;
	I2S0_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(15) | I2S_TCR4_MF
		| I2S_TCR4_FSE | I2S_TCR4_FSP;

	I2S0_TCR5 = I2S_TCR5_WNW(15) | I2S_TCR5_W0W(15) | I2S_TCR5_FBT(15);

	// configure receiver (sync'd to transmitter clocks)
	I2S0_RMR = 0;
	I2S0_RCR1 = I2S_RCR1_RFW(1);
	I2S0_RCR2 = I2S_RCR2_SYNC(1) | I2S_TCR2_BCP;

	I2S0_RCR3 = I2S_RCR3_RCE;
	I2S0_RCR4 = I2S_RCR4_FRSZ(1) | I2S_RCR4_SYWD(15) | I2S_RCR4_MF
		| I2S_RCR4_FSE | I2S_RCR4_FSP | I2S_RCR4_FSD;

	I2S0_RCR5 = I2S_RCR5_WNW(15) | I2S_RCR5_W0W(15) | I2S_RCR5_FBT(15);

	// configure pin mux for 3 clock signals
	CORE_PIN23_CONFIG = PORT_PCR_MUX(6); // pin 23, PTC2, I2S0_TX_FS (LRCLK)
	CORE_PIN9_CONFIG  = PORT_PCR_MUX(6); // pin  9, PTC3, I2S0_TX_BCLK
	CORE_PIN11_CONFIG = PORT_PCR_MUX(6); // pin 11, PTC6, I2S0_MCLK
}

/******************************************************************/


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



/******************************************************************/



