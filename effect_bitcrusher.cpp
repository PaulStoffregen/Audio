/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Jonathan Payne (jon@jonnypayne.com)
 * Based on Effect_Fade by Paul Stoffregen
 * Also samplerate reduction based on Pete Brown's bitcrusher here: 
 * http://10rem.net/blog/2013/01/13/a-simple-bitcrusher-and-sample-rate-reducer-in-cplusplus-for-a-windows-store-app
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

#include "effect_bitcrusher.h"

void AudioEffectBitCrusher::update(void)
{
	audio_block_t *block;
	uint32_t i,j;
	uint32_t sampleSquidge, sampleSqueeze; //squidge is bitdepth, squeeze is for samplerate
			
	if (crushBits == 16 && sampleRate == 44100) { // nothing to do. Output is sent through clean, then exit the function
		block = receiveReadOnly();
		if (!block) return;
		transmit(block);
		release(block);
		return;
		}
	else { // start of processing functions. Could be more elegant based on external functions but left like this to enable code optimisation later.
	block = receiveWritable();
	if (!block) return;
	
	if (sampleRate == 44100) 	{ //no sample rate mods, just crush the bitdepth.
		for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
			sampleSquidge = block->data[i] >> (16-crushBits); // shift bits right to cut off fine detail sampleSquidge is a uint32 so sign extension will not occur, fills with zeroes.
			 block->data[i] = sampleSquidge << (16-crushBits); // shift bits left again to regain the volume level. fills with zeroes.
		}
		transmit(block);
		release(block);
	}
	else

	if (crushBits == 16) { //bitcrusher not being used, samplerate mods only.
		i=0;
		while (i < AUDIO_BLOCK_SAMPLES) {
		sampleSqueeze = block->data[i]; //save the root sample. this will pick up a root sample every _sampleStep_ samples.
		for (int j = 0; j < sampleStep && i < AUDIO_BLOCK_SAMPLES; j++)
        {
			block->data[i] = sampleSqueeze; // for each repeated sample, paste in the current root sample, then move onto the next step.
            i++;
        }
		}
		transmit(block);
		release(block);
	}
	else {           //both being used. crush those bits and mash those samples.
		i=0;
		while (i < AUDIO_BLOCK_SAMPLES) {
		sampleSqueeze = block->data[i]; //save the root sample. this will pick up a root sample every _sampleStep_ samples.
			for (int j = 0; j < sampleStep && i < AUDIO_BLOCK_SAMPLES; j++)
			{
				sampleSquidge = sampleSqueeze >> (16-crushBits); // shift bits right to cut off fine detail sampleSquidge is a uint32 so sign extension will not occur, fills with zeroes.
				block->data[i] = sampleSquidge << (16-crushBits); // shift bits left again to regain the volume level. fills with zeroes. paste into buffer sample + sampleStep offset.
				i++;
			}
		}

		transmit(block);
		release(block);
	}
	} //end 

}

		

void AudioEffectBitCrusher::updateCrush(uint8_t xxcrushBits, uint32_t xxsampleRate )
{
	__disable_irq();
	if (xxcrushBits >= 16) //too high a bitdepth or specifying full 16 bit range?
	{	
		crushBits = 16; // this will simply passthrough the crusher clean
	}
	else
	if(xxcrushBits >= 1) // anywhere between 1 and 15, send it through to the Squidger.
	{
	crushBits = xxcrushBits;
	}
	else
	{
		crushBits =1; // now lets be honest, crushing to 0 bits is going to be a mute command. send a 1 through instead.
	} 

	if(xxsampleRate >=44100)// maximum sample rate to be 44100
	{
		sampleRate = 44100; //thats that
	}
	else
	if(xxsampleRate >= 1) // anywhere between 1 and 44100, send it through to the Squeezer.
	{
		sampleRate = xxsampleRate; 
		sampleStep = 44100 / sampleRate; // calcualte the number of samples to duplicate as we reduce the samplerate.
	}
	else
	{
		sampleRate = 1;
		sampleStep = 44100 / sampleRate; // calcualte the number of samples to duplicate as we reduce the samplerate.
	}
					
	__enable_irq();
}



