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

#if !defined(play_wav_buffered_h_)
#define play_wav_buffered_h_

#include "Arduino.h"
#include "AudioStream.h"
#include "SD.h"
#include "EventResponder.h"
#include "AudioBuffer.h"

/*
#define SCOPE_PIN 25
#define SCOPE_SERIAL Serial1
#define SCOPESER_SPEED 57600
#include "oscope.h"
//*/


class AudioPlayWAVbuffered : public EventResponder, public AudioBuffer, public AudioWAVdata, public AudioStream
{
public:
	AudioPlayWAVbuffered(void);
	~AudioPlayWAVbuffered(void);
	
	bool playSD(const char* filename, bool paused = false, float startFrom = 0.0f);
	bool play(const File _file, bool paused = false, float startFrom = 0.0f);
	bool play(const char* filename, FS& fs = SD, bool paused = false, float startFrom = 0.0f) 
		{ return play(fs.open(filename), paused, startFrom); }
	bool play(AudioPreload& p, bool paused = false, float startFrom = 0.0f);
		
	bool play(void)  { if (isPaused())  togglePlayPause(); return isPlaying(); }
	bool pause(void) { if (isPlaying()) togglePlayPause(); return isPaused();  }
	bool cueSD(const char* filename, float startFrom = 0.0f) { return playSD(filename,true,startFrom); }
	bool cue(const File _file, float startFrom = 0.0f) { return play(_file,true,startFrom); }
	void togglePlayPause(void);
	void stop(uint8_t fromInt=0);
	bool isPlaying(void);
	bool isPaused(void);
	bool isStopped(void);	
	uint32_t positionMillis(void);
	uint32_t lengthMillis(void);
	uint32_t adjustHeaderInfo(void);
	virtual void update(void);
	void EventResponse(EventResponderRef evref);
	
	static uint8_t objcnt;
	// debug members
	size_t lowWater;
	uint32_t playCalled,firstUpdate,fileLoaded;
	LogLastMinMax<uint32_t> readMicros, bufferAvail;

private:
	enum state_e {STATE_STOP,STATE_STOPPING,STATE_PAUSED,STATE_PLAYING,STATE_LOADING};
	File wavfile;
	AudioPreload* ppl;
	size_t preloadRemaining;
	void loadBuffer(uint8_t* pb, size_t sz, bool firstLoad = false);
	bool prepareFile(bool paused, float startFrom, size_t startFromI);
	
	bool eof;
	bool readPending;
	uint8_t objnum;
	uint32_t data_length;		// number of bytes remaining in current section
	uint32_t total_length;		// number of audio data bytes in file
	
	volatile uint8_t state;
	volatile uint8_t state_play;
	
	// States of playback machine: we need two to track everything.
	volatile enum {
		silent,		// nothing being played
		sample,		// sample is being played
		fileLoad,	// file needs initial buffering (filestate only)
		fileReq,
		fileEvent,
		filePrepared,
		fileReady,	// file is buffered and ready (filestate only)
		file,		// file is being played
		ending
		} playState, fileState;
	uint8_t leftover_bytes;
};

class AudioPlayWAVstereo : public AudioPlayWAVbuffered {};
class AudioPlayWAVquad	 : public AudioPlayWAVbuffered {};
class AudioPlayWAVhex	 : public AudioPlayWAVbuffered {};
class AudioPlayWAVoct	 : public AudioPlayWAVbuffered {};

#endif // !defined(play_wav_buffered_h_)
