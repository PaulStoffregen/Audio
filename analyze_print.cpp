#include "analyze_print.h"

// TODO: this needs some sort of trigger or delay or other options to make
// actually useful for watching interesting parts of data, without spewing
// tremendous and endless data to the Arduino Serial Monitor

void AudioPrint::update(void)
{
	audio_block_t *block;
	uint32_t i;

	Serial.println("AudioPrint::update");
	Serial.println(name);
	block = receiveReadOnly();
	if (block) {
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			Serial.print(block->data[i]);
			Serial.print(", ");
			if ((i % 12) == 11) Serial.println();
		}
		Serial.println();
		release(block);
	}
}

