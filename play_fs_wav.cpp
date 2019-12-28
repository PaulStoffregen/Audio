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

#if defined(ARDUINO_ARCH_SAMD)
#include <Arduino.h>
#endif

#include "play_fs_wav.h"
#include "spi_interrupt.h"

// File system object.
extern SdFat SD;


#define STATE_DIRECT_8BIT_MONO		0  // playing mono at native sample rate
#define STATE_DIRECT_8BIT_STEREO	1  // playing stereo at native sample rate
#define STATE_DIRECT_16BIT_MONO		2  // playing mono at native sample rate
#define STATE_DIRECT_16BIT_STEREO	3  // playing stereo at native sample rate
#define STATE_CONVERT_8BIT_MONO		4  // playing mono, converting sample rate
#define STATE_CONVERT_8BIT_STEREO	5  // playing stereo, converting sample rate
#define STATE_CONVERT_16BIT_MONO	6  // playing mono, converting sample rate
#define STATE_CONVERT_16BIT_STEREO	7  // playing stereo, converting sample rate
#define STATE_PARSE1			8  // looking for 20 byte ID header
#define STATE_PARSE2			9  // looking for 16 byte format header
#define STATE_PARSE3			10 // looking for 8 byte data header
#define STATE_PARSE4			11 // ignoring unknown chunk
#define STATE_STOP			12

void AudioPlayFSWav::begin(void)
{
	state = STATE_STOP;
	state_play = STATE_STOP;
	data_length = 0;
	if (block_left) {
		release(block_left);
		block_left = NULL;
	}
	if (block_right) {
		release(block_right);
		block_right = NULL;
	}
}


bool AudioPlayFSWav::play(File wavfile)
{
	stop();
        #if defined(HAS_KINETIS_SDHC)	
	if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStartUsingSPI();
        #else 	
	AudioStartUsingSPI();
        #endif
	if (!wavfile) {
	#if defined(HAS_KINETIS_SDHC)	
		if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStopUsingSPI();
	#else 	
		AudioStopUsingSPI();
	#endif			
		return false;
	}
	_wavfile = wavfile;
	buffer_length = 0;
	buffer_offset = 0;
	state_play = STATE_STOP;
	data_length = 20;
	header_offset = 0;
	state = STATE_PARSE1;
	return true;
}

void AudioPlayFSWav::stop(void)
{
	__disable_irq();
	if (state != STATE_STOP) {
		audio_block_t *b1 = block_left;
		block_left = NULL;
		audio_block_t *b2 = block_right;
		block_right = NULL;
		state = STATE_STOP;
		__enable_irq();
		if (b1) release(b1);
		if (b2) release(b2);
		_wavfile.close();
	#if defined(HAS_KINETIS_SDHC)	
		if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStopUsingSPI();
	#else 	
		AudioStopUsingSPI();
	#endif	
	} else {
		__enable_irq();
	}
}


void AudioPlayFSWav::update(void)
{
	int32_t n;

	// only update if we're playing
	if (state == STATE_STOP) return;

	// allocate the audio blocks to transmit
	block_left = allocate();
	if (block_left == NULL) return;
	if (state < 8 && (state & 1) == 1) {
		// if we're playing stereo, allocate another
		// block for the right channel output
		block_right = allocate();
		if (block_right == NULL) {
			release(block_left);
			return;
		}
	} else {
		// if we're playing mono or just parsing
		// the WAV file header, no right-side block
		block_right = NULL;
	}
	block_offset = 0;

	//Serial.println("update");

	// is there buffered data?
	n = buffer_length - buffer_offset;
	if (n > 0) {
		// we have buffered data
		if (consume(n)) return; // it was enough to transmit audio
	}

	// we only get to this point when buffer[512] is empty
	if (state != STATE_STOP && _wavfile.available()) {
		// we can read more data from the file...
		readagain:
		buffer_length = _wavfile.read(buffer, 512);
		if (buffer_length == 0) goto end;
		buffer_offset = 0;
		bool parsing = (state >= 8);
		bool txok = consume(buffer_length);
		if (txok) {
			if (state != STATE_STOP) return;
		} else {
			if (state != STATE_STOP) {
				if (parsing && state < 8) goto readagain;
				else goto cleanup;
			}
		}
	}
end:	// end of file reached or other reason to stop
	_wavfile.close();
#if defined(HAS_KINETIS_SDHC)	
	if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStopUsingSPI();
#else 	
	AudioStopUsingSPI();
#endif	
	state_play = STATE_STOP;
	state = STATE_STOP;
cleanup:
	if (block_left) {
		if (block_offset > 0) {
			for (uint32_t i=block_offset; i < AUDIO_BLOCK_SAMPLES; i++) {
				block_left->data[i] = 0;
			}
			transmit(block_left, 0);
			if (state < 8 && (state & 1) == 0) {
				transmit(block_left, 1);
			}
		}
		release(block_left);
		block_left = NULL;
	}
	if (block_right) {
		if (block_offset > 0) {
			for (uint32_t i=block_offset; i < AUDIO_BLOCK_SAMPLES; i++) {
				block_right->data[i] = 0;
			}
			transmit(block_right, 1);
		}
		release(block_right);
		block_right = NULL;
	}
}


// https://ccrma.stanford.edu/courses/422/projects/WaveFormat/

// Consume already buffered data.  Returns true if audio transmitted.
bool AudioPlayFSWav::consume(uint32_t size)
{
	uint32_t len;
	uint8_t lsb, msb;
	const uint8_t *p;

	p = buffer + buffer_offset;
start:
	if (size == 0) return false;
#if 0
	Serial.print("AudioPlaySdWav consume, ");
	Serial.print("size = ");
	Serial.print(size);
	Serial.print(", buffer_offset = ");
	Serial.print(buffer_offset);
	Serial.print(", data_length = ");
	Serial.print(data_length);
	Serial.print(", space = ");
	Serial.print((AUDIO_BLOCK_SAMPLES - block_offset) * 2);
	Serial.print(", state = ");
	Serial.println(state);
#endif
	switch (state) {
	  // parse wav file header, is this really a .wav file?
	  case STATE_PARSE1:
		len = data_length;
		if (size < len) len = size;
		memcpy((uint8_t *)header + header_offset, p, len);
		header_offset += len;
		buffer_offset += len;
		data_length -= len;
		if (data_length > 0) return false;
		// parse the header...
		if (header[0] == 0x46464952 && header[2] == 0x45564157
		  && header[3] == 0x20746D66 && header[4] >= 16) {
			if (header[4] > sizeof(header)) {
				// if such .wav files exist, increasing the
				// size of header[] should accomodate them...
				//Serial.println("WAVEFORMATEXTENSIBLE too long");
				break;
			}
			//Serial.println("header ok");
			p += len;
			size -= len;
			data_length = header[4];
			header_offset = 0;
			state = STATE_PARSE2;
			goto start;
		}
		//Serial.println("unknown WAV header");
		break;

	  // check & extract key audio parameters
	  case STATE_PARSE2:
		len = data_length;
		if (size < len) len = size;
		memcpy((uint8_t *)header + header_offset, p, len);
		header_offset += len;
		buffer_offset += len;
		data_length -= len;
		if (data_length > 0) return false;
		if (parse_format()) {
			//Serial.println("audio format ok");
			p += len;
			size -= len;
			data_length = 8;
			header_offset = 0;
			state = STATE_PARSE3;
			goto start;
		}
		//Serial.println("unknown audio format");
		break;

	  // find the data chunk
	  case STATE_PARSE3: // 10
		len = data_length;
		if (size < len) len = size;
		memcpy((uint8_t *)header + header_offset, p, len);
		header_offset += len;
		buffer_offset += len;
		data_length -= len;
		if (data_length > 0) return false;
		//Serial.print("chunk id = ");
		//Serial.print(header[0], HEX);
		//Serial.print(", length = ");
		//Serial.println(header[1]);
		p += len;
		size -= len;
		data_length = header[1];
		if (header[0] == 0x61746164) {
			//Serial.print("wav: found data chunk, len=");
			//Serial.println(data_length);
			// TODO: verify offset in file is an even number
			// as required by WAV format.  abort if odd.  Code
			// below will depend upon this and fail if not even.
			leftover_bytes = 0;
			state = state_play;
			if (state & 1) {
				// if we're going to start stereo
				// better allocate another output block
				block_right = allocate();
				if (!block_right) return false;
			}
			total_length = data_length;
		} else {
			state = STATE_PARSE4;
		}
		goto start;

	  // ignore any extra unknown chunks (title & artist info)
	  case STATE_PARSE4: // 11
		if (size < data_length) {
			data_length -= size;
			buffer_offset += size;
			return false;
		}
		p += data_length;
		size -= data_length;
		buffer_offset += data_length;
		data_length = 8;
		header_offset = 0;
		state = STATE_PARSE3;
		//Serial.println("consumed unknown chunk");
		goto start;

	  // playing mono at native sample rate
	  case STATE_DIRECT_8BIT_MONO:
	    //Serial.println("8 bit mono");
	    return false;

	  // playing stereo at native sample rate
	  case STATE_DIRECT_8BIT_STEREO:
	    //Serial.println("8 bit stereo");
	    return false;

	  // playing mono at native sample rate
	  case STATE_DIRECT_16BIT_MONO:
	    //Serial.println("16 bit mono");
		if (size > data_length) size = data_length;
		data_length -= size;
		while (1) {
			lsb = *p++;
			msb = *p++;
			size -= 2;
			block_left->data[block_offset++] = (msb << 8) | lsb;
			if (block_offset >= AUDIO_BLOCK_SAMPLES) {
				transmit(block_left, 0);
				transmit(block_left, 1);
				release(block_left);
				block_left = NULL;
				data_length += size;
				buffer_offset = p - buffer;
				if (block_right) release(block_right);
				if (data_length == 0) state = STATE_STOP;
				return true;
			}
			if (size == 0) {
				if (data_length == 0) break;
				return false;
			}
		}
		//Serial.println("end of file reached");
		// end of file reached
		if (block_offset > 0) {
			// TODO: fill remainder of last block with zero and transmit
		}
		state = STATE_STOP;
		return false;

	  // playing stereo at native sample rate
	  case STATE_DIRECT_16BIT_STEREO:
	        //Serial.println("16 bit stereo");
		if (size > data_length) size = data_length;
		data_length -= size;
		if (leftover_bytes) {
			block_left->data[block_offset] = header[0];
//PAH fix problem with left+right channels being swapped
			leftover_bytes = 0;
			goto right16;
		}
		while (1) {
			lsb = *p++;
			msb = *p++;
			size -= 2;
			if (size == 0) {
				if (data_length == 0) break;
				header[0] = (msb << 8) | lsb;
				leftover_bytes = 2;
				return false;
			}
			block_left->data[block_offset] = (msb << 8) | lsb;
			right16:
			lsb = *p++;
			msb = *p++;
			size -= 2;
			block_right->data[block_offset++] = (msb << 8) | lsb;
			if (block_offset >= AUDIO_BLOCK_SAMPLES) {
				transmit(block_left, 0);
				release(block_left);
				block_left = NULL;
				transmit(block_right, 1);
				release(block_right);
				block_right = NULL;
				data_length += size;
				buffer_offset = p - buffer;
				if (data_length == 0) state = STATE_STOP;
				return true;
			}
			if (size == 0) {
				if (data_length == 0) break;
				leftover_bytes = 0;
				return false;
			}
		}
		// end of file reached
		if (block_offset > 0) {
			// TODO: fill remainder of last block with zero and transmit
		}
		state = STATE_STOP;
		return false;

	  // playing mono, converting sample rate
	  case STATE_CONVERT_8BIT_MONO :
	    //Serial.println("convert 8 bit mono");
		return false;

	  // playing stereo, converting sample rate
	  case STATE_CONVERT_8BIT_STEREO:
	    //Serial.println("convert 8 bit stereo");
		return false;

	  // playing mono, converting sample rate
	  case STATE_CONVERT_16BIT_MONO:
	    //Serial.println("convert 16 bit mono");
		return false;

	  // playing stereo, converting sample rate
	  case STATE_CONVERT_16BIT_STEREO:
	    //Serial.println("convert 16 bit stereo");
		return false;

	  // ignore any extra data after playing
	  // or anything following any error
	  case STATE_STOP:
		return false;

	  // this is not supposed to happen!
	  //default:
		//Serial.println("AudioPlaySdWav, unknown state");
	}
	state_play = STATE_STOP;
	state = STATE_STOP;
	return false;
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





// SD library on Teensy3 at 96 MHz
//  256 byte chunks, speed is 443272 bytes/sec
//  512 byte chunks, speed is 468023 bytes/sec

#define B2M_44100 (uint32_t)((double)4294967296000.0 / AUDIO_SAMPLE_RATE_EXACT) // 97352592
#define B2M_22050 (uint32_t)((double)4294967296000.0 / AUDIO_SAMPLE_RATE_EXACT * 2.0)
#define B2M_11025 (uint32_t)((double)4294967296000.0 / AUDIO_SAMPLE_RATE_EXACT * 4.0)

bool AudioPlayFSWav::parse_format(void)
{
	uint8_t num = 0;
	uint16_t format;
	uint16_t channels;
	uint32_t rate, b2m;
	uint16_t bits;

	format = header[0];
	//Serial.print("  format = ");
	//Serial.println(format);
	if (format != 1) return false;

	rate = header[1];
	//Serial.print("  rate = ");
	//Serial.println(rate);
	if (rate == 44100) {
		b2m = B2M_44100;
	} else if (rate == 22050) {
		b2m = B2M_22050;
		num |= 4;
	} else if (rate == 11025) {
		b2m = B2M_11025;
		num |= 4;
	} else {
		return false;
	}

	channels = header[0] >> 16;
	//Serial.print("  channels = ");
	//Serial.println(channels);
	if (channels == 1) {
	} else if (channels == 2) {
		b2m >>= 1;
		num |= 1;
	} else {
		return false;
	}

	bits = header[3] >> 16;
	//Serial.print("  bits = ");
	//Serial.println(bits);
	if (bits == 8) {
	} else if (bits == 16) {
		b2m >>= 1;
		num |= 2;
	} else {
		return false;
	}

	bytes2millis = b2m;
	//Serial.print("  bytes2millis = ");
	//Serial.println(b2m);

	// we're not checking the byte rate and block align fields
	// if they're not the expected values, all we could do is
	// return false.  Do any real wav files have unexpected
	// values in these other fields?
	state_play = num;
	return true;
}


bool AudioPlayFSWav::isPlaying(void)
{
	uint8_t s = *(volatile uint8_t *)&state;
	return (s < 8);
}

uint32_t AudioPlayFSWav::positionMillis(void)
{
	uint8_t s = *(volatile uint8_t *)&state;
	if (s >= 8) return 0;
	uint32_t tlength = *(volatile uint32_t *)&total_length;
	uint32_t dlength = *(volatile uint32_t *)&data_length;
	uint32_t offset = tlength - dlength;
	uint32_t b2m = *(volatile uint32_t *)&bytes2millis;
	return ((uint64_t)offset * b2m) >> 32;
}


uint32_t AudioPlayFSWav::lengthMillis(void)
{
	uint8_t s = *(volatile uint8_t *)&state;
	if (s >= 8) return 0;
	uint32_t tlength = *(volatile uint32_t *)&total_length;
	uint32_t b2m = *(volatile uint32_t *)&bytes2millis;
	return ((uint64_t)tlength * b2m) >> 32;
}

