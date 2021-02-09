
//-----------------------------------------------------------
// Huovilainen New Moog (HNM) model as per CMJ jun 2006
// Implemented as Teensy Audio Library compatible object
// Richard van Hoesel, Feb. 9 2021
// v.1.01 now includes FC "CV" modulation input
// please retain this header if you use this code.
//-----------------------------------------------------------

// https://forum.pjrc.com/threads/60488?p=269609&viewfull=1#post269609

#ifndef filter_ladder_h_
#define filter_ladder_h_

#include "Arduino.h"
#include "AudioStream.h"

class AudioFilterLadder: public AudioStream
{
public:
	AudioFilterLadder() : AudioStream(2, inputQueueArray) {};
	void frequency(float FC);
	void resonance(float reson);
	virtual void update(void);
private:
	float LPF(float s, int i);
	void compute_coeffs(float fc);
	float alpha = 1.0;
	float beta[4] = {0.0, 0.0, 0.0, 0.0};
	float z0[4] = {0.0, 0.0, 0.0, 0.0};
	float z1[4] = {0.0, 0.0, 0.0, 0.0};
	float K = 1.0;
	float Fbase = 1000;
	float overdrive = 1.0;
	audio_block_t *inputQueueArray[2];
};

#endif
