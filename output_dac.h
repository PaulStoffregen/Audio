#ifndef output_dac_h_
#define output_dac_h_

#include "AudioStream.h"

class AudioOutputAnalog : public AudioStream
{
public:
	AudioOutputAnalog(void) : AudioStream(1, inputQueueArray) { begin(); }
	virtual void update(void);
	void begin(void);
	void analogReference(int ref);
	friend void dma_ch4_isr(void);
private:
	static audio_block_t *block_left_1st;
	static audio_block_t *block_left_2nd;
	static bool update_responsibility;
	audio_block_t *inputQueueArray[1];
};

#endif
