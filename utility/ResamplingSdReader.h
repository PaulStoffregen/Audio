/* Audio Library for Teensy 3.X
 * Copyright (c) 2019, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef TEENSYAUDIOLIBRARY_RESAMPLINGSDREADER_H
#define TEENSYAUDIOLIBRARY_RESAMPLINGSDREADER_H

#include "stdint.h"
#include <SD.h>
#include <AudioStream.h>

#include "../spi_interrupt.h"

const unsigned int ResamplingSdReader_NUM_BUFFERS = 2;

class ResamplingSdReader {
public:
    ResamplingSdReader() {

        for (int i=0; i<ResamplingSdReader_NUM_BUFFERS; i++ ) {
            _buffers[i] = NULL;
        }
    }
    void begin(void);
    bool play(const char *filename);
    void stop(void);
    bool isPlaying(void) { return _playing; }

    int read(void *buf, uint16_t nbyte);
    bool readNextValue(int16_t *value);

    void setPlaybackRate(float f) {
        _playbackRate = f;
    }

    float playbackRate() {
        return _playbackRate;
    }

    int available(void);

    void close(void);

    bool interpolationEnabled() {
        return _enable_interpolation;
    }

    void setInterpolationEnabled(bool enableInterpolation) {
        _enable_interpolation = enableInterpolation;
    }

    void updateBuffers();

private:
    volatile bool _playing = false;
    volatile int32_t _file_offset;

    bool _enable_interpolation = true;
    uint32_t _file_size;
    float _playbackRate = 1;
    float _remainder = 0;

    int _bufferPosition = 0;

    int16_t *_buffers[AUDIO_BLOCK_SAMPLES];
    unsigned int _bufferLength[ResamplingSdReader_NUM_BUFFERS];
    bool bufferIsAvailableForRead[ResamplingSdReader_NUM_BUFFERS];

    char _readBuffer = -1;
    char _playBuffer = -1;

    File _file;

    void StartUsingSPI(){
#if defined(HAS_KINETIS_SDHC)
        if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStartUsingSPI();
#else
        AudioStartUsingSPI();
#endif
    }

    void StopUsingSPI() {
#if defined(HAS_KINETIS_SDHC)
        if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStopUsingSPI();
#else
        AudioStopUsingSPI();
#endif
    }
};

#endif //TEENSYAUDIOLIBRARY_RESAMPLINGSDREADER_H
