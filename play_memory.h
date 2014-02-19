#ifndef play_memory_h_
#define play_memory_h_

#include "AudioStream.h"

class AudioPlayMemory : public AudioStream
{
public:
	AudioPlayMemory(void) : AudioStream(0, NULL), playing(0) { }
	void play(const unsigned int *data);
	void stop(void);
	virtual void update(void);
private:
	const unsigned int *next;
	uint32_t length;
	int16_t prior;
	volatile uint8_t playing;
};

#endif
