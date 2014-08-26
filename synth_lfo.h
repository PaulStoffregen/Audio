///////////////////////////////////////////////////////////////////////////////////////////
#ifndef lfo_h
#define lfo_h
#include "audiostream.h"
#include "arm_math.h"
#include "utility/dspinst.h"
///////////////////////////////////////////////////////////////////////////////////////////
extern const int16_t lfoSinTable[];
extern const int16_t lfoTriTable[257];
extern const int16_t lfoSqrTable[257];
extern const int16_t lfoSawTable[257];

#define selectSinTable 0
#define selectTriTable 1
#define selectSqrTable 2
#define selectSawTable 3
///////////////////////////////////////////////////////////////////////////////////////////
class AudioSynthWaveformLfo : public AudioStream
{
public:
    AudioSynthWaveformLfo() : AudioStream(0, NULL), magnitude(16384) {}
    void frequency(float freq);
    void amplitude(float n) 
    {
	if (n < 0) n = 0;
	else if (n > 1.0) n = 1.0;
	magnitude = n * 65536.0;
    }
    virtual void update(void);
    void toggleWaveTable();
    void setWaveTable(int newTable);
private:
    void switchTable(uint8_t table);  
    uint32_t phase;
    uint32_t phaseInc;
    int32_t magnitude;
    int selectTable;
    const int16_t *pWaveTable = lfoSinTable;
};

#endif
