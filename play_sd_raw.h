#ifndef play_sd_raw_h_
#define play_sd_raw_h_

#include "AudioStream.h"
#include "SD.h"

class AudioPlaySDcardRAW : public AudioStream
{
public:
	AudioPlaySDcardRAW(void) : AudioStream(0, NULL) { begin(); }
	void begin(void);
	bool play(const char *filename);
	void stop(void);
	virtual void update(void);
private:
	File rawfile;
	audio_block_t *block;
	bool playing;
	bool paused;
};

#endif
