#ifndef analyze_print_h_
#define analyze_print_h_

#include "AudioStream.h"

class AudioPrint : public AudioStream
{
public:
	AudioPrint(const char *str) : AudioStream(1, inputQueueArray), name(str) {}
	virtual void update(void);
private:
	const char *name;
	audio_block_t *inputQueueArray[1];
};

#endif
