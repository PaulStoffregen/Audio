#ifndef output_pwm_h_
#define output_pwm_h_

#include "AudioStream.h"

class AudioOutputPWM : public AudioStream
{
public:
	AudioOutputPWM(void) : AudioStream(1, inputQueueArray) { begin(); }
	virtual void update(void);
	void begin(void);
	friend void dma_ch3_isr(void);
private:
	static audio_block_t *block_1st;
	static audio_block_t *block_2nd;
	static uint32_t block_offset;
	static bool update_responsibility;
	static uint8_t interrupt_count;
	audio_block_t *inputQueueArray[1];
};

#endif
