//
// Created by Nicholas Newdigate on 10/02/2019.
//

#include "ResamplingSdReader.h"

int ResamplingSdReader::read(void *buf, uint16_t nbyte) {

    unsigned int count = 0;
    int16_t *index = (int16_t*)buf;
    for (int i=0; i< nbyte/2; i++) {
        //Serial.printf("i=%d \n", i);
        if (readNextValue(index))
            count+=2;
        else
            return count;

        index++;
    }
    return count;
}

bool ResamplingSdReader::readNextValue(int16_t *value) {
    if ( _bufferLength == 0 || _bufferPosition >= _bufferLength) {
        // fill buffer from file
        unsigned int numRead = 0;
        if (_file.available() > 0) {
            //Serial.printf("read %d bytes\n", AUDIO_BLOCK_SAMPLES * 2);
            numRead = _file.read(_buffer, AUDIO_BLOCK_SAMPLES * 2);
            if (numRead == 0)
                return false;
            _file_offset = _file.position();
            //Serial.printf("numread %d\n", numRead);
        }
        else
            return false;

        _bufferLength = numRead;
        _bufferPosition = 0;
    }

    //Serial.printf("buf %d/%d \n", _bufferPosition, _bufferLength);
    uint16_t result = _buffer[_bufferPosition/2];
    //Serial.printf("result %4x \n", result);
    _remainder += _readRate;

    unsigned int delta = static_cast<unsigned int>(_remainder);
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
    _file_offset = 0;
    //Serial.println("able to open file");
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
