//
// Created by Nicholas Newdigate on 10/02/2019.
//

#ifndef TEENSYAUDIOLIBRARY_PLAY_SD_RAW_RESAMPLED_H
#define TEENSYAUDIOLIBRARY_PLAY_SD_RAW_RESAMPLED_H
#include "Arduino.h"
#include "AudioStream.h"
#include "SD.h"
#include "stdint.h"
#include "utility/ResamplingSdReader.h"

class AudioPlaySdRawResampled : public AudioStream
{
public:
    AudioPlaySdRawResampled(void) :
            AudioStream(0, NULL),
            sdReader()
    {
        begin();
    }

    void begin(void);
    bool play(const char *filename);
    void stop(void);
    bool isPlaying(void) { return playing; }
    uint32_t positionMillis(void);
    uint32_t lengthMillis(void);
    virtual void update(void);

    void setPlaybackRate(float f) {
        sdReader.setReadRate(f);
    }

private:

    uint32_t file_size;
    volatile uint32_t file_offset;
    volatile bool playing;
    ResamplingSdReader sdReader;
};

#endif //TEENSYAUDIOLIBRARY_PLAY_SD_RAW_RESAMPLED_H
