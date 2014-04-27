#include "Audio.h"
#include "arm_math.h"

// data_waveforms.c
extern "C" {
extern const int16_t AudioWaveformSine[257];
}



void AudioSynthWaveformSine::frequency(float f)
{
	if (f > AUDIO_SAMPLE_RATE_EXACT / 2 || f < 0.0) return;
	phase_increment = (f / AUDIO_SAMPLE_RATE_EXACT) * 4294967296.0f;
}

void AudioSynthWaveformSine::update(void)
{
	audio_block_t *block;
	uint32_t i, ph, inc, index, scale;
	int32_t val1, val2;

	//Serial.println("AudioSynthWaveformSine::update");
	block = allocate();
	if (block) {
		ph = phase;
		inc = phase_increment;
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			index = ph >> 24;
			val1 = AudioWaveformSine[index];
			val2 = AudioWaveformSine[index+1];
			scale = (ph >> 8) & 0xFFFF;
			val2 *= scale;
			val1 *= 0xFFFF - scale;
			block->data[i] = (val1 + val2) >> 16;
			 //Serial.print(block->data[i]);
			 //Serial.print(", ");
			 //if ((i % 12) == 11) Serial.println();
			ph += inc;
		}
		 //Serial.println();
		phase = ph;
		transmit(block);
		release(block);
		return;
	}
	phase += phase_increment * AUDIO_BLOCK_SAMPLES;
}




void AudioSynthWaveformSineModulated::frequency(float f)
{
	if (f > AUDIO_SAMPLE_RATE_EXACT / 2 || f < 0.0) return;
	phase_increment = (f / AUDIO_SAMPLE_RATE_EXACT) * 4294967296.0f;
}

void AudioSynthWaveformSineModulated::update(void)
{
	audio_block_t *block, *modinput;
	uint32_t i, ph, inc, index, scale;
	int32_t val1, val2;

	//Serial.println("AudioSynthWaveformSineModulated::update");
	modinput = receiveReadOnly();
	ph = phase;
	inc = phase_increment;
	block = allocate();
	if (!block) {
		// unable to allocate memory, so we'll send nothing
		if (modinput) {
			// but if we got modulation data, update the phase
			for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
				ph += inc + modinput->data[i] * modulation_factor;
			}
			release(modinput);
		} else {
			ph += phase_increment * AUDIO_BLOCK_SAMPLES;
		}
		phase = ph;
		return;
	}
	if (modinput) {
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			index = ph >> 24;
			val1 = AudioWaveformSine[index];
			val2 = AudioWaveformSine[index+1];
			scale = (ph >> 8) & 0xFFFF;
			val2 *= scale;
			val1 *= 0xFFFF - scale;
			block->data[i] = (val1 + val2) >> 16;
			 //Serial.print(block->data[i]);
			 //Serial.print(", ");
			 //if ((i % 12) == 11) Serial.println();
			ph += inc + modinput->data[i] * modulation_factor;
		}
		release(modinput);
	} else {
		ph = phase;
		inc = phase_increment;
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			index = ph >> 24;
			val1 = AudioWaveformSine[index];
			val2 = AudioWaveformSine[index+1];
			scale = (ph >> 8) & 0xFFFF;
			val2 *= scale;
			val1 *= 0xFFFF - scale;
			block->data[i] = (val1 + val2) >> 16;
			 //Serial.print(block->data[i]);
			 //Serial.print(", ");
			 //if ((i % 12) == 11) Serial.println();
			ph += inc;
		}
	}
	phase = ph;
	transmit(block);
	release(block);
}





