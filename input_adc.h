#ifndef input_adc_h_
#define input_adc_h_

#include "AudioStream.h"

class AudioInputAnalog : public AudioStream
{
public:
        AudioInputAnalog(unsigned int pin) : AudioStream(0, NULL) { begin(pin); }
        virtual void update(void);
        void begin(unsigned int pin);
        friend void dma_ch2_isr(void);
private:
        static audio_block_t *block_left;
        static uint16_t block_offset;
	uint16_t dc_average;
        static bool update_responsibility;
};

#endif
