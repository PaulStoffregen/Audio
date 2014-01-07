#include "Audio.h"
#include "arm_math.h"




static arm_cfft_radix4_instance_q15 fft_inst;

void AudioAnalyzeFFT256::init(void)
{
	// TODO: replace this with static const version
	arm_cfft_radix4_init_q15(&fft_inst, 256, 0, 1);

	//for (int i=0; i<2048; i++) {
		//buffer[i] = i * 3;
	//}
	//__disable_irq();
	//ARM_DEMCR |= ARM_DEMCR_TRCENA;
	//ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
	//uint32_t n = ARM_DWT_CYCCNT;
	//arm_cfft_radix2_q15(&fft_inst, buffer);
	//n = ARM_DWT_CYCCNT - n;
	//__enable_irq();
	//cycles = n;
	//arm_cmplx_mag_q15(buffer, buffer, 512);

	// each audio block is 278525 cycles @ 96 MHz
	// 256 point fft2 takes 65408 cycles
	// 256 point fft4 takes 49108 cycles
	// 128 point cmag takes 10999 cycles
	// 1024 point fft2 takes 125948 cycles
	// 1024 point fft4 takes 125840 cycles
	// 512 point cmag takes 43764 cycles

	//state = 0;
	//outputflag = false;
}

static void copy_to_fft_buffer(void *destination, const void *source)
{
	const int16_t *src = (const int16_t *)source;
	int16_t *dst = (int16_t *)destination;

	// TODO: optimize this
	for (int i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
		*dst++ = *src++;  // real
		*dst++ = 0;       // imaginary
	}
}

// computes limit((val >> rshift), 2**bits)
static inline int32_t signed_saturate_rshift(int32_t val, int bits, int rshift) __attribute__((always_inline));
static inline int32_t signed_saturate_rshift(int32_t val, int bits, int rshift)
{
	int32_t out;
	asm volatile("ssat %0, %1, %2, asr %3" : "=r" (out) : "I" (bits), "r" (val), "I" (rshift));
	return out;
}

static void apply_window_to_fft_buffer(void *buffer, const void *window)
{
	int16_t *buf = (int16_t *)buffer;
	const int16_t *win = (int16_t *)window;;

	for (int i=0; i < 256; i++) {
		int32_t val = *buf * *win++;
		//*buf = signed_saturate_rshift(val, 16, 15);
		*buf = val >> 15;
		buf += 2;
	}

}

void AudioAnalyzeFFT256::update(void)
{
	audio_block_t *block;

	block = receiveReadOnly();
	if (!block) return;
	if (!prevblock) {
		prevblock = block;
		return;
	}
	copy_to_fft_buffer(buffer, prevblock->data);
	copy_to_fft_buffer(buffer+256, block->data);
	//window = AudioWindowBlackmanNuttall256;
	//window = NULL;
	if (window) apply_window_to_fft_buffer(buffer, window);
	arm_cfft_radix4_q15(&fft_inst, buffer);
	// TODO: is this averaging correct?  G. Heinzel's paper says we're
	// supposed to average the magnitude squared, then do the square
	// root at the end after dividing by naverage.
	arm_cmplx_mag_q15(buffer, buffer, 128);
	if (count == 0) {
		for (int i=0; i < 128; i++) {
			output[i] = buffer[i];
		}
	} else {
		for (int i=0; i < 128; i++) {
			output[i] += buffer[i];
		}
	}
	if (++count == naverage) {
		count = 0;
		for (int i=0; i < 128; i++) {
			output[i] /= naverage;
		}
		outputflag = true;
	}

	release(prevblock);
	prevblock = block;

#if 0
	block = receiveReadOnly();
	if (state == 0) {
		//Serial.print("0");
		if (block == NULL) return;
		copy_to_fft_buffer(buffer, block->data);
		state = 1;
	} else if (state == 1) {
		//Serial.print("1");
		if (block == NULL) return;
		copy_to_fft_buffer(buffer+256, block->data);
		arm_cfft_radix4_q15(&fft_inst, buffer);
		state = 2;
	} else {
		//Serial.print("2");
		arm_cmplx_mag_q15(buffer, output, 128);
		outputflag = true;
		if (block == NULL) return;
		copy_to_fft_buffer(buffer, block->data);
		state = 1;
	}
	release(block);
#endif
}








static const int16_t sine_table[] = {
     0,   804,  1608,  2410,  3212,  4011,  4808,  5602,  6393,  7179,
  7962,  8739,  9512, 10278, 11039, 11793, 12539, 13279, 14010, 14732,
 15446, 16151, 16846, 17530, 18204, 18868, 19519, 20159, 20787, 21403,
 22005, 22594, 23170, 23731, 24279, 24811, 25329, 25832, 26319, 26790,
 27245, 27683, 28105, 28510, 28898, 29268, 29621, 29956, 30273, 30571,
 30852, 31113, 31356, 31580, 31785, 31971, 32137, 32285, 32412, 32521,
 32609, 32678, 32728, 32757, 32767, 32757, 32728, 32678, 32609, 32521,
 32412, 32285, 32137, 31971, 31785, 31580, 31356, 31113, 30852, 30571,
 30273, 29956, 29621, 29268, 28898, 28510, 28105, 27683, 27245, 26790,
 26319, 25832, 25329, 24811, 24279, 23731, 23170, 22594, 22005, 21403,
 20787, 20159, 19519, 18868, 18204, 17530, 16846, 16151, 15446, 14732,
 14010, 13279, 12539, 11793, 11039, 10278,  9512,  8739,  7962,  7179,
  6393,  5602,  4808,  4011,  3212,  2410,  1608,   804,     0,  -804,
 -1608, -2410, -3212, -4011, -4808, -5602, -6393, -7179, -7962, -8739,
 -9512,-10278,-11039,-11793,-12539,-13279,-14010,-14732,-15446,-16151,
-16846,-17530,-18204,-18868,-19519,-20159,-20787,-21403,-22005,-22594,
-23170,-23731,-24279,-24811,-25329,-25832,-26319,-26790,-27245,-27683,
-28105,-28510,-28898,-29268,-29621,-29956,-30273,-30571,-30852,-31113,
-31356,-31580,-31785,-31971,-32137,-32285,-32412,-32521,-32609,-32678,
-32728,-32757,-32767,-32757,-32728,-32678,-32609,-32521,-32412,-32285,
-32137,-31971,-31785,-31580,-31356,-31113,-30852,-30571,-30273,-29956,
-29621,-29268,-28898,-28510,-28105,-27683,-27245,-26790,-26319,-25832,
-25329,-24811,-24279,-23731,-23170,-22594,-22005,-21403,-20787,-20159,
-19519,-18868,-18204,-17530,-16846,-16151,-15446,-14732,-14010,-13279,
-12539,-11793,-11039,-10278, -9512, -8739, -7962, -7179, -6393, -5602,
 -4808, -4011, -3212, -2410, -1608,  -804,     0
};


void AudioSineWave::frequency(float f)
{
	if (f > AUDIO_SAMPLE_RATE_EXACT / 2 || f < 0.0) return;
	phase_increment = (f / AUDIO_SAMPLE_RATE_EXACT) * 4294967296.0f;
}

void AudioSineWave::update(void)
{
	audio_block_t *block;
	uint32_t i, ph, inc, index, scale;
	int32_t val1, val2, val3;

	//Serial.println("AudioSineWave::update");

	if (magnitude > 0 && (block = allocate()) != NULL) {
		ph = phase;
		inc = phase_increment;
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			index = ph >> 24;
			val1 = sine_table[index];
			val2 = sine_table[index+1];
			scale = (ph >> 8) & 0xFFFF;
			val2 *= scale;
			val1 *= 0xFFFF - scale;
			val3 = (val1 + val2) >> 16;
			block->data[i] = (val3 * magnitude) >> 15;
			 //Serial.print(block->data[i]);
			 //Serial.print(", ");
			 //if ((i % 12) == 11) Serial.println();
			ph += inc;
		}
		 //Serial.println();
		phase = ph;
		transmit(block);
		release(block);
	} else {
		// is this numerical overflow ok?
		phase += phase_increment * AUDIO_BLOCK_SAMPLES;
	}
}
















void AudioSineWaveMod::frequency(float f)
{
	if (f > AUDIO_SAMPLE_RATE_EXACT / 2 || f < 0.0) return;
	phase_increment = (f / AUDIO_SAMPLE_RATE_EXACT) * 4294967296.0f;
}

void AudioSineWaveMod::update(void)
{
	audio_block_t *block, *modinput;
	uint32_t i, ph, inc, index, scale;
	int32_t val1, val2;

	//Serial.println("AudioSineWave::update");
	modinput = receiveReadOnly();
	ph = phase;
	inc = phase_increment;
	block = allocate();
	if (!block) {
		// unable to allocate memory, so we'll send nothing
		if (modinput) {
			// but if we got modulation data, update the phase
			for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
				ph += inc + modinput->data[i] * modulation_factor;
			}
			release(modinput);
		} else {
			ph += phase_increment * AUDIO_BLOCK_SAMPLES;
		}
		phase = ph;
		return;
	}
	if (modinput) {
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			index = ph >> 24;
			val1 = sine_table[index];
			val2 = sine_table[index+1];
			scale = (ph >> 8) & 0xFFFF;
			val2 *= scale;
			val1 *= 0xFFFF - scale;
			block->data[i] = (val1 + val2) >> 16;
			 //Serial.print(block->data[i]);
			 //Serial.print(", ");
			 //if ((i % 12) == 11) Serial.println();
			ph += inc + modinput->data[i] * modulation_factor;
		}
		release(modinput);
	} else {
		ph = phase;
		inc = phase_increment;
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			index = ph >> 24;
			val1 = sine_table[index];
			val2 = sine_table[index+1];
			scale = (ph >> 8) & 0xFFFF;
			val2 *= scale;
			val1 *= 0xFFFF - scale;
			block->data[i] = (val1 + val2) >> 16;
			 //Serial.print(block->data[i]);
			 //Serial.print(", ");
			 //if ((i % 12) == 11) Serial.println();
			ph += inc;
		}
	}
	phase = ph;
	transmit(block);
	release(block);
}







/******************************************************************/


void AudioPrint::update(void)
{
	audio_block_t *block;
	uint32_t i;

	Serial.println("AudioPrint::update");
	Serial.println(name);
	block = receiveReadOnly();
	if (block) {
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			Serial.print(block->data[i]);
			Serial.print(", ");
			if ((i % 12) == 11) Serial.println();
		}
		Serial.println();
		release(block);
	}
}


/******************************************************************/

audio_block_t * AudioOutputPWM::block_1st = NULL;
audio_block_t * AudioOutputPWM::block_2nd = NULL;
uint32_t  AudioOutputPWM::block_offset = 0;
bool AudioOutputPWM::update_responsibility = false;
uint8_t AudioOutputPWM::interrupt_count = 0;

DMAMEM uint32_t pwm_dma_buffer[AUDIO_BLOCK_SAMPLES*2];

void AudioOutputPWM::begin(void)
{
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
	for (int i=0; i<256; i+=2) {
		pwm_dma_buffer[i] = 120; // zero must not be used
		pwm_dma_buffer[i+1] = 0;
	}
	SIM_SCGC7 |= SIM_SCGC7_DMA;
	SIM_SCGC6 |= SIM_SCGC6_DMAMUX;
	DMA_CR = 0;
	DMA_TCD3_SADDR = pwm_dma_buffer;
	DMA_TCD3_SOFF = 4;
	DMA_TCD3_ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2) | DMA_TCD_ATTR_DMOD(4);
	DMA_TCD3_NBYTES_MLNO = 8;
	DMA_TCD3_SLAST = -sizeof(pwm_dma_buffer);
	DMA_TCD3_DADDR = &FTM1_C0V;
	DMA_TCD3_DOFF = 8;
	DMA_TCD3_CITER_ELINKNO = sizeof(pwm_dma_buffer) / 8;
	DMA_TCD3_DLASTSGA = 0;
	DMA_TCD3_BITER_ELINKNO = sizeof(pwm_dma_buffer) / 8;
	DMA_TCD3_CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	DMAMUX0_CHCFG3 = DMAMUX_DISABLE;
	DMAMUX0_CHCFG3 = DMAMUX_SOURCE_FTM1_CH0 | DMAMUX_ENABLE;
	DMA_SERQ = 3;
	update_responsibility = update_setup();
	NVIC_ENABLE_IRQ(IRQ_DMA_CH3);
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

void dma_ch3_isr(void)
{
	int16_t *src;
	uint32_t *dest;
	audio_block_t *block;
	uint32_t saddr, offset;

	saddr = (uint32_t)DMA_TCD3_SADDR;
        DMA_CINT = 3;
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



/******************************************************************/


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











DMAMEM static uint16_t analog_rx_buffer[AUDIO_BLOCK_SAMPLES];
audio_block_t * AudioInputAnalog::block_left = NULL;
uint16_t AudioInputAnalog::block_offset = 0;

#define PDB_CONFIG (PDB_SC_TRGSEL(15) | PDB_SC_PDBEN | PDB_SC_CONT)
#define PDB_PERIOD 1087 // 48e6 / 44100

void AudioInputAnalog::begin(unsigned int pin)
{
	uint32_t i, sum=0;

	// pin must be 0 to 13 (for A0 to A13)
	// or 14 to 23 for digital pin numbers A0-A9
	// or 34 to 37 corresponding to A10-A13
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
	//update_responsibility = update_setup();
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
		//if (AudioInputI2S::update_responsibility) AudioStream::update_all();
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























/******************************************************************/


#define STATE_DIRECT_8BIT_MONO		0  // playing mono at native sample rate
#define STATE_DIRECT_8BIT_STEREO	1  // playing stereo at native sample rate
#define STATE_DIRECT_16BIT_MONO		2  // playing mono at native sample rate
#define STATE_DIRECT_16BIT_STEREO	3  // playing stereo at native sample rate
#define STATE_CONVERT_8BIT_MONO		4  // playing mono, converting sample rate
#define STATE_CONVERT_8BIT_STEREO	5  // playing stereo, converting sample rate
#define STATE_CONVERT_16BIT_MONO	6  // playing mono, converting sample rate
#define STATE_CONVERT_16BIT_STEREO	7  // playing stereo, converting sample rate
#define STATE_PARSE1			8  // looking for 20 byte ID header
#define STATE_PARSE2			9  // looking for 16 byte format header
#define STATE_PARSE3			10 // looking for 8 byte data header
#define STATE_PARSE4			11 // ignoring unknown chunk
#define STATE_STOP			12

void AudioPlaySDcardWAV::begin(void)
{
	state = STATE_STOP;
	state_play = STATE_STOP;
	data_length = 0;
	if (block_left) {
		release(block_left);
		block_left = NULL;
	}
	if (block_right) {
		release(block_right);
		block_right = NULL;
	}
}


bool AudioPlaySDcardWAV::play(const char *filename)
{
	stop();
	wavfile = SD.open(filename);
	if (!wavfile) return false;
	buffer_remaining = 0;
	state_play = STATE_STOP;
	state = STATE_PARSE1;
	return true;
}

void AudioPlaySDcardWAV::stop(void)
{
	__disable_irq();
	if (state != STATE_STOP) {
		state = STATE_STOP;
		__enable_irq();
		wavfile.close();
	} else {
		__enable_irq();
	}
}

bool AudioPlaySDcardWAV::start(void)
{
	__disable_irq();
	if (state == STATE_STOP) {
		if (state_play == STATE_STOP) {
			__enable_irq();
			return false;
		}
		state = state_play;
	}
	__enable_irq();
	return true;
}


void AudioPlaySDcardWAV::update(void)
{
	// only update if we're playing
	if (state == STATE_STOP) return;

	// allocate the audio blocks to transmit
	block_left = allocate();
	if (block_left == NULL) return;
	if (state < 8 && (state & 1) == 1) {
		// if we're playing stereo, allocate another
		// block for the right channel output
		block_right = allocate();
		if (block_right == NULL) {
			release(block_left);
			return;
		}
	} else {
		// if we're playing mono or just parsing
		// the WAV file header, no right-side block
		block_right = NULL;
	}
	block_offset = 0;

	//Serial.println("update");

	// is there buffered data?
	if (buffer_remaining > 0) {
		// we have buffered data
		if (consume()) return; // it was enough to transmit audio
	}

	// we only get to this point when buffer[512] is empty
	if (state != STATE_STOP && wavfile.available()) {
		// we can read more data from the file...
		buffer_remaining = wavfile.read(buffer, 512);
		if (consume()) {
			// good, it resulted in audio transmit
			return;
		} else {
			// not good, no audio was transmitted
			buffer_remaining = 0;
			if (block_left) release(block_left);
			if (block_right) release(block_right);
			// if we're still playing, well, there's going to
			// be a gap in output, but we can't keep burning
			// time trying to read more data.  Hopefully things
			// will go better next time?
			if (state != STATE_STOP) return;
		}
	}
	// end of file reached or other reason to stop
	wavfile.close();
	state_play = STATE_STOP;
	state = STATE_STOP;
}


// https://ccrma.stanford.edu/courses/422/projects/WaveFormat/

// Consume already buffered data.  Returns true if audio transmitted.
bool AudioPlaySDcardWAV::consume(void)
{
	uint32_t len, size;
	uint8_t lsb, msb;
	const uint8_t *p;

	size = buffer_remaining;
	p = buffer + 512 - size;
start:
	if (size == 0) return false;
	//Serial.print("AudioPlaySDcardWAV write, size = ");
	//Serial.print(size);
	//Serial.print(", data_length = ");
	//Serial.print(data_length);
	//Serial.print(", state = ");
	//Serial.println(state);
	switch (state) {
	  // parse wav file header, is this really a .wav file?
	  case STATE_PARSE1:
		len = 20 - data_length;
		if (size < len) len = size;
		memcpy((uint8_t *)header + data_length, p, len);
		data_length += len;
		if (data_length < 20) return false;
		// parse the header...
		if (header[0] == 0x46464952 && header[2] == 0x45564157
		  && header[3] == 0x20746D66 && header[4] == 16) {
			//Serial.println("header ok");
			state = STATE_PARSE2;
			p += len;
			size -= len;
			data_length = 0;
			goto start;
		}
		//Serial.println("unknown WAV header");
		break;

	  // check & extract key audio parameters
	  case STATE_PARSE2:
		len = 16 - data_length;
		if (size < len) len = size;
		memcpy((uint8_t *)header + data_length, p, len);
		data_length += len;
		if (data_length < 16) return false;
		if (parse_format()) {
			//Serial.println("audio format ok");
			p += len;
			size -= len;
			data_length = 0;
			state = STATE_PARSE3;
			goto start;
		}
		//Serial.println("unknown audio format");
		break;

	  // find the data chunk
	  case STATE_PARSE3:
		len = 8 - data_length;
		if (size < len) len = size;
		memcpy((uint8_t *)header + data_length, p, len);
		data_length += len;
		if (data_length < 8) return false;
		//Serial.print("chunk id = ");
		//Serial.print(header[0], HEX);
		//Serial.print(", length = ");
		//Serial.println(header[1]);
		p += len;
		size -= len;
		data_length = header[1];
		if (header[0] == 0x61746164) {
			//Serial.println("found data chunk");
			// TODO: verify offset in file is an even number
			// as required by WAV format.  abort if odd.  Code
			// below will depend upon this and fail if not even.
			leftover_bytes = 0;
			state = state_play;
		} else {
			state = STATE_PARSE4;
		}
		goto start;

	  // ignore any extra unknown chunks (title & artist info)
	  case STATE_PARSE4:
		if (size < data_length) {
			data_length -= size;
			return false;
		}
		p += data_length;
		size -= data_length;
		data_length = 0;
		state = STATE_PARSE3;
		goto start;

	  // playing mono at native sample rate
	  case STATE_DIRECT_8BIT_MONO:
		return false;

	  // playing stereo at native sample rate
	  case STATE_DIRECT_8BIT_STEREO:
		return false;

	  // playing mono at native sample rate
	  case STATE_DIRECT_16BIT_MONO:
		if (size > data_length) {
			size = data_length;
		}
		data_length -= size;

		while (1) {
			lsb = *p++;
			msb = *p++;
			size -= 2;
			block_left->data[block_offset++] = (msb << 8) | lsb;
			if (block_offset >= AUDIO_BLOCK_SAMPLES) {
				transmit(block_left, 0);
				transmit(block_left, 1);
				 //Serial1.print('%');
				 //delayMicroseconds(90);
				release(block_left);
				block_left = NULL;
				data_length += size;
				buffer_remaining = size;
				if (block_right) release(block_right);
				return true;
			}
			if (size == 0) {
				if (data_length == 0) break;
				return false;
			}
		}
		// end of file reached
		if (block_offset > 0) {
			// TODO: fill remainder of last block with zero and transmit
		}
		state = STATE_STOP;
		return false;


	  // playing stereo at native sample rate
	  case STATE_DIRECT_16BIT_STEREO:
		return false;

	  // playing mono, converting sample rate
	  case STATE_CONVERT_8BIT_MONO :
		return false;

	  // playing stereo, converting sample rate
	  case STATE_CONVERT_8BIT_STEREO:
		return false;

	  // playing mono, converting sample rate
	  case STATE_CONVERT_16BIT_MONO:
		return false;

	  // playing stereo, converting sample rate
	  case STATE_CONVERT_16BIT_STEREO:
		return false;

	  // ignore any extra data after playing
	  // or anything following any error
	  case STATE_STOP:
		return false;

	  // this is not supposed to happen!
	  //default:
		//Serial.println("AudioPlaySDcardWAV, unknown state");
	}
	state_play = STATE_STOP;
	state = STATE_STOP;
	return false;
}


/*
00000000  52494646 66EA6903 57415645 666D7420  RIFFf.i.WAVEfmt 
00000010  10000000 01000200 44AC0000 10B10200  ........D.......
00000020  04001000 4C495354 3A000000 494E464F  ....LIST:...INFO
00000030  494E414D 14000000 49205761 6E742054  INAM....I Want T
00000040  6F20436F 6D65204F 76657200 49415254  o Come Over.IART
00000050  12000000 4D656C69 73736120 45746865  ....Melissa Ethe
00000060  72696467 65006461 746100EA 69030100  ridge.data..i...
00000070  FEFF0300 FCFF0400 FDFF0200 0000FEFF  ................
00000080  0300FDFF 0200FFFF 00000100 FEFF0300  ................
00000090  FDFF0300 FDFF0200 FFFF0100 0000FFFF  ................
*/





// SD library on Teensy3 at 96 MHz
//  256 byte chunks, speed is 443272 bytes/sec
//  512 byte chunks, speed is 468023 bytes/sec





bool AudioPlaySDcardWAV::parse_format(void)
{
	uint8_t num = 0;
	uint16_t format;
	uint16_t channels;
	uint32_t rate;
	uint16_t bits;

	format = header[0];
	//Serial.print("  format = ");
	//Serial.println(format);
	if (format != 1) return false;

	channels = header[0] >> 16;
	//Serial.print("  channels = ");
	//Serial.println(channels);
	if (channels == 1) {
	} else if (channels == 2) {
		num = 1;
	} else {
		return false;
	}

	bits = header[3] >> 16;
	//Serial.print("  bits = ");
	//Serial.println(bits);
	if (bits == 8) {
	} else if (bits == 16) {
		num |= 2;
	} else {
		return false;
	}

	rate = header[1];
	//Serial.print("  rate = ");
	//Serial.println(rate);
	if (rate == AUDIO_SAMPLE_RATE) {
	} else if (rate >= 8000 && rate <= 48000) {
		num |= 4;
	} else {
		return false;
	}
	// we're not checking the byte rate and block align fields
	// if they're not the expected values, all we could do is
	// return false.  Do any real wav files have unexpected
	// values in these other fields?
	state_play = num;
	return true;
}



/******************************************************************/


void AudioPlaySDcardRAW::begin(void)
{
	playing = false;
	if (block) {
		release(block);
		block = NULL;
	}
}


bool AudioPlaySDcardRAW::play(const char *filename)
{
	stop();
	rawfile = SD.open(filename);
	if (!rawfile) {
		Serial.println("unable to open file");
		return false;
	}
	Serial.println("able to open file");
	playing = true;
	return true;
}

void AudioPlaySDcardRAW::stop(void)
{
	__disable_irq();
	if (playing) {
		playing = false;
		__enable_irq();
		rawfile.close();
	} else {
		__enable_irq();
	}
}


void AudioPlaySDcardRAW::update(void)
{
	unsigned int i, n;

	// only update if we're playing
	if (!playing) return;

	// allocate the audio blocks to transmit
	block = allocate();
	if (block == NULL) return;

	if (rawfile.available()) {
		// we can read more data from the file...
		n = rawfile.read(block->data, AUDIO_BLOCK_SAMPLES*2);
		for (i=n/2; i < AUDIO_BLOCK_SAMPLES; i++) {
			block->data[i] = 0;
		}
		transmit(block);
		release(block);
	} else {
		rawfile.close();
		playing = false;
	}
}





/******************************************************************/




// computes ((a[31:0] * b[15:0]) >> 16)
static inline int32_t signed_multiply_32x16b(int32_t a, uint32_t b) __attribute__((always_inline));
static inline int32_t signed_multiply_32x16b(int32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smulwb %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes ((a[31:0] * b[31:16]) >> 16)
static inline int32_t signed_multiply_32x16t(int32_t a, uint32_t b) __attribute__((always_inline));
static inline int32_t signed_multiply_32x16t(int32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smulwt %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}


// computes ((a[15:0) << 16) | b[15:0])
static inline uint32_t pack_16x16(int32_t a, int32_t b) __attribute__((always_inline));
static inline uint32_t pack_16x16(int32_t a, int32_t b)
{
	int32_t out;
	asm volatile("pkhbt %0, %1, %2, lsl #16" : "=r" (out) : "r" (b), "r" (a));
	return out;
}

// computes (((a[31:16] + b[31:16]) << 16) | (a[15:0 + b[15:0]))
static inline uint32_t signed_add_16_and_16(uint32_t a, uint32_t b) __attribute__((always_inline));
static inline uint32_t signed_add_16_and_16(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("qadd16 %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}


void applyGain(int16_t *data, int32_t mult)
{
	uint32_t *p = (uint32_t *)data;
	const uint32_t *end = (uint32_t *)(data + AUDIO_BLOCK_SAMPLES);

	do {
		uint32_t tmp32 = *p; // read 2 samples from *data
		int32_t val1 = signed_multiply_32x16b(mult, tmp32);
		int32_t val2 = signed_multiply_32x16t(mult, tmp32);
		val1 = signed_saturate_rshift(val1, 16, 0);
		val2 = signed_saturate_rshift(val2, 16, 0);
		*p++ = pack_16x16(val2, val1);
	} while (p < end);
}

// page 133

void applyGainThenAdd(int16_t *data, const int16_t *in, int32_t mult)
{
	uint32_t *dst = (uint32_t *)data;
	const uint32_t *src = (uint32_t *)in;
	const uint32_t *end = (uint32_t *)(data + AUDIO_BLOCK_SAMPLES);

	if (mult == 65536) {
		do {
			uint32_t tmp32 = *dst;
			*dst++ = signed_add_16_and_16(tmp32, *src++);
			tmp32 = *dst;
			*dst++ = signed_add_16_and_16(tmp32, *src++);
		} while (dst < end);
	} else {
		do {
			uint32_t tmp32 = *src++; // read 2 samples from *data
			int32_t val1 = signed_multiply_32x16b(mult, tmp32);
			int32_t val2 = signed_multiply_32x16t(mult, tmp32);
			val1 = signed_saturate_rshift(val1, 16, 0);
			val2 = signed_saturate_rshift(val2, 16, 0);
			tmp32 = pack_16x16(val2, val1);
			uint32_t tmp32b = *dst;
			*dst++ = signed_add_16_and_16(tmp32, tmp32b);
		} while (dst < end);
	}
}


void AudioMixer4::update(void)
{
	audio_block_t *in, *out=NULL;
	unsigned int channel;

	for (channel=0; channel < 4; channel++) {
		if (!out) {
			out = receiveWritable(channel);
			if (out) {
				int32_t mult = multiplier[channel];
				if (mult != 65536) applyGain(out->data, mult);
			}
		} else {
			in = receiveReadOnly(channel);
			if (in) {
				applyGainThenAdd(out->data, in->data, multiplier[channel]);
				release(in);
			}
		}
	}
	if (out) {
		transmit(out);
		release(out);
	}
}




/******************************************************************/

#include "Wire.h"

#define WM8731_I2C_ADDR 0x1A
//#define WM8731_I2C_ADDR 0x1B

#define WM8731_REG_LLINEIN	0
#define WM8731_REG_RLINEIN	1
#define WM8731_REG_LHEADOUT	2
#define WM8731_REG_RHEADOUT	3
#define WM8731_REG_ANALOG	4
#define WM8731_REG_DIGITAL	5
#define WM8731_REG_POWERDOWN	6
#define WM8731_REG_INTERFACE	7
#define WM8731_REG_SAMPLING	8
#define WM8731_REG_ACTIVE	9
#define WM8731_REG_RESET	15

bool AudioControlWM8731::enable(void)
{
	Wire.begin();
	delay(5);
	//write(WM8731_REG_RESET, 0);

	write(WM8731_REG_INTERFACE, 0x02); // I2S, 16 bit, MCLK slave
	write(WM8731_REG_SAMPLING, 0x20);  // 256*Fs, 44.1 kHz, MCLK/1

	// In order to prevent pops, the DAC should first be soft-muted (DACMU),
	// the output should then be de-selected from the line and headphone output
	// (DACSEL), then the DAC powered down (DACPD).

	write(WM8731_REG_DIGITAL, 0x08);   // DAC soft mute
	write(WM8731_REG_ANALOG, 0x00);    // disable all

	write(WM8731_REG_POWERDOWN, 0x60); // linein, mic, adc, dac, lineout

	write(WM8731_REG_LHEADOUT, 0x80);      // volume off
	write(WM8731_REG_RHEADOUT, 0x80);

	delay(100); // how long to power up?

	write(WM8731_REG_ACTIVE, 1);
	delay(5);
	write(WM8731_REG_DIGITAL, 0x00);   // DAC unmuted
	write(WM8731_REG_ANALOG, 0x10);    // DAC selected

	return true;
}


bool AudioControlWM8731::write(unsigned int reg, unsigned int val)
{
	Wire.beginTransmission(WM8731_I2C_ADDR);
	Wire.write((reg << 1) | ((val >> 8) & 1));
	Wire.write(val & 0xFF);
	Wire.endTransmission();
	return true;
}

bool AudioControlWM8731::volumeInteger(unsigned int n)
{
	if (n > 127) n = 127;
	 //Serial.print("volumeInteger, n = ");
	 //Serial.println(n);
	write(WM8731_REG_LHEADOUT, n | 0x180);
	write(WM8731_REG_RHEADOUT, n | 0x80);
	return true;
}



/******************************************************************/

#define CHIP_ID				0x0000
// 15:8 PARTID		0xA0 - 8 bit identifier for SGTL5000
// 7:0  REVID		0x00 - revision number for SGTL5000.

#define CHIP_DIG_POWER			0x0002
// 6	ADC_POWERUP	1=Enable, 0=disable the ADC block, both digital & analog, 
// 5	DAC_POWERUP	1=Enable, 0=disable the DAC block, both analog and digital
// 4	DAP_POWERUP	1=Enable, 0=disable the DAP block
// 1	I2S_OUT_POWERUP	1=Enable, 0=disable the I2S data output
// 0	I2S_IN_POWERUP	1=Enable, 0=disable the I2S data input

#define CHIP_CLK_CTRL			0x0004
// 5:4	RATE_MODE	Sets the sample rate mode. MCLK_FREQ is still specified
//			relative to the rate in SYS_FS
//				0x0 = SYS_FS specifies the rate
//				0x1 = Rate is 1/2 of the SYS_FS rate
//				0x2 = Rate is 1/4 of the SYS_FS rate
//				0x3 = Rate is 1/6 of the SYS_FS rate
// 3:2	SYS_FS		Sets the internal system sample rate (default=2)
//				0x0 = 32 kHz
//				0x1 = 44.1 kHz
//				0x2 = 48 kHz
//				0x3 = 96 kHz
// 1:0	MCLK_FREQ	Identifies incoming SYS_MCLK frequency and if the PLL should be used
//				0x0 = 256*Fs
//				0x1 = 384*Fs
//				0x2 = 512*Fs
//				0x3 = Use PLL
//				The 0x3 (Use PLL) setting must be used if the SYS_MCLK is not
//				a standard multiple of Fs (256, 384, or 512). This setting can
//				also be used if SYS_MCLK is a standard multiple of Fs.
//				Before this field is set to 0x3 (Use PLL), the PLL must be
//				powered up by setting CHIP_ANA_POWER->PLL_POWERUP and
//				CHIP_ANA_POWER->VCOAMP_POWERUP.  Also, the PLL dividers must
//				be calculated based on the external MCLK rate and
//				CHIP_PLL_CTRL register must be set (see CHIP_PLL_CTRL register
//				description details on how to calculate the divisors).

#define CHIP_I2S_CTRL			0x0006
// 8	SCLKFREQ	Sets frequency of I2S_SCLK when in master mode (MS=1). When in slave
//			mode (MS=0), this field must be set appropriately to match SCLK input
//			rate.
//				0x0 = 64Fs
//				0x1 = 32Fs - Not supported for RJ mode (I2S_MODE = 1)
// 7	MS		Configures master or slave of I2S_LRCLK and I2S_SCLK.
//				0x0 = Slave: I2S_LRCLK an I2S_SCLK are inputs
//				0x1 = Master: I2S_LRCLK and I2S_SCLK are outputs
//				NOTE: If the PLL is used (CHIP_CLK_CTRL->MCLK_FREQ==0x3),
//				the SGTL5000 must be a master of the I2S port (MS==1)
// 6	SCLK_INV	Sets the edge that data (input and output) is clocked in on for I2S_SCLK
//				0x0 = data is valid on rising edge of I2S_SCLK
//				0x1 = data is valid on falling edge of I2S_SCLK
// 5:4	DLEN		I2S data length (default=1)
//				0x0 = 32 bits (only valid when SCLKFREQ=0),
//					not valid for Right Justified Mode
//				0x1 = 24 bits (only valid when SCLKFREQ=0)
//				0x2 = 20 bits
//				0x3 = 16 bits
// 3:2	I2S_MODE	Sets the mode for the I2S port
//				0x0 = I2S mode or Left Justified (Use LRALIGN to select)
//				0x1 = Right Justified Mode
//				0x2 = PCM Format A/B
//				0x3 = RESERVED
// 1	LRALIGN		I2S_LRCLK Alignment to data word. Not used for Right Justified mode
//				0x0 = Data word starts 1 I2S_SCLK delay after I2S_LRCLK
//					transition (I2S format, PCM format A)
//				0x1 = Data word starts after I2S_LRCLK transition (left
//					justified format, PCM format B)
// 0	LRPOL		I2S_LRCLK Polarity when data is presented.
//				0x0 = I2S_LRCLK = 0 - Left, 1 - Right
//				1x0 = I2S_LRCLK = 0 - Right, 1 - Left
//				The left subframe should be presented first regardless of
//				the setting of LRPOL.

#define CHIP_SSS_CTRL			0x000A
// 14	DAP_MIX_LRSWAP	DAP Mixer Input Swap
//				0x0 = Normal Operation
//				0x1 = Left and Right channels for the DAP MIXER Input are swapped.
// 13	DAP_LRSWAP	DAP Mixer Input Swap
//				0x0 = Normal Operation
//				0x1 = Left and Right channels for the DAP Input are swapped
// 12	DAC_LRSWAP	DAC Input Swap
//				0x0 = Normal Operation
//				0x1 = Left and Right channels for the DAC are swapped
// 10	I2S_LRSWAP	I2S_DOUT Swap
//				0x0 = Normal Operation
//				0x1 = Left and Right channels for the I2S_DOUT are swapped
// 9:8	DAP_MIX_SELECT	Select data source for DAP mixer
//				0x0 = ADC
//				0x1 = I2S_IN
//				0x2 = Reserved
//				0x3 = Reserved
// 7:6	DAP_SELECT	Select data source for DAP
//				0x0 = ADC
//				0x1 = I2S_IN
//				0x2 = Reserved
//				0x3 = Reserved
// 5:4	DAC_SELECT	Select data source for DAC (default=1)
//				0x0 = ADC
//				0x1 = I2S_IN
//				0x2 = Reserved
//				0x3 = DAP
// 1:0	I2S_SELECT	Select data source for I2S_DOUT
//				0x0 = ADC
//				0x1 = I2S_IN
//				0x2 = Reserved
//				0x3 = DAP

#define CHIP_ADCDAC_CTRL		0x000E
// 13	VOL_BUSY_DAC_RIGHT Volume Busy DAC Right
//				0x0 = Ready
//				0x1 = Busy - This indicates the channel has not reached its
//					programmed volume/mute level
// 12	VOL_BUSY_DAC_LEFT  Volume Busy DAC Left
//				0x0 = Ready
//				0x1 = Busy - This indicates the channel has not reached its
//					programmed volume/mute level
// 9	VOL_RAMP_EN	Volume Ramp Enable (default=1)
//				0x0 = Disables volume ramp. New volume settings take immediate
//					effect without a ramp
//				0x1 = Enables volume ramp
//				This field affects DAC_VOL. The volume ramp effects both
//				volume settings and mute When set to 1 a soft mute is enabled.
// 8	VOL_EXPO_RAMP	Exponential Volume Ramp Enable
//				0x0 = Linear ramp over top 4 volume octaves
//				0x1 = Exponential ramp over full volume range
//				This bit only takes effect if VOL_RAMP_EN is 1.
// 3	DAC_MUTE_RIGHT	DAC Right Mute (default=1)
//				0x0 = Unmute
//				0x1 = Muted
//				If VOL_RAMP_EN = 1, this is a soft mute.
// 2	DAC_MUTE_LEFT	DAC Left Mute (default=1)
//				0x0 = Unmute
//				0x1 = Muted
//				If VOL_RAMP_EN = 1, this is a soft mute.
// 1	ADC_HPF_FREEZE	ADC High Pass Filter Freeze
//				0x0 = Normal operation
//				0x1 = Freeze the ADC high-pass filter offset register.  The
//				offset continues to be subtracted from the ADC data stream.
// 0	ADC_HPF_BYPASS	ADC High Pass Filter Bypass
//				0x0 = Normal operation
//				0x1 = Bypassed and offset not updated

#define CHIP_DAC_VOL			0x0010
// 15:8	DAC_VOL_RIGHT	DAC Right Channel Volume.  Set the Right channel DAC volume
//			with 0.5017 dB steps from 0 to -90 dB
//				0x3B and less = Reserved
//				0x3C = 0 dB
//				0x3D = -0.5 dB
//				0xF0 = -90 dB
//				0xFC and greater = Muted
//				If VOL_RAMP_EN = 1, there is an automatic ramp to the
//				new volume setting.
// 7:0	DAC_VOL_LEFT	DAC Left Channel Volume.  Set the Left channel DAC volume
//			with 0.5017 dB steps from 0 to -90 dB
//				0x3B and less = Reserved
//				0x3C = 0 dB
//				0x3D = -0.5 dB
//				0xF0 = -90 dB
//				0xFC and greater = Muted
//				If VOL_RAMP_EN = 1, there is an automatic ramp to the
//				new volume setting.

#define CHIP_PAD_STRENGTH		0x0014
// 9:8	I2S_LRCLK	I2S LRCLK Pad Drive Strength (default=1)
//				Sets drive strength for output pads per the table below.
//				 VDDIO    1.8 V     2.5 V     3.3 V
//				0x0 = Disable
//				0x1 =     1.66 mA   2.87 mA   4.02 mA
//				0x2 =     3.33 mA   5.74 mA   8.03 mA
//				0x3 =     4.99 mA   8.61 mA   12.05 mA
// 7:6	I2S_SCLK	I2S SCLK Pad Drive Strength (default=1)
// 5:4	I2S_DOUT	I2S DOUT Pad Drive Strength (default=1)
// 3:2	CTRL_DATA	I2C DATA Pad Drive Strength (default=3)
// 1:0	CTRL_CLK	I2C CLK Pad Drive Strength (default=3)
//				(all use same table as I2S_LRCLK)

#define CHIP_ANA_ADC_CTRL		0x0020
// 8	ADC_VOL_M6DB	ADC Volume Range Reduction
//				This bit shifts both right and left analog ADC volume
//				range down by 6.0 dB.
//				0x0 = No change in ADC range
//				0x1 = ADC range reduced by 6.0 dB
// 7:4	ADC_VOL_RIGHT	ADC Right Channel Volume
//				Right channel analog ADC volume control in 1.5 dB steps.
//				0x0 = 0 dB
//				0x1 = +1.5 dB
//				...
//				0xF = +22.5 dB
//				This range is -6.0 dB to +16.5 dB if ADC_VOL_M6DB is set to 1.
// 3:0	ADC_VOL_LEFT	ADC Left Channel Volume
//				(same scale as ADC_VOL_RIGHT)

#define CHIP_ANA_HP_CTRL		0x0022
// 14:8	HP_VOL_RIGHT	Headphone Right Channel Volume  (default 0x18)
//				Right channel headphone volume control with 0.5 dB steps.
//				0x00 = +12 dB
//				0x01 = +11.5 dB
//				0x18 = 0 dB
//				...
//				0x7F = -51.5 dB
// 6:0	HP_VOL_LEFT	Headphone Left Channel Volume  (default 0x18)
//				(same scale as HP_VOL_RIGHT)

#define CHIP_ANA_CTRL			0x0024
// 8	MUTE_LO		LINEOUT Mute, 0 = Unmute, 1 = Mute  (default 1)
// 6	SELECT_HP	Select the headphone input, 0 = DAC, 1 = LINEIN
// 5	EN_ZCD_HP	Enable the headphone zero cross detector (ZCD)
//				0x0 = HP ZCD disabled
//				0x1 = HP ZCD enabled
// 4	MUTE_HP		Mute the headphone outputs, 0 = Unmute, 1 = Mute (default)
// 2	SELECT_ADC	Select the ADC input, 0 = Microphone, 1 = LINEIN
// 1	EN_ZCD_ADC	Enable the ADC analog zero cross detector (ZCD)
//				0x0 = ADC ZCD disabled
//				0x1 = ADC ZCD enabled
// 0	MUTE_ADC	Mute the ADC analog volume, 0 = Unmute, 1 = Mute (default)

#define CHIP_LINREG_CTRL		0x0026
// 6	VDDC_MAN_ASSN	Determines chargepump source when VDDC_ASSN_OVRD is set.
//				0x0 = VDDA
//				0x1 = VDDIO
// 5	VDDC_ASSN_OVRD	Charge pump Source Assignment Override
//				0x0 = Charge pump source is automatically assigned based
//					on higher of VDDA and VDDIO
//				0x1 = the source of charge pump is manually assigned by
//					VDDC_MAN_ASSN If VDDIO and VDDA are both the same
//					and greater than 3.1 V, VDDC_ASSN_OVRD and
//					VDDC_MAN_ASSN should be used to manually assign
//					VDDIO as the source for charge pump.
// 3:0	D_PROGRAMMING	Sets the VDDD linear regulator output voltage in 50 mV steps.
//			Must clear the LINREG_SIMPLE_POWERUP and STARTUP_POWERUP bits
//			in the 0x0030 (CHIP_ANA_POWER) register after power-up, for
//			this setting to produce the proper VDDD voltage.
//				0x0 = 1.60
//				0xF = 0.85

#define CHIP_REF_CTRL			0x0028 // bandgap reference bias voltage and currents
// 8:4	VAG_VAL		Analog Ground Voltage Control
//				These bits control the analog ground voltage in 25 mV steps.
//				This should usually be set to VDDA/2 or lower for best
//				performance (maximum output swing at minimum THD). This VAG
//				reference is also used for the DAC and ADC voltage reference.
//				So changing this voltage scales the output swing of the DAC
//				and the output signal of the ADC.
//				0x00 = 0.800 V
//				0x1F = 1.575 V
// 3:1	BIAS_CTRL	Bias control
//				These bits adjust the bias currents for all of the analog
//				blocks. By lowering the bias current a lower quiescent power
//				is achieved. It should be noted that this mode can affect
//				performance by 3-4 dB.
//				0x0 = Nominal
//				0x1-0x3=+12.5%
//				0x4=-12.5%
//				0x5=-25%
//				0x6=-37.5%
//				0x7=-50%
// 0	SMALL_POP	VAG Ramp Control
//				Setting this bit slows down the VAG ramp from ~200 to ~400 ms
//				to reduce the startup pop, but increases the turn on/off time.
//				0x0 = Normal VAG ramp
//				0x1 = Slow down VAG ramp

#define CHIP_MIC_CTRL			0x002A // microphone gain & internal microphone bias
// 9:8	BIAS_RESISTOR	MIC Bias Output Impedance Adjustment
//				Controls an adjustable output impedance for the microphone bias.
//				If this is set to zero the micbias block is powered off and
//				the output is highZ.
//				0x0 = Powered off
//				0x1 = 2.0 kohm
//				0x2 = 4.0 kohm
//				0x3 = 8.0 kohm
// 6:4	BIAS_VOLT	MIC Bias Voltage Adjustment
//				Controls an adjustable bias voltage for the microphone bias
//				amp in 250 mV steps. This bias voltage setting should be no
//				more than VDDA-200 mV for adequate power supply rejection.
//				0x0 = 1.25 V
//				...
//				0x7 = 3.00 V
// 1:0	GAIN		MIC Amplifier Gain
//				Sets the microphone amplifier gain. At 0 dB setting the THD
//				can be slightly higher than other paths- typically around
//				~65 dB. At other gain settings the THD are better.
//				0x0 = 0 dB
//				0x1 = +20 dB
//				0x2 = +30 dB
//				0x3 = +40 dB

#define CHIP_LINE_OUT_CTRL		0x002C
// 11:8	OUT_CURRENT	Controls the output bias current for the LINEOUT amplifiers.  The
//			nominal recommended setting for a 10 kohm load with 1.0 nF load cap
//			is 0x3. There are only 5 valid settings.
//				0x0=0.18 mA
//				0x1=0.27 mA
//				0x3=0.36 mA
//				0x7=0.45 mA
//				0xF=0.54 mA
// 5:0	LO_VAGCNTRL	LINEOUT Amplifier Analog Ground Voltage
//				Controls the analog ground voltage for the LINEOUT amplifiers
//				in 25 mV steps. This should usually be set to VDDIO/2.
//				0x00 = 0.800 V
//				...
//				0x1F = 1.575 V
//				...
//				0x23 = 1.675 V
//				0x24-0x3F are invalid

#define CHIP_LINE_OUT_VOL		0x002E
// 12:8	LO_VOL_RIGHT	LINEOUT Right Channel Volume (default=4)
//				Controls the right channel LINEOUT volume in 0.5 dB steps.
//				Higher codes have more attenuation.
// 4:0	LO_VOL_LEFT	LINEOUT Left Channel Output Level (default=4)
//				Used to normalize the output level of the left line output
//				to full scale based on the values used to set
//				LINE_OUT_CTRL->LO_VAGCNTRL and CHIP_REF_CTRL->VAG_VAL.
//				In general this field should be set to:
//				40*log((VAG_VAL)/(LO_VAGCNTRL)) + 15
//				Suggested values based on typical VDDIO and VDDA voltages.
//					VDDA  VAG_VAL VDDIO  LO_VAGCNTRL LO_VOL_*
//					1.8 V    0.9   3.3 V     1.55      0x06
//					1.8 V    0.9   1.8 V      0.9      0x0F
//					3.3 V   1.55   1.8 V      0.9      0x19
//					3.3 V   1.55   3.3 V     1.55      0x0F
//				After setting to the nominal voltage, this field can be used
//				to adjust the output level in +/-0.5 dB increments by using
//				values higher or lower than the nominal setting.

#define CHIP_ANA_POWER			0x0030 // power down controls for the analog blocks.
		// The only other power-down controls are BIAS_RESISTOR in the MIC_CTRL register
		//  and the EN_ZCD control bits in ANA_CTRL.
// 14	DAC_MONO	While DAC_POWERUP is set, this allows the DAC to be put into left only
//				mono operation for power savings. 0=mono, 1=stereo (default)
// 13	LINREG_SIMPLE_POWERUP  Power up the simple (low power) digital supply regulator.
//				After reset, this bit can be cleared IF VDDD is driven
//				externally OR the primary digital linreg is enabled with
//				LINREG_D_POWERUP
// 12	STARTUP_POWERUP	Power up the circuitry needed during the power up ramp and reset.
//				After reset this bit can be cleared if VDDD is coming from
//				an external source.
// 11	VDDC_CHRGPMP_POWERUP Power up the VDDC charge pump block. If neither VDDA or VDDIO
//				is 3.0 V or larger this bit should be cleared before analog
//				blocks are powered up.
// 10	PLL_POWERUP	PLL Power Up, 0 = Power down, 1 = Power up
//				When cleared, the PLL is turned off. This must be set before
//				CHIP_CLK_CTRL->MCLK_FREQ is programmed to 0x3. The
//				CHIP_PLL_CTRL register must be configured correctly before
//				setting this bit.
// 9	LINREG_D_POWERUP Power up the primary VDDD linear regulator, 0 = Power down, 1 = Power up
// 8	VCOAMP_POWERUP	Power up the PLL VCO amplifier, 0 = Power down, 1 = Power up
// 7	VAG_POWERUP	Power up the VAG reference buffer.
//				Setting this bit starts the power up ramp for the headphone
//				and LINEOUT. The headphone (and/or LINEOUT) powerup should
//				be set BEFORE clearing this bit. When this bit is cleared
//				the power-down ramp is started. The headphone (and/or LINEOUT)
//				powerup should stay set until the VAG is fully ramped down
//				(200 to 400 ms after clearing this bit).
//				0x0 = Power down, 0x1 = Power up
// 6	ADC_MONO	While ADC_POWERUP is set, this allows the ADC to be put into left only
//				mono operation for power savings. This mode is useful when
//				only using the microphone input.
//				0x0 = Mono (left only), 0x1 = Stereo
// 5	REFTOP_POWERUP	Power up the reference bias currents
//				0x0 = Power down, 0x1 = Power up
//				This bit can be cleared when the part is a sleep state
//				to minimize analog power.
// 4	HEADPHONE_POWERUP Power up the headphone amplifiers
//				0x0 = Power down, 0x1 = Power up
// 3	DAC_POWERUP	Power up the DACs
//				0x0 = Power down, 0x1 = Power up
// 2	CAPLESS_HEADPHONE_POWERUP Power up the capless headphone mode
//				0x0 = Power down, 0x1 = Power up
// 1	ADC_POWERUP	Power up the ADCs
//				0x0 = Power down, 0x1 = Power up
// 0	LINEOUT_POWERUP	Power up the LINEOUT amplifiers
//				0x0 = Power down, 0x1 = Power up

#define CHIP_PLL_CTRL			0x0032
// 15:11 INT_DIVISOR
// 10:0 FRAC_DIVISOR

#define CHIP_CLK_TOP_CTRL		0x0034
// 11	ENABLE_INT_OSC	Setting this bit enables an internal oscillator to be used for the
//				zero cross detectors, the short detect recovery, and the
//				charge pump. This allows the I2S clock to be shut off while
//				still operating an analog signal path. This bit can be kept
//				on when the I2S clock is enabled, but the I2S clock is more
//				accurate so it is preferred to clear this bit when I2S is present.
// 3	INPUT_FREQ_DIV2	SYS_MCLK divider before PLL input
//				0x0 = pass through
//				0x1 = SYS_MCLK is divided by 2 before entering PLL
//				This must be set when the input clock is above 17 Mhz. This
//				has no effect when the PLL is powered down.

#define CHIP_ANA_STATUS			0x0036
// 9	LRSHORT_STS	This bit is high whenever a short is detected on the left or right
//				channel headphone drivers.
// 8	CSHORT_STS	This bit is high whenever a short is detected on the capless headphone
//				common/center channel driver.
// 4	PLL_IS_LOCKED	This bit goes high after the PLL is locked.

#define CHIP_ANA_TEST1			0x0038 //  intended only for debug.
#define CHIP_ANA_TEST2			0x003A //  intended only for debug.

#define CHIP_SHORT_CTRL			0x003C
// 14:12 LVLADJR	Right channel headphone	short detector in 25 mA steps.
//				0x3=25 mA
//				0x2=50 mA
//				0x1=75 mA
//				0x0=100 mA
//				0x4=125 mA
//				0x5=150 mA
//				0x6=175 mA
//				0x7=200 mA
//				This trip point can vary by ~30% over process so leave plenty
//				of guard band to avoid false trips.  This short detect trip
//				point is also effected by the bias current adjustments made
//				by CHIP_REF_CTRL->BIAS_CTRL and by CHIP_ANA_TEST1->HP_IALL_ADJ.
// 10:8	LVLADJL		Left channel headphone short detector in 25 mA steps.
//				(same scale as LVLADJR)
// 6:4	LVLADJC		Capless headphone center channel short detector in 50 mA steps.
//				0x3=50 mA
//				0x2=100 mA
//				0x1=150 mA
//				0x0=200 mA
//				0x4=250 mA
//				0x5=300 mA
//				0x6=350 mA
//				0x7=400 mA
// 3:2	MODE_LR		Behavior of left/right short detection
//				0x0 = Disable short detector, reset short detect latch,
//					software view non-latched short signal
//				0x1 = Enable short detector and reset the latch at timeout
//					(every ~50 ms)
//				0x2 = This mode is not used/invalid
//				0x3 = Enable short detector with only manual reset (have
//					to return to 0x0 to reset the latch)
// 1:0	MODE_CM		Behavior of capless headphone central short detection
//				(same settings as MODE_LR)

#define DAP_CONTROL			0x0100
#define DAP_PEQ				0x0102
#define DAP_BASS_ENHANCE		0x0104
#define DAP_BASS_ENHANCE_CTRL		0x0106
#define DAP_AUDIO_EQ			0x0108
#define DAP_SGTL_SURROUND		0x010A
#define DAP_FILTER_COEF_ACCES		0x010C
#define DAP_COEF_WR_B0_MSB		0x010E
#define DAP_COEF_WR_B0_LSB		0x0110
#define DAP_AUDIO_EQ_BASS_BAND0		0x0116 // 115 Hz
#define DAP_AUDIO_EQ_BAND1		0x0118 // 330 Hz
#define DAP_AUDIO_EQ_BAND2		0x011A // 990 Hz
#define DAP_AUDIO_EQ_BAND3		0x011C // 3000 Hz
#define DAP_AUDIO_EQ_TREBLE_BAND4	0x011E // 9900 Hz
#define DAP_MAIN_CHAN			0x0120
#define DAP_MIX_CHAN			0x0122
#define DAP_AVC_CTRL			0x0124
#define DAP_AVC_THRESHOLD		0x0126
#define DAP_AVC_ATTACK			0x0128
#define DAP_AVC_DECAY			0x012A
#define DAP_COEF_WR_B1_MSB		0x012C
#define DAP_COEF_WR_B1_LSB		0x012E
#define DAP_COEF_WR_B2_MSB		0x0130
#define DAP_COEF_WR_B2_LSB		0x0132
#define DAP_COEF_WR_A1_MSB		0x0134
#define DAP_COEF_WR_A1_LSB		0x0136
#define DAP_COEF_WR_A2_MSB		0x0138
#define DAP_COEF_WR_A2_LSB		0x013A

#define SGTL5000_I2C_ADDR  0x0A  // CTRL_ADR0_CS pin low (normal configuration)
//#define SGTL5000_I2C_ADDR  0x2A // CTRL_ADR0_CS  pin high



bool AudioControlSGTL5000::enable(void)
{
	unsigned int n;

	muted = true;
	Wire.begin();
	delay(5);
	Serial.print("chip ID = ");
	delay(5);
	n = read(CHIP_ID);
	Serial.println(n, HEX);

	write(CHIP_ANA_POWER, 0x4060);  // VDDD is externally driven with 1.8V
	write(CHIP_LINREG_CTRL, 0x006C);  // VDDA & VDDIO both over 3.1V
	write(CHIP_REF_CTRL, 0x01F1); // VAG=1.575 slow ramp, normal bias current
	write(CHIP_LINE_OUT_CTRL, 0x0322);
	write(CHIP_SHORT_CTRL, 0x4446);  // allow up to 125mA
	write(CHIP_ANA_CTRL, 0x0137);  // enable zero cross detectors
	write(CHIP_ANA_POWER, 0x40FF); // power up: lineout, hp, adc, dac
	write(CHIP_DIG_POWER, 0x0073); // power up all digital stuff
	delay(400);
	write(CHIP_LINE_OUT_VOL, 0x0505); // TODO: correct value for 3.3V
	write(CHIP_CLK_CTRL, 0x0004);  // 44.1 kHz, 256*Fs
	write(CHIP_I2S_CTRL, 0x0130); // SCLK=32*Fs, 16bit, I2S format
	// default signal routing is ok?
	write(CHIP_SSS_CTRL, 0x0010); // ADC->I2S, I2S->DAC
	write(CHIP_ADCDAC_CTRL, 0x0000); // disable dac mute
	write(CHIP_DAC_VOL, 0x3C3C); // digital gain, 0dB
	write(CHIP_ANA_HP_CTRL, 0x7F7F); // set volume (lowest level)
	write(CHIP_ANA_CTRL, 0x0136);  // enable zero cross detectors
	//mute = false;
	return true;
}

unsigned int AudioControlSGTL5000::read(unsigned int reg)
{
	unsigned int val;

	Wire.beginTransmission(SGTL5000_I2C_ADDR);
	Wire.write(reg >> 8);
	Wire.write(reg);
	if (Wire.endTransmission(false) != 0) return 0;
	if (Wire.requestFrom(SGTL5000_I2C_ADDR, 2) < 2) return 0;
	val = Wire.read() << 8;
	val |= Wire.read();
	return val;
}

bool AudioControlSGTL5000::write(unsigned int reg, unsigned int val)
{
	if (reg == CHIP_ANA_CTRL) ana_ctrl = val;
	Wire.beginTransmission(SGTL5000_I2C_ADDR);
	Wire.write(reg >> 8);
	Wire.write(reg);
	Wire.write(val >> 8);
	Wire.write(val);
	if (Wire.endTransmission() == 0) return true;
	return false;
}

bool AudioControlSGTL5000::volumeInteger(unsigned int n)
{
	if (n == 0) {
		muted = true;
		write(CHIP_ANA_HP_CTRL, 0x7F7F);
		return muteHeadphone();
	} else if (n > 0x80) {
		n = 0;
	} else {
		n = 0x80 - n;
	}
	if (muted) {
		muted = false;
		unmuteHeadphone();
	}
	n = n | (n << 8);
	return write(CHIP_ANA_HP_CTRL, n);  // set volume
}




