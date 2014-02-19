#ifndef analyze_fft256_h_
#define analyze_fft256_h_

#include "AudioStream.h"

// windows.c
extern "C" {
extern const int16_t AudioWindowHanning256[];
extern const int16_t AudioWindowBartlett256[];
extern const int16_t AudioWindowBlackman256[];
extern const int16_t AudioWindowFlattop256[];
extern const int16_t AudioWindowBlackmanHarris256[];
extern const int16_t AudioWindowNuttall256[];
extern const int16_t AudioWindowBlackmanNuttall256[];
extern const int16_t AudioWindowWelch256[];
extern const int16_t AudioWindowHamming256[];
extern const int16_t AudioWindowCosine256[];
extern const int16_t AudioWindowTukey256[];
}

class AudioAnalyzeFFT256 : public AudioStream
{
public:
	AudioAnalyzeFFT256(uint8_t navg = 8, const int16_t *win = AudioWindowHanning256)
	  : AudioStream(1, inputQueueArray), window(win), 
	    prevblock(NULL), count(0), naverage(navg), outputflag(false) { init(); }

	bool available() {
		if (outputflag == true) {
			outputflag = false;
			return true;
		}
		return false;
	}
	virtual void update(void);
	//uint32_t cycles;
	int32_t output[128] __attribute__ ((aligned (4)));
private:
	void init(void);
	const int16_t *window;
	audio_block_t *prevblock;
	int16_t buffer[512] __attribute__ ((aligned (4)));
	uint8_t count;
	uint8_t naverage;
	bool outputflag;
	audio_block_t *inputQueueArray[1];
};

#endif
