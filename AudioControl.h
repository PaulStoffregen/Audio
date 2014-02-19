#ifndef AudioControl_h_
#define AudioControl_h_

#include <stdint.h>

// A base class for all Codecs, DACs and ADCs, so at least the
// most basic functionality is consistent.

#define AUDIO_INPUT_LINEIN  0
#define AUDIO_INPUT_MIC     1

class AudioControl
{
public:
	virtual bool enable(void) = 0;
	virtual bool disable(void) = 0;
	virtual bool volume(float volume) = 0;      // volume 0.0 to 100.0
	virtual bool inputLevel(float volume) = 0;  // volume 0.0 to 100.0
	virtual bool inputSelect(int n) = 0;
};

#endif
