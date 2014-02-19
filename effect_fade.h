#ifndef effect_fade_h_
#define effect_fade_h_

#include "AudioStream.h"

class AudioEffectFade : public AudioStream
{
public:
	AudioEffectFade(void)
	  : AudioStream(1, inputQueueArray), position(0xFFFFFFFF) {}
	void fadeIn(uint32_t milliseconds) {
		uint32_t samples = (uint32_t)(milliseconds * 441u + 5u) / 10u;
		//Serial.printf("fadeIn, %u samples\n", samples);
		fadeBegin(0xFFFFFFFFu / samples, 1);
	}
	void fadeOut(uint32_t milliseconds) {
		uint32_t samples = (uint32_t)(milliseconds * 441u + 5u) / 10u;
		//Serial.printf("fadeOut, %u samples\n", samples);
		fadeBegin(0xFFFFFFFFu / samples, 0);
	}
	virtual void update(void);
private:
	void fadeBegin(uint32_t newrate, uint8_t dir);
	uint32_t position; // 0 = off, 0xFFFFFFFF = on
	uint32_t rate;
	uint8_t direction; // 0 = fading out, 1 = fading in
	audio_block_t *inputQueueArray[1];
};

#endif
