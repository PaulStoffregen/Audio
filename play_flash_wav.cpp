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

#include "play_flash_wav.h"
#include "utility/dspinst.h"
#include "flash_spi.h"

void AudioPlayFlashWAV::play(unsigned int start_page,unsigned int num_pages)
{
	n_pages = num_pages;
	c_page = start_page;
	playing = 1;
}

void AudioPlayFlashWAV::stop(void)
{
	playing = 0;
}


void AudioPlayFlashWAV::update(void)
{
	audio_block_t *l_block;
	audio_block_t *r_block;
	uint16_t *l_bl,*r_bl;
	uint16_t *f_buf;
	int i;

	if (!playing) return;
	// For now, only handle stereo samples		
	l_block = allocate();
	if(l_block == NULL)return;
	r_block = allocate();
	if(r_block == NULL) {
		release(l_block);
		return;
	}
	// This reads two pages (512 bytes) starting at the specified page
	// flash_fast_read sometimes glitches - for now, don't use it
	flash_read_pages((unsigned char *)f_buffer,c_page,2);
	f_buf = f_buffer;
	l_bl = (uint16_t *)&l_block->data[0];
	r_bl = (uint16_t *)&r_block->data[0];
	for(i = 0;i < 128;i++) {
		*l_bl++ = *f_buf++;
		*r_bl++ = *f_buf++;
	}
	c_page += 2;
	n_pages -= 2;
	if(n_pages <= 0)playing = 0;
	transmit(l_block,0);
	transmit(r_block,1);
	release(l_block);
	release(r_block);
}
