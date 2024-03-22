/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef play_queue_h_
#define play_queue_h_

#include <Arduino.h>     // github.com/PaulStoffregen/cores/blob/master/teensy4/Arduino.h
#include <AudioStream.h> // github.com/PaulStoffregen/cores/blob/master/teensy4/AudioStream.h

class AudioPlayQueue : public AudioStream
{
private:
#if defined(__IMXRT1062__) || defined(__MK66FX1M0__) || defined(__MK64FX512__)
	static const unsigned int MAX_BUFFERS = 80;
#else
	static const unsigned int MAX_BUFFERS = 32;
#endif
public:
	AudioPlayQueue(void) : AudioStream(0, NULL),
	  userblock(NULL), uptr(0), head(0), tail(0), max_buffers(MAX_BUFFERS) { }
	uint32_t play(int16_t data);
	uint32_t play(const int16_t *data, uint32_t len);
	bool available(void);
	int16_t * getBuffer(void);
	uint32_t playBuffer(void);
	void stop(void);
	void setMaxBuffers(uint8_t);
	//bool isPlaying(void) { return playing; }
	virtual void update(void);
	enum behaviour_e {ORIGINAL,NON_STALLING};
	void setBehaviour(behaviour_e behave) {behaviour = behave;}
private:
	audio_block_t *queue[MAX_BUFFERS];
	audio_block_t *userblock;
	unsigned int uptr; // actually an index, NOT a pointer!
	volatile uint8_t head, tail;
	volatile uint8_t max_buffers;
	behaviour_e behaviour;
};

#endif
