#ifndef play_qspi_wav_h_
#define play_qspi_wav_h_

#include "Arduino.h"
#include "AudioStream.h"
#include "play_sd_wav.h"

#include <SPI.h>
#include <SdFat.h>
#include <Adafruit_SPIFlash.h>


class AudioPlayQspiWav : public AudioPlaySdWav
{
public:
    bool play(const char *filename);
    void useFilesystem(FatFileSystem *fs){ _fs = fs; }
    void update(void);
    void stop(void);

protected:
    File wavfile;
    FatFileSystem *_fs;
};

#endif
