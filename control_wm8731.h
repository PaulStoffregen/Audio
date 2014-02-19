#ifndef control_wm8731_h_
#define control_wm8731_h_

#include "AudioControl.h"

class AudioControlWM8731 : public AudioControl
{
public:
	bool enable(void);
	bool disable(void) { return false; }
	bool volume(float n) { return volumeInteger(n * 0.8 + 47.499); }
	bool inputLevel(float n) { return false; }
	bool inputSelect(int n) { return false; }
protected:
	bool write(unsigned int reg, unsigned int val);
	bool volumeInteger(unsigned int n); // range: 0x2F to 0x7F
};

class AudioControlWM8731master : public AudioControlWM8731
{
public:
	bool enable(void);
};

#endif
