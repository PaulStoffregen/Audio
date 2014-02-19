#include "Audio.h"
#include "arm_math.h"
#include "utility/dspinst.h"



void AudioPlaySDcardRAW::begin(void)
{
	playing = false;
	if (block) {
		release(block);
		block = NULL;
	}
}


bool AudioPlaySDcardRAW::play(const char *filename)
{
	stop();
	rawfile = SD.open(filename);
	if (!rawfile) {
		Serial.println("unable to open file");
		return false;
	}
	Serial.println("able to open file");
	playing = true;
	return true;
}

void AudioPlaySDcardRAW::stop(void)
{
	__disable_irq();
	if (playing) {
		playing = false;
		__enable_irq();
		rawfile.close();
	} else {
		__enable_irq();
	}
}


void AudioPlaySDcardRAW::update(void)
{
	unsigned int i, n;

	// only update if we're playing
	if (!playing) return;

	// allocate the audio blocks to transmit
	block = allocate();
	if (block == NULL) return;

	if (rawfile.available()) {
		// we can read more data from the file...
		n = rawfile.read(block->data, AUDIO_BLOCK_SAMPLES*2);
		for (i=n/2; i < AUDIO_BLOCK_SAMPLES; i++) {
			block->data[i] = 0;
		}
		transmit(block);
		release(block);
	} else {
		rawfile.close();
		playing = false;
	}
}


