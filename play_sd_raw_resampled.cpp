//
// Created by Nicholas Newdigate on 10/02/2019.
//

#include "play_sd_raw_resampled.h"
#include "spi_interrupt.h"

void AudioPlaySdRawResampled::begin()
{
    playing = false;
    file_offset = 0;
    file_size = 0;
}

bool AudioPlaySdRawResampled::play(const char *filename)
{
    stop();
    playing = sdReader.play(filename);
    return playing;
}

void AudioPlaySdRawResampled::stop()
{
    sdReader.stop();
}

void AudioPlaySdRawResampled::update()
{
    unsigned int i, n;
    audio_block_t *block;

    // only update if we're playing
    if (!playing) return;

    // allocate the audio blocks to transmit
    block = allocate();
    if (block == NULL) return;

    if (sdReader.available()) {
        // we can read more data from the file...
        n = sdReader.read(block->data, AUDIO_BLOCK_SAMPLES*2);
        file_offset += n;
        for (i=n/2; i < AUDIO_BLOCK_SAMPLES; i++) {
            block->data[i] = 0;
        }
        transmit(block);
    } else {
        sdReader.close();
#if defined(HAS_KINETIS_SDHC)
        if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStopUsingSPI();
#else
        AudioStopUsingSPI();
#endif
        playing = false;
    }
    release(block);
}

#define B2M (uint32_t)((double)4294967296000.0 / AUDIO_SAMPLE_RATE_EXACT / 2.0) // 97352592

uint32_t AudioPlaySdRawResampled::positionMillis()
{
    return ((uint64_t)file_offset * B2M) >> 32;
}

uint32_t AudioPlaySdRawResampled::lengthMillis()
{
    return ((uint64_t)file_size * B2M) >> 32;
}
