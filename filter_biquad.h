#ifndef filter_biquad_h_
#define filter_biquad_h_

#include "AudioStream.h"

class AudioFilterBiquad : public AudioStream
{
public:
	AudioFilterBiquad(int *parameters)
	   : AudioStream(1, inputQueueArray), definition(parameters) { }
	virtual void update(void);
	
	void updateCoefs(int *source, bool doReset);
	void updateCoefs(int *source);
private:
	int *definition;
	audio_block_t *inputQueueArray[1];
};

#endif
