#include "Audio.h"
#include "arm_math.h"
#include "utility/dspinst.h"



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

void AudioPlaySDcardWAV::begin(void)
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


bool AudioPlaySDcardWAV::play(const char *filename)
{
	stop();
	wavfile = SD.open(filename);
	if (!wavfile) return false;
	buffer_remaining = 0;
	state_play = STATE_STOP;
	data_length = 0;
	state = STATE_PARSE1;
	return true;
}

void AudioPlaySDcardWAV::stop(void)
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
		wavfile.close();
	} else {
		__enable_irq();
	}
}

bool AudioPlaySDcardWAV::start(void)
{
	__disable_irq();
	if (state == STATE_STOP) {
		if (state_play == STATE_STOP) {
			__enable_irq();
			return false;
		}
		state = state_play;
	}
	__enable_irq();
	return true;
}


void AudioPlaySDcardWAV::update(void)
{
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
	if (buffer_remaining > 0) {
		// we have buffered data
		if (consume()) return; // it was enough to transmit audio
	}

	// we only get to this point when buffer[512] is empty
	if (state != STATE_STOP && wavfile.available()) {
		// we can read more data from the file...
		buffer_remaining = wavfile.read(buffer, 512);
		if (consume()) {
			// good, it resulted in audio transmit
			return;
		} else {
			// not good, no audio was transmitted
			buffer_remaining = 0;
			if (block_left) {
				release(block_left);
				block_left = NULL;
			}
			if (block_right) {
				release(block_right);
				block_right = NULL;
			}
			// if we're still playing, well, there's going to
			// be a gap in output, but we can't keep burning
			// time trying to read more data.  Hopefully things
			// will go better next time?
			if (state != STATE_STOP) return;
		}
	}
	// end of file reached or other reason to stop
	wavfile.close();
	if (block_left) {
		release(block_left);
		block_left = NULL;
	}
	if (block_right) {
		release(block_right);
		block_right = NULL;
	}
	state_play = STATE_STOP;
	state = STATE_STOP;
}


// https://ccrma.stanford.edu/courses/422/projects/WaveFormat/

// Consume already buffered data.  Returns true if audio transmitted.
bool AudioPlaySDcardWAV::consume(void)
{
	uint32_t len, size;
	uint8_t lsb, msb;
	const uint8_t *p;

	size = buffer_remaining;
	p = buffer + 512 - size;
start:
	if (size == 0) return false;
	//Serial.print("AudioPlaySDcardWAV write, size = ");
	//Serial.print(size);
	//Serial.print(", data_length = ");
	//Serial.print(data_length);
	//Serial.print(", state = ");
	//Serial.println(state);
	switch (state) {
	  // parse wav file header, is this really a .wav file?
	  case STATE_PARSE1:
		len = 20 - data_length;
		if (size < len) len = size;
		memcpy((uint8_t *)header + data_length, p, len);
		data_length += len;
		if (data_length < 20) return false;
		// parse the header...
		if (header[0] == 0x46464952 && header[2] == 0x45564157
		  && header[3] == 0x20746D66 && header[4] == 16) {
			//Serial.println("header ok");
			state = STATE_PARSE2;
			p += len;
			size -= len;
			data_length = 0;
			goto start;
		}
		//Serial.println("unknown WAV header");
		break;

	  // check & extract key audio parameters
	  case STATE_PARSE2:
		len = 16 - data_length;
		if (size < len) len = size;
		memcpy((uint8_t *)header + data_length, p, len);
		data_length += len;
		if (data_length < 16) return false;
		if (parse_format()) {
			//Serial.println("audio format ok");
			p += len;
			size -= len;
			data_length = 0;
			state = STATE_PARSE3;
			goto start;
		}
		//Serial.println("unknown audio format");
		break;

	  // find the data chunk
	  case STATE_PARSE3:
		len = 8 - data_length;
		if (size < len) len = size;
		memcpy((uint8_t *)header + data_length, p, len);
		data_length += len;
		if (data_length < 8) return false;
		//Serial.print("chunk id = ");
		//Serial.print(header[0], HEX);
		//Serial.print(", length = ");
		//Serial.println(header[1]);
		p += len;
		size -= len;
		data_length = header[1];
		if (header[0] == 0x61746164) {
			//Serial.println("found data chunk");
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
		} else {
			state = STATE_PARSE4;
		}
		goto start;

	  // ignore any extra unknown chunks (title & artist info)
	  case STATE_PARSE4:
		if (size < data_length) {
			data_length -= size;
			return false;
		}
		p += data_length;
		size -= data_length;
		data_length = 0;
		state = STATE_PARSE3;
		goto start;

	  // playing mono at native sample rate
	  case STATE_DIRECT_8BIT_MONO:
		return false;

	  // playing stereo at native sample rate
	  case STATE_DIRECT_8BIT_STEREO:
		return false;

	  // playing mono at native sample rate
	  case STATE_DIRECT_16BIT_MONO:
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
				 //Serial1.print('%');
				 //delayMicroseconds(90);
				release(block_left);
				block_left = NULL;
				data_length += size;
				buffer_remaining = size;
				if (block_right) release(block_right);
				return true;
			}
			if (size == 0) {
				if (data_length == 0) break;
				return false;
			}
		}
		// end of file reached
		if (block_offset > 0) {
			// TODO: fill remainder of last block with zero and transmit
		}
		state = STATE_STOP;
		return false;

	  // playing stereo at native sample rate
	  case STATE_DIRECT_16BIT_STEREO:
		if (size > data_length) size = data_length;
		data_length -= size;
		if (leftover_bytes) {
			block_left->data[block_offset] = header[0];
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
				buffer_remaining = size;
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
		return false;

	  // playing stereo, converting sample rate
	  case STATE_CONVERT_8BIT_STEREO:
		return false;

	  // playing mono, converting sample rate
	  case STATE_CONVERT_16BIT_MONO:
		return false;

	  // playing stereo, converting sample rate
	  case STATE_CONVERT_16BIT_STEREO:
		return false;

	  // ignore any extra data after playing
	  // or anything following any error
	  case STATE_STOP:
		return false;

	  // this is not supposed to happen!
	  //default:
		//Serial.println("AudioPlaySDcardWAV, unknown state");
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





bool AudioPlaySDcardWAV::parse_format(void)
{
	uint8_t num = 0;
	uint16_t format;
	uint16_t channels;
	uint32_t rate;
	uint16_t bits;

	format = header[0];
	//Serial.print("  format = ");
	//Serial.println(format);
	if (format != 1) return false;

	channels = header[0] >> 16;
	//Serial.print("  channels = ");
	//Serial.println(channels);
	if (channels == 1) {
	} else if (channels == 2) {
		num = 1;
	} else {
		return false;
	}

	bits = header[3] >> 16;
	//Serial.print("  bits = ");
	//Serial.println(bits);
	if (bits == 8) {
	} else if (bits == 16) {
		num |= 2;
	} else {
		return false;
	}

	rate = header[1];
	//Serial.print("  rate = ");
	//Serial.println(rate);
	if (rate == AUDIO_SAMPLE_RATE) {
	} else if (rate >= 8000 && rate <= 48000) {
		num |= 4;
	} else {
		return false;
	}
	// we're not checking the byte rate and block align fields
	// if they're not the expected values, all we could do is
	// return false.  Do any real wav files have unexpected
	// values in these other fields?
	state_play = num;
	return true;
}


