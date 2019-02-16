//
// Created by Nicholas Newdigate on 10/02/2019.
//

#ifndef TEENSYAUDIOLIBRARY_RESAMPLINGSDREADER_H
#define TEENSYAUDIOLIBRARY_RESAMPLINGSDREADER_H

#include "stdint.h"
#include <SD.h>
#include <AudioStream.h>

#include "../spi_interrupt.h"

class ResamplingSdReader {
public:
    ResamplingSdReader() {

    }
    void begin(void);
    bool play(const char *filename);
    void stop(void);
    bool isPlaying(void) { return _playing; }

    int read(void *buf, uint16_t nbyte);
    bool readNextValue(int16_t *value);

    void setReadRate(float f) {
        _readRate = f;
    }
    int available(void);
    void close(void);

private:
    volatile bool _playing;
    volatile int32_t _file_offset;

    uint32_t _file_size;
    float _readRate = 0.5;
    float _remainder = 0;

    int _bufferPosition = 0;
    int _bufferLength = 0;
    uint16_t _buffer[AUDIO_BLOCK_SAMPLES];

    File _file;
};



#endif //PAULSTOFFREGEN_RESAMPLINGSDREADER_H
