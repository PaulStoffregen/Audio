#include "effect_fade.h"
#include "utility/dspinst.h"

extern "C" {
extern const int16_t fader_table[256];
};

void AudioEffectFade::update(void)
{
	audio_block_t *block;
	uint32_t i, pos, inc, index, scale;
	int32_t val1, val2, val, sample;
	uint8_t dir;

	pos = position;
	if (pos == 0) {
		// output is silent
		block = receiveReadOnly();
		if (block) release(block);
		return;
	} else if (pos == 0xFFFFFFFF) {
		// output is 100%
		block = receiveReadOnly();
		if (!block) return;
		transmit(block);
		release(block);
		return;
	}
	block = receiveWritable();
	if (!block) return;
	inc = rate;
	dir = direction;
	for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
		index = pos >> 24;
		val1 = fader_table[index];
		val2 = fader_table[index+1];
		scale = (pos >> 8) & 0xFFFF;
		val2 *= scale;
		val1 *= 0x10000 - scale;
		val = (val1 + val2) >> 16;
		sample = block->data[i];
		sample = (sample * val) >> 15;
		block->data[i] = sample;
		if (dir > 0) {
			// output is increasing
			if (inc < 0xFFFFFFFF - pos) pos += inc;
			else pos = 0xFFFFFFFF;
		} else {
			// output is decreasing
			if (inc < pos) pos -= inc;
			else pos = 0;
		}
	}
	position = pos;
	transmit(block);
	release(block);
}

void AudioEffectFade::fadeBegin(uint32_t newrate, uint8_t dir)
{
	__disable_irq();
	uint32_t pos = position;
	if (pos == 0) position = 1;
	else if (pos == 0xFFFFFFFF) position = 0xFFFFFFFE;
	rate = newrate;
	direction = dir;
	__enable_irq();
}



