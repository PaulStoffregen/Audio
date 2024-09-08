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

#include <Arduino.h>
#include "play_wav_buffered.h"

/*
 * Prepare to play back from a file
 */
bool AudioPlayWAVbuffered::prepareFile(bool paused, float startFrom, size_t startFromI)
{
	bool rv = false;
	
	if (wavfile && nullptr != buffer) 
	{
		uint8_t* pb;
		size_t sz,stagger,skip;
		constexpr int SLOTS=16;
		int actualSlots = SLOTS;
		
		//* stagger pre-load:
		actualSlots = bufSize / 1024; 
		if (actualSlots > SLOTS)
			actualSlots = SLOTS;
		stagger = (bufSize>>1) / actualSlots;
		
		// load data
		emptyBuffer((objnum & (actualSlots-1)) * stagger); // ensure we start from scratch
		parseWAVheader(wavfile); 	// figure out WAV file structure
		getNextRead(&pb,&sz);		// find out where and how much the buffer pre-load is
		
		data_length = total_length = audioSize; // all available data
		eof = false;
		
		if (startFrom <= 0.0f && 0 == startFromI) // not starting later in time or bytes
		{
			skip = firstAudio;
		}
		else // we want to start playback later into the file: compute sector containing start point
		{
			if (0 == startFromI) // use time, not absolute file position
				startFromI = millisToPosition(startFrom,AUDIO_SAMPLE_RATE);
			skip = startFromI & (512-1);	// skip partial sector...
			startFromI -= skip;				// ...having loaded from sector containing start point
			wavfile.seek(startFromI);
			data_length  = audioSize - skip + firstAudio - startFromI; // where we started playing, so already "used"
		}
		
		loadBuffer(pb,sz,true);	// load initial file data to the buffer: may set eof
		read(nullptr,skip);	// skip the header
		
		fileLoaded = ARM_DWT_CYCCNT;
		
		state_play = STATE_PLAYING;
		if (paused)
			state = STATE_PAUSED;
		else
			state = STATE_PLAYING;
		rv = true;
		setInUse(true); // prevent changes to buffer memory
		fileState = filePrepared;
	}
	
	return rv;
}


/* static */ uint8_t AudioPlayWAVbuffered::objcnt;
void AudioPlayWAVbuffered::EventResponse(EventResponderRef evref)
{
	uint8_t* pb;
	size_t sz;
	
	switch (evref.getStatus())
	{
		case STATE_LOADING: // playing pre-load: open and prepare file, ready to switch over
			fileState = fileEvent;
			wavfile = ppl->open();
			if (prepareFile(STATE_PAUSED == state,0.0f,ppl->fileOffset))
				fileState = fileReady;
			else
			{
				//fileState = ending;
				triggerEvent(STATE_LOADING); // re-trigger first file read
			}
			break;
			
		case STATE_PLAYING: // request from update() to re-fill buffer
			getNextRead(&pb,&sz);		// find out where and how much
			if (sz > bufSize / 2)
				sz = bufSize / 2;		// limit reads to a half-buffer at a time
			loadBuffer(pb,sz);		// load more file data to the buffer
			break;
			
		case STATE_STOP: // stopped from interrupt - finish the job
// if (!eof) Serial.printf("STOP event before EOF at %lu\n",millis());		
			stop();
			break;
			
		default:
			asm("nop");
			break;
	}
}


static void EventDespatcher(EventResponderRef evref)
{
	AudioPlayWAVbuffered* pPWB = (AudioPlayWAVbuffered*) evref.getContext();
	
	pPWB->EventResponse(evref);
}


void AudioPlayWAVbuffered::loadBuffer(uint8_t* pb, size_t sz, bool firstLoad /* = false */)
{
	size_t got;
	
SCOPE_HIGH();	
SCOPESER_TX(objnum);

	if (sz > 0 && !eof) // read triggered, but there's no room or already stopped - ignore the request
	{
		
{//----------------------------------------------------
	size_t av = getAvailable();
	if (av != 0 && av < lowWater && !eof)
		lowWater = av;
SCOPESER_TX((av >> 8) & 0xFF);
SCOPESER_TX(av & 0xFF);
}//----------------------------------------------------

		uint32_t now = micros();
		got = wavfile.read(pb,sz);	// try for that
		readMicros.newValue(micros() - now);
		
		if (got < sz) // there wasn't enough data
		{
			if (got < 0)
				got = 0;

			memset(pb+got,0,sz-got); // zero the rest of the buffer
			eof = true;
		}
		if (!firstLoad) // empty on first load, don't record result
			bufferAvail.newValue(getAvailable()); // worse than lowWater
		readExecuted(got);
	}
	readPending = false;
SCOPE_LOW();
}


/**
 * Adjust header information if file has changed since playback started.
 * This can happen in looper applications, where we want to begin playback
 * while still recording the tail of the file.
 *
 * Called from the application, so we are guaranteed the EventResponder
 * will not access the file to re-fill buffers during this function.
 *
 * \return amount the audio size changed by: could be 0
 */
uint32_t AudioPlayWAVbuffered::adjustHeaderInfo(void)
{
	uint32_t result = 0;
	
	if (wavfile) // we'd better be playing, really!
	{
		size_t readPos = wavfile.position(); // keep current position safe
		AudioWAVdata newWAV;
		
		// parse the current header, then seek back to where we were
		newWAV.parseWAVheader(wavfile);
		wavfile.seek(readPos);
		
		if (newWAV.audioSize != audioSize) // file header has been changed since we started
		{
			result = newWAV.audioSize - audioSize;  // change is this (in bytes)
			__disable_irq(); // audio update may change this...
				data_length += result;
			__enable_irq(); // ...safe now
			total_length += result;
			samples = newWAV.samples;
		}
	}
	
	return result;
}


/* Constructor */
AudioPlayWAVbuffered::AudioPlayWAVbuffered(void) : 
		AudioStream(0, NULL),
		lowWater(0xFFFFFFFF),
		wavfile(0), ppl(0), preloadRemaining(0),
		eof(false), readPending(false), objnum(objcnt++),
		data_length(0), total_length(0),
		state(STATE_STOP), state_play(STATE_STOP),
		playState(silent), fileState(silent),
		leftover_bytes(0)
{
SCOPE_ENABLE();
SCOPESER_ENABLE();
	
	// prepare EventResponder to refill buffer
	// during yield(), if triggered from update()
	setContext(this);
	attach(EventDespatcher);
}


/*
 * Destructor
 *
 * Could be called while active, so need to take care that destruction
 * is done in a sane order. 
 *
 * MUST NOT destruct the object from an ISR!
 */
AudioPlayWAVbuffered::~AudioPlayWAVbuffered(void)
{
	stop(); // close file, relinquish use of preload buffer, any triggered event cleared, set to STATE_STOPPING
	// Further destructor actions:
	// This destructor exits, wavfile is destructed
	// ~AudioStream: unlinked from connections and update list
	// ~AudioWAVdata
	// ~AudioBuffer: ~MemBuffer disposes of buffer memory, if bufType != given
	// ~EventResponder: detached from event list
}

bool AudioPlayWAVbuffered::playSD(const char *filename, bool paused /* = false */, float startFrom /* = 0.0f */)
{
	return play(SD.open(filename), paused, startFrom);
}


bool AudioPlayWAVbuffered::play(const File _file, bool paused /* = false */, float startFrom /* = 0.0f */)
{
	bool rv = false;
	
	playCalled = ARM_DWT_CYCCNT;
	firstUpdate = fileLoaded = 0;
	
	stop();
	wavfile = _file;
	
	// ensure a minimal buffer exists
	if (nullptr == buffer)
		createBuffer(1024,inHeap);
	
	rv = prepareFile(paused,startFrom,0); // get file ready to play
	if (rv)
	{
		fileState = fileReady;
		playState = file;
	}

	return rv;
}


/*
 * Play audio starting from pre-loaded buffer, and switching to filesystem when that's exhausted.
 *
 * Note that while there's an option to play from a point later than the first sample of the pre-load,
 * this could result in skipping the pre-loaded data altogether, followed by a delay while
 * the first samples are loaded from the filesystem. It's up to the user to manage this. 
 *
 * Note also that the two startFrom values are cumulative; if you pre-load starting at 100.0ms, 
 * then play() starting at 50.0ms, playback starts 150.0ms into the audio file.
 */
bool AudioPlayWAVbuffered::play(AudioPreload& p, bool paused /* = false */, float startFrom /* = 0.0f */)
{
	bool rv = false;
	
	playCalled = ARM_DWT_CYCCNT;
	firstUpdate = fileLoaded = 0;
	
	stop();
	
	if (p.isReady())
	{
		bool justPlay = false;  // assume we can use the pre-load
		
		ppl = &p;				// using this preload
		preloadRemaining = ppl->valid; // got this much data left
		chanCnt = ppl->chanCnt;	// we need to know the channel count to de-interleave
		
		if (startFrom > 0.0f)
		{
			float plms = (float) p.valid / p.sampleSize / AUDIO_SAMPLE_RATE; // audio pre-loaded [ms]
			if (plms > startFrom) // can use some of the pre-load
			{
				int nsamp = startFrom * AUDIO_SAMPLE_RATE / 1000.0f; // samples to skip
				preloadRemaining -= nsamp * p.sampleSize; // ensure we're on a sample boundary 
			}
			else // just do a normal play()
				justPlay = true;
		}
		
		if (justPlay)
			rv = play(p.filepath,*p.pFS,paused,startFrom + p.startSample / AUDIO_SAMPLE_RATE / 1000.0f);
		else
		{
			playState = sample;		// start by playing pre-loaded data
			fileState = fileLoad;	// load file buffer on first event
			ppl->setInUse(true); // prevent changes to buffer memory

			state_play = STATE_PLAYING;
			if (paused)
				state = STATE_PAUSED;
			else
				state = STATE_PLAYING;
			rv = true;
		}
	}
	
	return rv;
}


void AudioPlayWAVbuffered::stop(uint8_t fromInt /* = false */)
{
	bool eventTriggered = false;
	if (state != STATE_STOP) 
	{
		state = STATE_STOPPING; // prevent update() from doing anything
		if (wavfile) // audio file is open
		{
			if (fromInt)	// can't close file, SD action may be in progress
			{
				triggerEvent(STATE_STOP); // close on next yield()
				eventTriggered = true;
			}
			else
			{
				wavfile.close();
				state = state_play = STATE_STOP;
				playState /* = fileState */ = silent;
			}
		}
		else // file is closed, can always stop immediately
		{
			state = state_play = STATE_STOP;
			playState /* = fileState */ = silent;
		}
		
		if (!eventTriggered) // if we didn't just trigger a stop event...
			clearEvent();	 // ...clear pending read, file will be closed!
		
		if (!eventTriggered && nullptr != ppl) // preload is in use
		{
			ppl->setInUse(false);
			ppl = nullptr;
		}
		
		readPending = false;
		eof = true;
		setInUse(false); // allow changes to buffer memory
	}
}


void AudioPlayWAVbuffered::togglePlayPause(void) {
	// take no action if wave header is not parsed OR
	// state is explicitly STATE_STOP
	if(state_play >= 8 || state == STATE_STOP || state == STATE_STOPPING) return;

	// toggle back and forth between state_play and STATE_PAUSED
	if(state == state_play) {
		state = STATE_PAUSED;
	}
	else if(state == STATE_PAUSED) {
		state = state_play;
	}
}


// de-interleave channels of audio from buf into separate blocks
static void deinterleave(int16_t* buf,int16_t** blocks,uint16_t channels)
{
	if (1 == channels) // mono, do the simple thing
		memcpy(blocks[0],buf,AUDIO_BLOCK_SAMPLES * sizeof *buf);
	else
		for (uint16_t i=0;i<channels;i++)
		{
			int16_t* ps = buf+i;
			int16_t* pd = blocks[i];
			for (int j=0;j<AUDIO_BLOCK_SAMPLES;j++)
			{
				*pd++ = *ps;
				ps += channels;
			}
		}
}


void AudioPlayWAVbuffered::update(void)
{
	int16_t buf[chanCnt * AUDIO_BLOCK_SAMPLES];
	audio_block_t* blocks[chanCnt];	
	int16_t* data[chanCnt];
	int alloCnt = 0; // count of blocks successfully allocated
	
	// only update if we're playing and not paused
	if (state == STATE_STOP || state == STATE_STOPPING || state == STATE_PAUSED) 
		return;
	
	// just possible the channel count will be zero, if a file suddenly goes AWOL:
	if (0 == chanCnt)
	{
		stop(true);
		return;
	}

	if (0 == firstUpdate)
		firstUpdate = ARM_DWT_CYCCNT;
	
	// if just started, we may need to trigger the first file read
	if (!readPending && fileLoad == fileState)
	{
		triggerEvent(STATE_LOADING); // trigger first file read
		readPending = true;
		fileState = fileReq;
	}

	// allocate the audio blocks to transmit
	while (alloCnt < chanCnt)
	{
		blocks[alloCnt] = allocate();
		if (nullptr == blocks[alloCnt])
			break;
		data[alloCnt] = blocks[alloCnt]->data;
		alloCnt++;
	}
	
	if (alloCnt >= chanCnt) // allocated enough - fill them with data
	{
		size_t toFill = sizeof buf, got = 0;
		result rdr = ok;
		
		// if we're using pre-load buffer, try to fill from there
		if (sample == playState)
		{
			if (preloadRemaining >= toFill) // enough or more!
			{
				memcpy(buf,ppl->buffer+(ppl->valid - preloadRemaining),toFill);
				preloadRemaining -= toFill;
				got = toFill;
				toFill = 0;
			}
			else // not enough, but some
			{
				memcpy(buf,ppl->buffer+(ppl->valid - preloadRemaining),preloadRemaining);
				got = preloadRemaining;
				preloadRemaining = 0;
				toFill -= got;
			}

			if (0 == preloadRemaining)
				playState = file;
		}
		
		// preload, if in use, is needed until the file buffer is ready
		if (nullptr != ppl && file == playState && fileReady == fileState)
		{
			ppl->setInUse(false); // finished with preload
			ppl = nullptr;
		}
		
		// try to fill buffer from file, settle for what's available
		if (file == playState && toFill > 0) // file is ready and we still need data
		{
			size_t toRead = getAvailable();
			if (toRead > toFill)
			{
				toRead = toFill;
			}
			
			// also, don't play past end of data
			// could leave some data unread in buffer, but
			// it's not audio!
			if (toRead > data_length)
				toRead = data_length;
			
			// unbuffer
			rdr = read((uint8_t*) buf + got,toRead);
			
			if (invalid != rdr) // we got enough
				got += toRead;				
		}
		
		if (got < sizeof buf) // not enough data in buffer
		{
			memset(((uint8_t*) buf)+got,0,sizeof buf - got); // fill with silence
			stop(true); // and stop (within ISR): brutal, but probably better than losing sync
		}
		
		// deinterleave to audio blocks
		deinterleave(buf,data,chanCnt);

		if (ok != rdr 			// there's now room for a buffer read,
			&& !eof 			// and more file data available
			&& !readPending)  	// and we haven't already asked
		{
			triggerEvent(STATE_PLAYING); // trigger a file read
			readPending = true;
		}
		
		// transmit: mono goes to both outputs, stereo
		// upward go to the relevant channels
		transmit(blocks[0], 0);
		if (1 == chanCnt)
			transmit(blocks[0], 1);
		else
		{
			for (int i=1;i<chanCnt;i++)
				transmit(blocks[i], i);
		}

		// deal with position tracking
		if (got <= data_length)
			data_length -= got;
		else
			data_length = 0;
	}
	
	// relinquish our interest in these blocks
	while (--alloCnt >= 0)
		release(blocks[alloCnt]);
	
}

/*
00000000  52494646 66EA6903 57415645 666D7420  RIFFf.i.WAVEfmt 
00000010  10000000 01000200 44AC0000 10B10200  ........D.......
00000020  04001000 4C495354 3A000000 494E464F  ....LIST:...INFO
00000030  494E414D 14000000 49205761 6E742054  INAM....I Want T
00000040  6F20436F 6D65204F 76657200 49415254  o Come Over.IART
00000050  12000000 4D656C69 73736120 45746865  ....Melissa Ethe
00000060  72696467 65006461 746100EA 69030100  ridge.data..i...
00000070  FEFF0300 FCFF0400 FDFF0200 0000FEFF  ................
00000080  0300FDFF 0200FFFF 00000100 FEFF0300  ................
00000090  FDFF0300 FDFF0200 FFFF0100 0000FFFF  ................
*/



bool AudioPlayWAVbuffered::isPlaying(void)
{
	uint8_t s = *(volatile uint8_t *)&state;
	return (s == STATE_PLAYING);
}


bool AudioPlayWAVbuffered::isPaused(void)
{
	uint8_t s = *(volatile uint8_t *)&state;
	return (s == STATE_PAUSED);
}


bool AudioPlayWAVbuffered::isStopped(void)
{
	uint8_t s = *(volatile uint8_t *)&state;
	return (s == STATE_STOP || s == STATE_STOP);
}


/**
 * Approximate progress in milliseconds.
 *
 * This actually reflects the state of play when the last
 * update occurred, so it will be out of date by up to
 * 2.9ms (with normal settings of 44100/16bit), and jump
 * in increments of 2.9ms. Also, it will differ from
 * what you hear, depending on the design and hardware 
 * delays.
 */
uint32_t AudioPlayWAVbuffered::positionMillis(void)
{
	uint8_t s = *(volatile uint8_t *)&state;
	if (s >= 8 && s != STATE_PAUSED) return 0;
	uint32_t tlength = *(volatile uint32_t *)&total_length;
	uint32_t dlength = *(volatile uint32_t *)&data_length;
	uint32_t offset = tlength - dlength;
	uint32_t b2m = *(volatile uint32_t *)&bytes2millis;
	return ((uint64_t)offset * b2m) >> 32;
}


uint32_t AudioPlayWAVbuffered::lengthMillis(void)
{
	uint8_t s = *(volatile uint8_t *)&state;
	if (s >= 8 && s != STATE_PAUSED) return 0;
	uint32_t tlength = *(volatile uint32_t *)&total_length;
	uint32_t b2m = *(volatile uint32_t *)&bytes2millis;
	return ((uint64_t)tlength * b2m) >> 32;
}






