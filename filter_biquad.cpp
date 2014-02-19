#include "Audio.h"
#include "arm_math.h"
#include "utility/dspinst.h"


void AudioFilterBiquad::update(void)
{
	audio_block_t *block;
	int32_t a0, a1, a2, b1, b2, sum;
	uint32_t in2, out2, aprev, bprev, flag;
	uint32_t *data, *end;
	int32_t *state;

	block = receiveWritable();
	if (!block) return;
	data = (uint32_t *)(block->data);
	end = data + AUDIO_BLOCK_SAMPLES/2;
	state = (int32_t *)definition;
	do {
		a0 = *state++;
		a1 = *state++;
		a2 = *state++;
		b1 = *state++;
		b2 = *state++;
		aprev = *state++;
		bprev = *state++;
		sum = *state & 0x3FFF;
		do {
			in2 = *data;
			sum = signed_multiply_accumulate_32x16b(sum, a0, in2);
			sum = signed_multiply_accumulate_32x16t(sum, a1, aprev);
			sum = signed_multiply_accumulate_32x16b(sum, a2, aprev);
			sum = signed_multiply_accumulate_32x16t(sum, b1, bprev);
			sum = signed_multiply_accumulate_32x16b(sum, b2, bprev);
			out2 = (uint32_t)sum >> 14;
			sum &= 0x3FFF;
			sum = signed_multiply_accumulate_32x16t(sum, a0, in2);
			sum = signed_multiply_accumulate_32x16b(sum, a1, in2);
			sum = signed_multiply_accumulate_32x16t(sum, a2, aprev);
			sum = signed_multiply_accumulate_32x16b(sum, b1, out2);
			sum = signed_multiply_accumulate_32x16t(sum, b2, bprev);
			aprev = in2;
			bprev = pack_16x16(sum >> 14, out2);
			sum &= 0x3FFF;
			aprev = in2;
			*data++ = bprev;
		} while (data < end);
		flag = *state & 0x80000000;
		*state++ = sum | flag;
		*(state-2) = bprev;
		*(state-3) = aprev;
	} while (flag);
	transmit(block);
	release(block);
}

void AudioFilterBiquad::updateCoefs(int *source, bool doReset)
{
	int32_t *dest=(int32_t *)definition;
	int32_t *src=(int32_t *)source;
	__disable_irq();
	for(uint8_t index=0;index<5;index++)
	{
		*dest++=*src++;
	}
	if(doReset)
	{
		*dest++=0;
		*dest++=0;
		*dest++=0;
	}
	__enable_irq();
}

void AudioFilterBiquad::updateCoefs(int *source)
{
	updateCoefs(source,false);
}

