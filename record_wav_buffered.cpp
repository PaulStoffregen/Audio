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
#include "record_wav_buffered.h"

/* static */ uint8_t AudioRecordWAVbuffered::objcnt;
/* static */ void AudioRecordWAVbuffered::EventResponse(EventResponderRef evref)
{
	uint8_t* pb;
	size_t sz;
	
	AudioRecordWAVbuffered* pPWB = (AudioRecordWAVbuffered*) evref.getContext();
	pPWB->getNextWrite(&pb,&sz);	// find out where and how much
	if (sz > pPWB->bufSize / 2)			// never do more than half a buffer
		sz = pPWB->bufSize / 2;
	pPWB->flushBuffer(pb,sz);		// write out more file data from the buffer
}


void AudioRecordWAVbuffered::flushBuffer(uint8_t* pb, size_t sz)
{
	size_t outN;
	
SCOPE_HIGH();	
SCOPESER_TX(objnum);

	if (sz > 0) // file write triggered, but there's no data - ignore the request
	{
		
{//----------------------------------------------------
	size_t av = getAvailable();
	if (av != 0 && av < lowWater && !eof)
		lowWater = av;
SCOPESER_TX((av >> 8) & 0xFF);
SCOPESER_TX(av & 0xFF);
}//----------------------------------------------------

		uint32_t now = micros();
		outN = wavfile.write(pb,sz);	// try for that
		wavfile.flush();
		readMicros.newValue(micros() - now);
		
		if (outN < sz) // failed to write out all data
		{
			Serial.printf("write of %d bytes failed: wrote %d\n",sz,outN);
			// NOW what do we do?!
		}
		if (0 == total_length) 		// first write, includes WAV header...
			outN -= sizeof header;	// ...so remove that
		total_length += outN; 		// add to number of audio bytes recorded
		bufferAvail.newValue(getAvailable()); // worse than lowWater
		writeExecuted(sz);
	}
	writePending = false;
SCOPE_LOW();
}


AudioRecordWAVbuffered::AudioRecordWAVbuffered(unsigned char ninput, audio_block_t **iqueue) : 
		AudioWAVdata(ninput),
		AudioStream(ninput, iqueue),
		lowWater(0xFFFFFFFF),
		eof(false), writePending(false), objnum(objcnt++),
		data_length(0), total_length(0),
		state(STATE_STOP), state_record(STATE_STOP)
{
SCOPE_ENABLE();
SCOPESER_ENABLE();
	
	// prepare EventResponder to refill buffer
	// during yield(), if triggered from update()
	setContext(this);
	attach(EventResponse);
}

/**
 * Record to a file on SD card.
 * We open for read / write, but don't truncate the file as this may save
 * time allocating new sectors while it's shorter than the old file. We
 * do truncate at the end of recording, as the file length needs to be
 * consistent with the WAV header.
 */
bool AudioRecordWAVbuffered::recordSD(const char *filename, bool paused /* = false */)
{
	return record(SD.open(filename,O_RDWR), paused);
}


bool AudioRecordWAVbuffered::record(const File _file, bool paused /* = false */)
{
	bool rv = false;
	
	stop();
	wavfile = _file;
	
	// ensure a minimal buffer exists
	if (nullptr == buffer)
		createBuffer(1024,inHeap);
	
	if (wavfile && nullptr != buffer) 
	{
		uint8_t* pb;
		size_t sz,stagger;
		constexpr int SLOTS=16;
		
		//* stagger write timings:
		stagger = bufSize / 1024; 
		if (stagger > SLOTS)
			stagger = SLOTS;
		stagger = (bufSize>>1) / stagger;

		// prepare to write data
		emptyBuffer((objnum & (SLOTS-1)) * stagger); // ensure we start from scratch
		getNextWrite(&pb,&sz);	// find out where and how much we need for first write
		
		makeWAVheader(&header,chanCnt); // create a default WAV header
		write((uint8_t*) &header,sizeof header);	// not sure of alignment, can't do it in place: copy
		
		data_length = total_length = 0; // haven't written any audio data yet
		bytes2millis = getB2M(header.fmt.chanCnt,header.fmt.sampleRate,header.fmt.bitsPerSample);
		
		state_record = STATE_RECORDING;
		if (paused)
			state = STATE_PAUSED;
		else
			state = STATE_RECORDING;
		eof = false;
		rv = true;
		setInUse(true); // prevent changes to buffer memory
	}

	return rv;
}


/**
 * Write the current header data at the start of the file.
 * NOTE does NOT seek back to the current write position!
 * \return current file end position, or 0 if the header write failed
 */
uint32_t AudioRecordWAVbuffered::_writeCurrentHeader(void)
{
	uint32_t endPos = 0;
	
	if (wavfile)
	{
		// fix up the WAV file header with the correct lengths
		header.data.clen = total_length;
		header.riff.flen = total_length + sizeof header - offsetof(wavhdr_t,riff.wav);
		wavfile.seek(0);
		if (sizeof header == wavfile.write(&header,sizeof header))
			endPos = total_length;
	}
	
	return endPos;
}


/**
 * Write the current header data at the start of the file.
 * Seeks back to the current write position after the header write, so recording can continue.
 * \return current file end position, or 0 if the header write failed
 */
uint32_t AudioRecordWAVbuffered::writeCurrentHeader(void)
{
	uint32_t endPos = 0;
	
	if (wavfile)
	{
		size_t sz = wavfile.position();	// get current file position
		endPos = _writeCurrentHeader(); // seek to 0 and write header
		
		wavfile.seek(sz); // back to old write position
	}
	
	return endPos;
}


void AudioRecordWAVbuffered::stop(void)
{
	if (state != STATE_STOP) 
	{
		uint8_t* pb;
		size_t sz;
		
		state = STATE_STOP; // ensure update() no longer tries to write
		
		// ensure everything buffered gets written out
		if (writePending)
			yield();
		getNextWrite(&pb,&sz);	// find out where and how much
		flushBuffer(pb,sz);		// write out residual file data from the buffer

		// truncate at current position
		// we opened read/write, and it could already have existed; if so, and 
		// the new file is shorter, it will appear inconsistent with the WAV
		// header if it's not truncated to the new length
		sz = wavfile.position();	// need this, apparently
		wavfile.truncate(sz);
		
		// fix up the WAV file header with the correct lengths
		_writeCurrentHeader();
		
		wavfile.close();
		eof = true;
		setInUse(false); // allow changes to buffer memory
	}
}


void AudioRecordWAVbuffered::toggleRecordPause(void) {
	// take no action if wave header is not parsed OR
	// state is explicitly STATE_STOP
	if(state_record >= 8 || state == STATE_STOP) return;

	// toggle back and forth between state_record and STATE_PAUSED
	if(state == state_record) {
		state = STATE_PAUSED;
	}
	else if(state == STATE_PAUSED) {
		state = state_record;
	}
}


// interleave channels of audio from separate blocks into buf 
static void interleave(int16_t* buf,int16_t** blocks,uint16_t channels)
{
	if (1 == channels) // mono, do the simple thing
	{
		if (nullptr != blocks[0])
			memcpy(buf,blocks[0],AUDIO_BLOCK_SAMPLES * sizeof *buf);
		else
			memset(buf,0,AUDIO_BLOCK_SAMPLES * sizeof *buf);
	}
	else
		for (uint16_t i=0;i<channels;i++)
		{
			int16_t* pd = buf+i;
			int16_t* ps = blocks[i];
			
			if (nullptr != ps)
			{
				for (int j=0;j<AUDIO_BLOCK_SAMPLES;j++)
				{
					*pd = *ps++;
					pd += channels;
				}
			}
			else // null data, interpret as silence
			{
				for (int j=0;j<AUDIO_BLOCK_SAMPLES;j++)
				{
					*pd = 0;
					pd += channels;
				}
			}
		}
}


void AudioRecordWAVbuffered::update(void)
{
	int16_t buf[chanCnt * AUDIO_BLOCK_SAMPLES];
	audio_block_t* blocks[chanCnt];	
	int16_t* data[chanCnt] = {0};
	int alloCnt = 0; 	// count of blocks successfully received
	
	// receive the audio blocks to record
	while (alloCnt < chanCnt)
	{
		blocks[alloCnt] = receiveReadOnly(alloCnt);
		if (nullptr != blocks[alloCnt])
			data[alloCnt] = blocks[alloCnt]->data;
		alloCnt++;
	}
	
	// only update if we're recording and not paused,
	// but we must discard the received blocks!
	if (state != STATE_STOP && state != STATE_PAUSED)
	{
		if (alloCnt >= chanCnt) // received enough - extract the data
		{
			interleave(buf,data,chanCnt);	// make a chunk of data for the file
			result rdr = write((uint8_t*) buf, sizeof buf); // send it to the buffer
			data_length += sizeof buf;
			
			if (ok != rdr 			// there's now room for a buffer read,
				&& !eof 			// and more file data available
				&& !writePending)  	// and we haven't already asked
			{
				triggerEvent(rdr);
				writePending = true;
			}
		}
	}
	
	// relinquish our interest in these blocks
	while (--alloCnt >= 0)
		if (nullptr != blocks[alloCnt]) // stock release() can't cope with NULL pointer
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



bool AudioRecordWAVbuffered::isRecording(void)
{
	uint8_t s = *(volatile uint8_t *)&state;
	return (s == STATE_RECORDING);
}


bool AudioRecordWAVbuffered::isPaused(void)
{
	uint8_t s = *(volatile uint8_t *)&state;
	return (s == STATE_PAUSED);
}


bool AudioRecordWAVbuffered::isStopped(void)
{
	uint8_t s = *(volatile uint8_t *)&state;
	return (s == STATE_STOP);
}

/**
 * Approximate progress in milliseconds.
 *
 * This actually reflects the state when the last
 * update occurred, so it will be out of date by up to
 * 2.9ms (with normal settings of 44100/16bit), and jump
 * in increments of 2.9ms. Also, it will differ from
 * what you hear, depending on the design and hardware 
 * delays.
 */
uint32_t AudioRecordWAVbuffered::positionMillis(void)
{
	uint8_t s = *(volatile uint8_t *)&state;
	if (s >= 8 && s != STATE_PAUSED) return 0;
	uint32_t dlength = *(volatile uint32_t *)&data_length;
	uint32_t b2m = *(volatile uint32_t *)&bytes2millis;
	return ((uint64_t)dlength * b2m) >> 32;
}


/**
 * Approximate file write progress in milliseconds.
 *
 * This will be behind the positionMillis() value due
 * to the buffering, and will also "jump" in value
 * as writes occur. The bigger the buffer, the larger
 * and less frequent these jumps will be.
 */
uint32_t AudioRecordWAVbuffered::lengthMillis(void)
{
	uint8_t s = *(volatile uint8_t *)&state;
	if (s >= 8 && s != STATE_PAUSED) return 0;
	uint32_t tlength = *(volatile uint32_t *)&total_length;
	uint32_t b2m = *(volatile uint32_t *)&bytes2millis;
	return ((uint64_t)tlength * b2m) >> 32;
}

