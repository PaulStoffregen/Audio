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
#include "ResamplingSdReader.h"

int ResamplingSdReader::read(void *buf, uint16_t nbyte) {

    unsigned int count = 0;
    int16_t *index = (int16_t*)buf;
    for (int i=0; i< nbyte/2; i++) {

        if (readNextValue(index))
            count+=2;
        else
            return count;

        index++;
    }
    return count;
}

bool ResamplingSdReader::readNextValue(int16_t *value) {
    if (_readRate > 0 ) {
        //forward playback

        if (_bufferPosition < 0) // occurs when direction has switched
            _bufferPosition = 0;

        if ( _bufferLength == 0 || _bufferPosition >= _bufferLength) {

            //      if (_file.available() == 0) {
            //               _file.seek(0);
            //        }
            //Serial.printf("read %d bytes\n", AUDIO_BLOCK_SAMPLES * 2);
            int numRead = _file.read(_buffer, AUDIO_BLOCK_SAMPLES * 2);
            if (numRead == 0)
                return false;
            _file_offset = _file.position();
            _bufferLength = numRead;
            _bufferPosition = 0;

        }
    } else if (_readRate < 0) {

        // reverse playback
        if (_bufferPosition < 0) {

            if (_file_offset < 0)
                return false;

            int numRead = _file.read(_buffer, AUDIO_BLOCK_SAMPLES * 2);
            _bufferPosition += numRead;

            _file_offset = _file_offset - AUDIO_BLOCK_SAMPLES * 2;
            if (_file_offset > 0)
                _file.seek(static_cast<unsigned int>(_file_offset));
        } else if (_bufferPosition > (AUDIO_BLOCK_SAMPLES-1) * 2) {
            _bufferPosition = (AUDIO_BLOCK_SAMPLES-1) * 2;
        }
    }

    //Serial.printf("buf %d/%d \n", _bufferPosition, _bufferLength);
    int16_t result = _buffer[_bufferPosition/2];
    if (_enable_interpolation) {
        if (_remainder != 0.0) {
            if (_readRate > 0.0) {
                if (_bufferPosition < _bufferLength-2) {
                    int16_t next =_buffer[(_bufferPosition/2)+1];
                    int16_t interpolated =  (((1-_remainder) * 1000.0 * result) + (_remainder * 1000.0 * next)) / 1000;
                    result = interpolated;
                }
            } else if (_readRate < 0.0) {
                if (_bufferPosition >= 2) {
                    int16_t prev =_buffer[(_bufferPosition/2)-1];
                    int16_t interpolated = (((1 + _remainder) * 1000.0 * result) + (-_remainder * 1000.0 * prev)) / 1000;
                    result = interpolated;
                }
            }
        }
    }

    //Serial.printf("result %4x \n", result);
    _remainder += _readRate;

    auto delta = static_cast<signed int>(_remainder);
    _remainder -= static_cast<float>(delta);

    _bufferPosition += 2 * delta;
    *value = result;
    return true;
}

void ResamplingSdReader::begin(void)
{
    _playing = false;
    _file_offset = 0;
    _file_size = 0;
}

bool ResamplingSdReader::play(const char *filename)
{
    stop();
#if defined(HAS_KINETIS_SDHC)
    if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStartUsingSPI();
#else
    AudioStartUsingSPI();
#endif
    __disable_irq();
    _file = SD.open(filename);
    __enable_irq();
    if (!_file) {
        //Serial.println("unable to open file");
#if defined(HAS_KINETIS_SDHC)
        if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStopUsingSPI();
#else
        AudioStopUsingSPI();
#endif
        return false;
    }
    _file_size = _file.size();
    if (_readRate < 0) {
        // reverse playback should start at the end of the audio sample
        _file_offset = _file.size() - AUDIO_BLOCK_SAMPLES * 2;
        _file.seek(_file_offset);

        int numRead = _file.read(_buffer, AUDIO_BLOCK_SAMPLES * 2);
        _bufferLength = numRead;
        if (numRead == 0)
            return false;

        // return file to position for next read
        if (_file_offset > AUDIO_BLOCK_SAMPLES * 2)
            _file_offset = _file_offset - AUDIO_BLOCK_SAMPLES * 2;
        else if (_file_offset < AUDIO_BLOCK_SAMPLES * 2)
            _file_offset = 0;
        _file.seek(_file_offset);

        _bufferPosition = numRead - 2;
    } else
        _file_offset = 0;

    _playing = true;
    return true;
}

void ResamplingSdReader::stop(void)
{
    __disable_irq();
    if (_playing) {
        _playing = false;
        __enable_irq();
        _file.close();
#if defined(HAS_KINETIS_SDHC)
        if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStopUsingSPI();
#else
        AudioStopUsingSPI();
#endif
    } else {
        __enable_irq();
    }
}

int ResamplingSdReader::available(void) {
    return _file.available() / _readRate;
}

void ResamplingSdReader::close(void) {
    if (_playing)
        stop();
    _file.close();
}
