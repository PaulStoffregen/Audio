#ifndef play_qspi_wav_h_
#define play_qspi_wav_h_

#include "Arduino.h"
#include "AudioStream.h"
#include "play_sd_wav.h"

#include <Adafruit_SPIFlash.h>
#include <Adafruit_SPIFlash_FatFs.h>
#include "Adafruit_QSPI_GD25Q.h"

class AudioPlayQspiWav : public AudioPlaySdWav
{
public:
    bool play(const char *filename);
    void useFilesystem(Adafruit_M0_Express_CircuitPython *fs){ _fs = fs; }
    void update(void);
    void stop(void);

protected:
    Adafruit_SPIFlash_FAT::File wavfile;
    Adafruit_M0_Express_CircuitPython *_fs;
};

#endif
