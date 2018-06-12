/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 * Modified to use SerialFlash instead of SD library by Wyatt Olson <wyatt@digitalcave.ca>
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

 /* This is my proposal to add "pitch" functionality. All new lines/modification are commented with // ***** <text>
 *  Sandro Grassia, sandro.grassia@gmail.com.
 */

#include <Arduino.h>
#include "play_serialflash_raw.h"
#include "spi_interrupt.h"

int32_t ride;   					// ***** number of time update() is called
int32_t first_index_to_read, last_index_to_read; 	// ***** indexes of samples
int16_t penultimate_sample, last_sample;  		// ***** values of samples
int64_t t, t_0;						// ***** t  is the computational time (in microseconds) of the new code, should be less than 2902 microseconds (period of update() when 128 samples are required)
float pitch;						// ***** pitch value

void AudioPlaySerialflashRaw::begin(void)
{
	playing = false;
	file_offset = 0;
	file_size = 0;
}


bool AudioPlaySerialflashRaw::play(const char *filename, float pitch_value)
{
	stop();
	ride=0; // ***** set first ride
	pitch= ((pitch_value>=0.05)? ((pitch_value<=10.0)? pitch_value : 10.0) : 0.05) ; // ***** set pitch value within 0.05 and 10.0
	AudioStartUsingSPI();
	rawfile = SerialFlash.open(filename);
	if (!rawfile) {
		//Serial.println("unable to open file");
		AudioStopUsingSPI();
		return false;
	}
	file_size = rawfile.size();
	file_offset = 0;
	//Serial.println("able to open file");
	playing = true;
	return true;
}

void AudioPlaySerialflashRaw::stop(void)
{
	__disable_irq();
	if (playing) {
		playing = false;
		__enable_irq();
		rawfile.close();
		AudioStopUsingSPI();
	} else {
		__enable_irq();
	}
}


void AudioPlaySerialflashRaw::update(void)
{
	unsigned int i, n;
	audio_block_t *block;
	int16_t samples_vector[2000]; 		// ***** current vector of samples read from file	
	int32_t h, k, j; 			// ***** indexes of samples
	float virtual_index; 			// ***** likely not integer index
	float index_delta; 			// ***** mantissa of virtual_index
	int16_t h_sample_value, k_sample_value; // ***** samples value
	int f, g, w; 				// ***** variables
	int16_t samples_to_read; 		// ***** number of samples

	// only update if we're playing
	if (!playing) return;

	// allocate the audio blocks to transmit
	block = allocate();
	if (block == NULL) return;

	if (rawfile.available()) {

		// ***** my main code starts here
		t_0=micros(); // ***** calculating the computational time
		first_index_to_read=((ride==0)? 0 :last_index_to_read+1);
		last_index_to_read=ceil(((ride*128)+127)*pitch);
		samples_to_read=last_index_to_read-first_index_to_read+1;

		// ***** this part of code reads from file up to 128 samples per time; if we read more than 128 samples there are errors!. The result is the samples_vector[samples_to_read]			
		f=samples_to_read%128;
		g=samples_to_read/128;
		
		// ***** read one group of "f" samples
		n = rawfile.read(block->data, f*2); 
		file_offset += n;
		for (i=n/2; i < f; i++) {
			block->data[i] = 0;
		}
		for (i=0; i < f; i++) {
			samples_vector[i]=block->data[i];
		}
		
		// ***** read "g" groups of 128 samples
		for (w=0; w<g; w++)	{ 
	  		n = rawfile.read(block->data, 128*2); // ***** read one group of f samples
			file_offset += n;
			for (i=n/2; i < 128; i++) {
				block->data[i] = 0;
			}
			for (i=0; i < 128; i++) {
				samples_vector[f+(w*128)+i]=block->data[i];
			}
		}
		// ***** samples_vector[samples_to_read] is ready!	
		
		// ***** calculating block->data[128]
		for (i=0;i<128;i++) {
			j=i+(128*ride);
			virtual_index=(float)(j)*pitch; // ***** virtual (it means that can be not-integer) index of the needed sample
			h=floor(virtual_index); 		// ***** index of the lower sample needed for calculation
			k=ceil(virtual_index);  		// ***** index of the upper sample needed for calculation
			index_delta=virtual_index-h;  	// ***** mantissa
			h=h-first_index_to_read; 		// ***** lower (relative) index of the sample needed for calculation 
			k=k-first_index_to_read;	 	// ***** upper (relative) index of the sample needed for calculation
			h_sample_value =((h==-2)? penultimate_sample: ((h==-1)? last_sample: samples_vector[h])); 	// ***** value of the lower sample needed for calculation
			k_sample_value =((k==-2)? penultimate_sample: ((k==-1)? last_sample: samples_vector[k])); 	// ***** value of the upper sample needed for calculation
			block->data[i]=h_sample_value + (index_delta*(k_sample_value-h_sample_value)); 				// ***** value of the calculated sample
		}		
		penultimate_sample=samples_vector[k-1]; 	// ***** value of the penultimate sample read from file will be used in the next ride
		last_sample=samples_vector[k]; 				// ***** value of the last sample read from file will be used in the next ride
		ride ++;
		t=micros()-t_0; // ***** calculating the computational time "t"
		// ***** my main code ends here
				
		transmit(block);
	}

	else {
		rawfile.close();
		AudioStopUsingSPI();
		playing = false;
		//Serial.println("Finished playing sample");		//TODO
	}
	release(block);
}

#define B2M (uint32_t)((double)4294967296000.0 / AUDIO_SAMPLE_RATE_EXACT / 2.0) // 97352592

uint32_t AudioPlaySerialflashRaw::positionMillis(void)
{
	return ((uint64_t)file_offset * B2M) >> 32;
}

uint32_t AudioPlaySerialflashRaw::lengthMillis(void)
{
	return ((uint64_t)file_size * B2M) >> 32;
}

int AudioPlaySerialflashRaw::time_lapse(void) // ***** reporting the computational time "t"
{
	return t;
}
