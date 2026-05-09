/* Audio Library for Teensy 3.x, 4.x
 * Copyright (c) 2022, Jonathan Oakley, teensy-jro@0akley.co.uk
 *
 * Development of this audio library was enabled by PJRC.COM, LLC by sales of
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

#include "Audio.h"
#include "AudioBuffer.h"

static const uint32_t B2M_44100 = (uint32_t)((double)4294967296000.0 / AUDIO_SAMPLE_RATE_EXACT);
static const uint32_t B2M_22050 = (uint32_t)((double)4294967296000.0 / AUDIO_SAMPLE_RATE_EXACT * 2.0);
static const uint32_t B2M_11025 = (uint32_t)((double)4294967296000.0 / AUDIO_SAMPLE_RATE_EXACT * 4.0);

/**
 * Dispose of allocated buffer by releasing it back to the heap.
 * Heap used depends on where it was allocated from: in the case of a buffer 
 * "given" to the instance, the buffer must be freed by the application
 * itself, after this function has been called.
 * \return ok
 */
AudioBuffer::result MemBuffer::disposeBuffer()
{
	result rv = invalid;
	
	if (0 == inUse)
	{
		if (nullptr != buffer)
		{
			switch (bufTypeX) // free buffer, if we created it
			{
				case inHeap:
					free(buffer);
					break;

#if defined(ARDUINO_TEENSY41)
				case inExt:
					extmem_free(buffer);
					break;
#endif // defined(ARDUINO_TEENSY41)
					
				default:
					break;
			}
		}
		
		buffer = nullptr;
		bufSize = 0;
		bufTypeX = none;
		
		rv = ok;
	}
	
	return rv;
}


/**
 * Create buffer by passing a memory pointer and size.
 * The application is responsible for managing the buffer, e.g.
 * by freeing it after disposeBuffer() has been called.
 */
AudioBuffer::result MemBuffer::createBuffer(uint8_t* buf, //!< pointer to memory buffer
											  size_t sz)	//!< size of memory buffer
{
	result rv = invalid;
	
	if (0 == inUse)
	{
		disposeBuffer(); // ensure existing buffer is freed, if possible
		
		buffer = (uint8_t*) buf;	
		bufSize = sz;
		bufTypeX = given;	

		rv = ok;
	}
	
	return rv;
}


/**
 * Create buffer by passing a required size and memory type.
 * The class is responsible for managing the buffer, e.g.
 * by freeing it when disposeBuffer() has been called or an
 * instance is deleted.
 * \return ok if created, invalid if memory couldn't be allocated
 */
AudioBuffer::result MemBuffer::createBuffer(size_t sz, //!< requested size of memory buffer
											  bufType typ)
{
	AudioBuffer::result rv = invalid;
	void* buf = nullptr;

	if (0 == inUse)
	{
		switch (typ)
		{
			case inHeap:
				buf = malloc(sz);
				break;

#if defined(ARDUINO_TEENSY41)
			case inExt:
				buf = extmem_malloc(sz);
				break;
#endif // defined(ARDUINO_TEENSY41)
				
			default:
				break;
		}
		
		if (buf != nullptr)
		{
			rv = createBuffer((uint8_t*) buf,sz);
			bufTypeX = typ; // fix up buffer memory type
		}
	}	
	
	return rv;
}


/********************************************************************************/
/**
 * Read data from buffer.
 * The class deals with unwrapping the data if the requested size overlaps
 * the end of the buffer memory. It also keeps track of whether the buffer has
 * enough data to satisfy the request, and if it has become possible to refill
 * the buffer with new data.
 *
 * Data may be discarded by passing a NULL destination pointer.
 *
 * \return ok if data read and no refill needed; halfEmpty if a partial refill can be done;
 * underflow if a complete refill is needed; invalid if data was not read from the buffer
 * because the amount required was greater than that available
 */
AudioBuffer::result AudioBuffer::read(uint8_t* dest, //!< pointer to memory to copy buffer data to, or null
									  size_t bytes)  //!< amount of data required
{
	AudioBuffer::result rv = ok;
	size_t halfSize = bufSize / 2;

	if (bytes != 0)
	{
		if (bytes <= getAvailable())
		{
			isFull = false;
			if (queueOut + bytes >= bufSize)
			{
				if (nullptr != dest) 
				{
					memcpy(dest,buffer+queueOut,bufSize-queueOut);
					dest += bufSize-queueOut;
				}
//for (size_t i=0;i<bufSize-queueOut;i++) buffer[queueOut+i] |= 0x20;
				bytes -= bufSize-queueOut; // this could be zero
				queueOut = 0;
			}
		
			if (bytes > 0)
			{
				if (nullptr != dest) 
					memcpy(dest,buffer+queueOut,bytes);
//for (size_t i=0;i<bytes;i++) buffer[queueOut+i] |= 0x20;
				
				queueOut += bytes;
			}
			
			if (queueIn == queueOut)
				rv = underflow;
			else if ((queueIn  < halfSize && queueOut >= halfSize)
				 ||  (queueOut < halfSize && queueIn  >= halfSize))
				rv = halfEmpty;
		}
		else
			rv = invalid; // buffer was not read, insufficient data was available
	}
	
	return rv;
}


/**
 * Write data to buffer.
 * The class deals with wrapping the data if the requested size overlaps
 * the end of the buffer memory. 
 *
 * Data may be zeroed by passing a NULL destination pointer.
 *
 * \return ok if data read and no refill needed; halfEmpty if a partial refill can be done;
 * underflow if a complete refill is needed; invalid if data was not read from the buffer
 * because the amount required was greater than that available
 */
AudioBuffer::result AudioBuffer::write(uint8_t* src, //!< pointer to memory to copy buffer data from, or null
									  size_t bytes)  //!< amount of data 
{
	AudioBuffer::result rv = ok;
	size_t halfSize = bufSize / 2;

	if (bytes != 0)
	{
		if (bytes <= bufSize - getAvailable())
		{
			if (queueIn + bytes >= bufSize)
			{
				if (nullptr != src) 
				{
					memcpy(buffer+queueIn,src,bufSize-queueIn);
					src += bufSize-queueIn;
				}
				else
					memset(buffer+queueIn,0,bufSize-queueIn);
				bytes -= bufSize-queueIn; // this could be zero
				queueIn = 0;
			}
		
			if (bytes > 0)
			{
				if (nullptr != src) 
					memcpy(buffer+queueIn,src,bytes);
				else
					memset(buffer+queueIn,0,bytes);
				
				queueIn += bytes;
			}
			
			if (queueIn == queueOut)
			{
				isFull = true;
				rv = full;
			}
			else if ((queueIn  < halfSize && queueOut >= halfSize)
				 ||  (queueOut < halfSize && queueIn  >= halfSize))
				rv = halfEmpty;
		}
		else
			rv = invalid; // buffer was not written, insufficient space was available
	}
	
	return rv;
}


/**
 * Check how much data remains in the buffer.
 * \return remaining data (bytes)
 */
size_t AudioBuffer::getAvailable()
{
	size_t rv = queueIn - queueOut;
	if (queueOut > queueIn)
		rv += bufSize;
	
	if (0 == rv && isFull)
		rv = bufSize;
	
	return rv;
}


/**
 * Get data on where to read next data into buffer.
 * Caller provides pointers to where the results should be. 
 *
 * The caller may then load the indicated zone with up to the required amount of data,
 * and must then call readExecuted() with the actual amount of data read into the buffer.
 *
 * If the required amount of data is 0 then no buffer read is needed.
 *
 * In rare circumstances only a half-buffer fill will be requested, even when 
 * the buffer is completely empty.
 *
 * \return true if size is more than half the entire buffer
 */
bool AudioBuffer::getNextRead(uint8_t** pbuf, 	//!< pointer to pointer returning buffer zone to fill next
								size_t* psz)	//!< pointer to available size of buffer zone to fill
{
	size_t sz = queueOut - queueIn;
	
	if (queueIn >= queueOut) // indexes have wrapped, unwrap
		sz += bufSize;
		
	if (queueIn + sz > bufSize) // read would exceed buffer, limit it
		sz = bufSize - queueIn;

	if (isFull && queueOut == queueIn) // no room at all
		sz = 0;
	
	*pbuf = buffer+queueIn;
	*psz  = sz;

	return sz > bufSize / 2;
}


/**
 * Signal that a required read into the buffer has been done.
 */
void AudioBuffer::readExecuted(size_t sz)
{
	queueIn += sz;
	if (queueIn >= bufSize)
		queueIn -= bufSize;
	
	if (queueIn == queueOut)
		isFull = true;
}


/**
 * Get data on where to write next data out of buffer.
 * Caller provides pointers to where the results should be. 
 *
 * The caller may then write out data from  the indicated zone with up to the available amount of data,
 * and must then call readExecuted() with the actual amount of data written from the buffer.
 *
 * If the required amount of data is 0 then no buffer write is needed.
 *
 * In rare circumstances only a half-buffer fill will be requested, even when 
 * the buffer is completely empty.
 *
 * \return true if size is more than half the entire buffer
 */
bool AudioBuffer::getNextWrite(uint8_t** pbuf, 	//!< pointer to pointer returning buffer zone to write from next
								size_t* psz)	//!< pointer to available size of buffer zone to write from
{
	size_t sz = queueIn - queueOut;
	
	if (queueIn < queueOut) // indexes have wrapped, unwrap
		sz += bufSize;
		
	if (0 == sz && isFull) // buffer is completely full
		sz = bufSize;
		
	if (queueOut + sz > bufSize) // write would exceed buffer, limit it
		sz = bufSize - queueOut;
	
	*pbuf = buffer+queueOut;
	*psz  = sz;
	
	return sz > bufSize / 2;
}


/**
 * Signal that a required write out of the buffer has been done.
 */
void AudioBuffer::writeExecuted(size_t sz)
{
	queueOut += sz;
	if (queueOut >= bufSize)
		queueOut -= bufSize;
	isFull = false;
}


/********************************************************************************/
/*
 * Constructor which creates buffer.
 */
AudioPreload::AudioPreload(AudioBuffer::bufType bt, size_t sz)
	: AudioPreload()
{
	createBuffer(sz,bt);
}


/*
 * Constructor which uses supplied buffer
 */
AudioPreload::AudioPreload(uint8_t* buf, size_t sz)
	: AudioPreload()
{
	createBuffer(buf,sz);
}


/*
 * Constructor which creates buffer and preloads file data.
 */
AudioPreload::AudioPreload(const char* fp, AudioBuffer::bufType bt, size_t sz, float startFrom /* = 0.0f */, FS& fs /* = SD */)
	: AudioPreload()
{
	preLoad(fp,bt,sz,startFrom,fs);
}


/*
 * Constructor which uses supplied buffer and preloads file data.
 */
AudioPreload::AudioPreload(const char* fp, uint8_t* buf, size_t sz, float startFrom /* = 0.0f */, FS& fs /* = SD */)
	: AudioPreload()
{
	preLoad(fp,buf,sz,startFrom,fs);
}

/*
 * Create buffer and pre-load file data
 */
AudioBuffer::result AudioPreload::preLoad(const char* fp, AudioBuffer::bufType bt, size_t sz, float startFrom /* = 0.0f */, FS& fs /* = SD */)
{
	result rv = createBuffer(sz, bt);
	if (ok == rv)
		rv = preLoad(fp,startFrom,fs);
	return rv;
}


/*
 * Use supplied buffer and pre-load file data
 */
AudioBuffer::result AudioPreload::preLoad(const char* fp, uint8_t* buf, size_t sz, float startFrom /* = 0.0f */, FS& fs /* = SD */)
{
	result rv = createBuffer(buf,sz);
	if (ok == rv)
		rv = preLoad(fp,startFrom,fs);
	return rv;
}


/*
 * Use existing buffer and pre-load file data
 */
AudioBuffer::result AudioPreload::preLoad(const char* fp, float startFrom /* = 0.0f */, FS& fs /* = SD */)
{
	result rv = invalid;
	File f = fs.open(fp);
	
	pFS = nullptr;
	filepath = nullptr;
	valid = 0;
	
	if (f)
	{
		AudioWAVdata wavd;
		
		if (wavd.parseWAVheader(f) > 0) // channel count >0, file is OK
		{
			size_t loadPos = wavd.firstAudio; // where audio starts in file
			size_t maxAudio = wavd.audioSize; // how much we could read into buffer
			
			// get ready for possible play() calls with non-zero startFrom
			sampleSize = wavd.chanCnt * wavd.bitsPerSample / 8;
			startSample = 0;
			chanCnt = wavd.chanCnt; // need to know this
			
			if (startFrom > 0.0f) // start later in file?
			{
				loadPos = wavd.millisToPosition(startFrom,AUDIO_SAMPLE_RATE); // read from here
				maxAudio -= loadPos - wavd.firstAudio; 		// can't read as much
				startSample = (loadPos - wavd.firstAudio) / sampleSize;	// make a note of start sample # (not file position!)
			}
			
			// load to sector boundary, or end of file if smaller than buffer
			size_t avail = bufSize; // total size of buffer
			size_t fpspace = strlen(fp)+1; 			// need this for copy of file path
			avail -=  fpspace; 						// this much space remains for data
			char* fpcopy = (char*) buffer+avail;	// record this for later
			fileOffset = loadPos + avail; 			// could get this much into the file...
			fileOffset &= -512;						// ...go only this far, so we hit sector boundary
			avail = fileOffset - loadPos;			// amount we want to buffer
			
			if (maxAudio < avail)  // there isn't that much audio in the file...
				avail = maxAudio;  // ...pretend we don't have the space
				
			if (avail > 0) // we do have something to buffer
			{
				f.seek(loadPos); 			// go to start of audio data
				if (avail == f.read(buffer,avail)) 	// and buffer it
				{
					strcpy(fpcopy,fp);  // copy file path to buffer memory
					filepath = fpcopy;  // keep a record of its location
					pFS = &fs;			// and the filesystem it's on
					valid = avail;		/// and how much data there is
					
					rv = ok; // success!
				}
			}
		}
		f.close();
	}
	
	return rv;
}

/********************************************************************************/
//#define CHARS_TO_ID(a,b,c,d) ((a<<24) | (b<<16) | (c<<8) | (d))
#define CHARS_TO_ID(a,b,c,d) ((d<<24) | (c<<16) | (b<<8) | (a))
static constexpr struct {
	  uint32_t RIFF,WAVE,fmt,data,fact;
} IDs = {CHARS_TO_ID('R','I','F','F'),
		 CHARS_TO_ID('W','A','V','E'),
		 CHARS_TO_ID('f','m','t',' '),
		 CHARS_TO_ID('d','a','t','a'),
		 CHARS_TO_ID('f','a','c','t')
		 };

uint32_t AudioWAVdata::getB2M(uint16_t chanCnt, uint32_t sampleRate, uint16_t bitsPerSample)
{
	uint32_t b2m;
	
	if (sampleRate == 44100) {
		b2m = B2M_44100;
	} else if (sampleRate == 22050) {
		b2m = B2M_22050;
	} else if (sampleRate == 11025) {
		b2m = B2M_11025;
	} else {
		b2m = B2M_44100; // wrong, but we should say something
	}
	
	b2m /= chanCnt;

	if (bitsPerSample == 16) 
		b2m >>= 1;
				
	return b2m;
}


/**
 * Parse a WAV file header, filling in the various class members.
 * Leaves the file pointer at 0, but firstAudio tells us where
 * audio data actually starts. This helps with ensuring reads
 * happen on 512-byte sector boundaries, which should be
 * maximally efficient.
 * \return channel count, or 0 if header didn't parse
 */
uint16_t AudioWAVdata::parseWAVheader(File& f)
{
	uint32_t seekTo;
	RIFFhdr_t rhdr = {0};
	ext_fmt_t dt = {0}; 
	uint16_t tmpChanCnt = 0; // audio update may occur, don't use class member
	
	samples = 0;
	dataChunks = 0;
	f.seek(0);
	f.read(&rhdr,sizeof rhdr);
	if (rhdr.riff.u == IDs.RIFF && rhdr.wav.u == IDs.WAVE)
	{
		do
		{
			uint32_t tmp;
			   
			f.read(&dt.fmt.hdr, sizeof dt.fmt.hdr);
			seekTo = f.position() + dt.fmt.hdr.clen;

			switch (dt.fmt.hdr.id.u)
			{
			  case IDs.fmt:
				f.read(&dt.fmt.fmt,dt.fmt.hdr.clen);

				format = dt.fmt.fmt;
				bitsPerSample = dt.fmt.bitsPerSample; 
				tmpChanCnt = dt.fmt.chanCnt;

				if (format == 0xFFFE) // extensible
				{
					format = dt.ext.subFmt;
				}

				bytes2millis = getB2M(tmpChanCnt,dt.fmt.sampleRate,bitsPerSample);
			  
				break;
			  
			  case IDs.data:
				samples += dt.fmt.hdr.clen / tmpChanCnt / (bitsPerSample / 8);
				if (0 == dataChunks++) // first data chunk: we only allow use of one
				{
					firstAudio = f.position(); 		// audio data starts here
					audioSize = dt.fmt.hdr.clen;	// and is this many bytes
				}
				break;
			  
			  case IDs.fact:
				f.read(&tmp,sizeof tmp);
				break;
		  }
		  f.seek(seekTo);
		} while (seekTo < rhdr.flen);
		
		if (0 == samples)	// no data?
			tmpChanCnt = 0;	// say there are no channels!
	}

	f.seek(0);

	// update playback channel count: this could cause mayhem if
	// somehow the preload and file channel counts are different,
	// but this should never happen!
	chanCnt = tmpChanCnt;
	
	return chanCnt;
}


void AudioWAVdata::makeWAVheader(wavhdr_t* wav,uint16_t chans, uint16_t fmt, uint16_t bits, uint32_t rate)
{
	*wav = {0};
	
	wav->riff.riff.u = IDs.RIFF; // ID is fixed
	wav->riff.wav.u = IDs.WAVE;	 // ID is fixed
	
	wav->fmt.hdr.id.u = IDs.fmt; // ID is fixed
	wav->fmt.hdr.clen = sizeof wav->fmt - sizeof wav->fmt.hdr; // we know the size of this chunk!
	wav->fmt.fmt = fmt;
	wav->fmt.chanCnt = chans;
	wav->fmt.sampleRate = rate;
	wav->fmt.byteRate = rate * chans * bits / 8;
	wav->fmt.blockAlign = chans * bits / 8;
	wav->fmt.bitsPerSample = bits;	
	
	wav->data.id.u = IDs.data; // ID is fixed
}


/*
 * Convert time in milliseconds to file position.
 * \return 0 if header values haven't been parsed, otherwise file position
 */
size_t AudioWAVdata::millisToPosition(float m,	// time [milliseconds]
									  float sr)	// sample rate [Hz]
{
	size_t pos = 0;
	
	if (bitsPerSample > 0) // simple check - could be better
	{
		pos = (size_t)(m / 1000.0f * sr); // samples from audio start
		pos *= chanCnt * bitsPerSample / 8; // convert to bytes
		pos += firstAudio; 		// and total file offset
	}
	
	return pos;
}