#ifndef input_i2s_h_
#define _input_i2sh_

#include "AudioStream.h"

class AudioInputI2S : public AudioStream
{
public:
	AudioInputI2S(void) : AudioStream(0, NULL) { begin(); }
	virtual void update(void);
	void begin(void);
	friend void dma_ch1_isr(void);
protected:	
	AudioInputI2S(int dummy): AudioStream(0, NULL) {} // to be used only inside AudioInputI2Sslave !!
	static bool update_responsibility;
private:
	static audio_block_t *block_left;
	static audio_block_t *block_right;
	static uint16_t block_offset;
};


class AudioInputI2Sslave : public AudioInputI2S
{
public:
	AudioInputI2Sslave(void) : AudioInputI2S(0) { begin(); }
	void begin(void);
	friend void dma_ch1_isr(void);
};

#endif
