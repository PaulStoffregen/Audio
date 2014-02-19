#ifndef play_sd_wav_h_
#define play_sd_wav_h_

#include "AudioStream.h"
#include "SD.h"

class AudioPlaySDcardWAV : public AudioStream
{
public:
	AudioPlaySDcardWAV(void) : AudioStream(0, NULL) { begin(); }
	void begin(void);

	bool play(const char *filename);
	void stop(void);
	bool start(void);
	virtual void update(void);
private:
	File wavfile;
	bool consume(void);
	bool parse_format(void);
	uint32_t header[5];
	uint32_t data_length;		// number of bytes remaining in data section
	audio_block_t *block_left;
	audio_block_t *block_right;
	uint16_t block_offset;
	uint8_t buffer[512];
	uint16_t buffer_remaining;
	uint8_t state;
	uint8_t state_play;
	uint8_t leftover_bytes;
};

#endif
