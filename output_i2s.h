#ifndef output_i2s_h_
#define output_i2s_h_

#include "AudioStream.h"

class AudioOutputI2S : public AudioStream
{
public:
	AudioOutputI2S(void) : AudioStream(2, inputQueueArray) { begin(); }
	virtual void update(void);
	void begin(void);
	friend void dma_ch0_isr(void);
	friend class AudioInputI2S;
protected:
	AudioOutputI2S(int dummy): AudioStream(2, inputQueueArray) {} // to be used only inside AudioOutputI2Sslave !!
	static void config_i2s(void);
	static audio_block_t *block_left_1st;
	static audio_block_t *block_right_1st;
	static bool update_responsibility;
private:
	static audio_block_t *block_left_2nd;
	static audio_block_t *block_right_2nd;
	static uint16_t block_left_offset;
	static uint16_t block_right_offset;
	audio_block_t *inputQueueArray[2];
};


class AudioOutputI2Sslave : public AudioOutputI2S
{
public:
	AudioOutputI2Sslave(void) : AudioOutputI2S(0) { begin(); } ;
	void begin(void);
	friend class AudioInputI2Sslave;
	friend void dma_ch0_isr(void);
protected:
	static void config_i2s(void);
};

#endif
