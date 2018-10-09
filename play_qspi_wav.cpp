#include "play_qspi_wav.h"

#define STATE_DIRECT_8BIT_MONO		0  // playing mono at native sample rate
#define STATE_DIRECT_8BIT_STEREO	1  // playing stereo at native sample rate
#define STATE_DIRECT_16BIT_MONO		2  // playing mono at native sample rate
#define STATE_DIRECT_16BIT_STEREO	3  // playing stereo at native sample rate
#define STATE_CONVERT_8BIT_MONO		4  // playing mono, converting sample rate
#define STATE_CONVERT_8BIT_STEREO	5  // playing stereo, converting sample rate
#define STATE_CONVERT_16BIT_MONO	6  // playing mono, converting sample rate
#define STATE_CONVERT_16BIT_STEREO	7  // playing stereo, converting sample rate
#define STATE_PARSE1			8  // looking for 20 byte ID header
#define STATE_PARSE2			9  // looking for 16 byte format header
#define STATE_PARSE3			10 // looking for 8 byte data header
#define STATE_PARSE4			11 // ignoring unknown chunk
#define STATE_STOP			12


bool AudioPlayQspiWav::play(const char *filename)
{
    stop();
	__disable_irq();
	wavfile = _fs->open(filename);
	__enable_irq();
	if (!wavfile) {			
		return false;
	}
	buffer_length = 0;
	buffer_offset = 0;
	state_play = STATE_STOP;
	data_length = 20;
	header_offset = 0;
	state = STATE_PARSE1;
	return true;
}

void AudioPlayQspiWav::stop(void)
{
	__disable_irq();
	if (state != STATE_STOP) {
		audio_block_t *b1 = block_left;
		block_left = NULL;
		audio_block_t *b2 = block_right;
		block_right = NULL;
		state = STATE_STOP;
		__enable_irq();
		if (b1) release(b1);
		if (b2) release(b2);
		wavfile.close();
	} else {
		__enable_irq();
	}
}


void AudioPlayQspiWav::update(void)
{
	int32_t n;

	// only update if we're playing
	if (state == STATE_STOP) return;

	// allocate the audio blocks to transmit
	block_left = allocate();
	if (block_left == NULL) return;
	if (state < 8 && (state & 1) == 1) {
		// if we're playing stereo, allocate another
		// block for the right channel output
		block_right = allocate();
		if (block_right == NULL) {
			release(block_left);
			return;
		}
	} else {
		// if we're playing mono or just parsing
		// the WAV file header, no right-side block
		block_right = NULL;
	}
	block_offset = 0;

	//Serial.println("update");

	// is there buffered data?
	n = buffer_length - buffer_offset;
	if (n > 0) {
		// we have buffered data
		if (consume(n)) return; // it was enough to transmit audio
	}

	// we only get to this point when buffer[512] is empty
	if (state != STATE_STOP && wavfile.available()) {
		// we can read more data from the file...
		readagain:
		buffer_length = wavfile.read(buffer, 512);
		if (buffer_length == 0) goto end;
		buffer_offset = 0;
		bool parsing = (state >= 8);
		bool txok = consume(buffer_length);
		if (txok) {
			if (state != STATE_STOP) return;
		} else {
			if (state != STATE_STOP) {
				if (parsing && state < 8) goto readagain;
				else goto cleanup;
			}
		}
	}
end:	// end of file reached or other reason to stop
	wavfile.close();
	state_play = STATE_STOP;
	state = STATE_STOP;
cleanup:
	if (block_left) {
		if (block_offset > 0) {
			for (uint32_t i=block_offset; i < AUDIO_BLOCK_SAMPLES; i++) {
				block_left->data[i] = 0;
			}
			transmit(block_left, 0);
			if (state < 8 && (state & 1) == 0) {
				transmit(block_left, 1);
			}
		}
		release(block_left);
		block_left = NULL;
	}
	if (block_right) {
		if (block_offset > 0) {
			for (uint32_t i=block_offset; i < AUDIO_BLOCK_SAMPLES; i++) {
				block_right->data[i] = 0;
			}
			transmit(block_right, 1);
		}
		release(block_right);
		block_right = NULL;
	}
}