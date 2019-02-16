//
// Created by Nicholas Newdigate on 10/02/2019.
//

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
    uint16_t result = _buffer[_bufferPosition/2];
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
