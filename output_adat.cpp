/* ADAT for Teensy 3.X
 * Copyright (c) 2017, Ernstjan Freriks,
 * Thanks to Frank Bösing & KPC & Paul Stoffregen!
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
#include "output_adat.h"

#if defined(KINETISK)

audio_block_t * AudioOutputADAT::block_ch1_1st = NULL;
audio_block_t * AudioOutputADAT::block_ch2_1st = NULL;
audio_block_t * AudioOutputADAT::block_ch3_1st = NULL;
audio_block_t * AudioOutputADAT::block_ch4_1st = NULL;
audio_block_t * AudioOutputADAT::block_ch5_1st = NULL;
audio_block_t * AudioOutputADAT::block_ch6_1st = NULL;
audio_block_t * AudioOutputADAT::block_ch7_1st = NULL;
audio_block_t * AudioOutputADAT::block_ch8_1st = NULL;

audio_block_t * AudioOutputADAT::block_ch1_2nd = NULL;
audio_block_t * AudioOutputADAT::block_ch2_2nd = NULL;
audio_block_t * AudioOutputADAT::block_ch3_2nd = NULL;
audio_block_t * AudioOutputADAT::block_ch4_2nd = NULL;
audio_block_t * AudioOutputADAT::block_ch5_2nd = NULL;
audio_block_t * AudioOutputADAT::block_ch6_2nd = NULL;
audio_block_t * AudioOutputADAT::block_ch7_2nd = NULL;
audio_block_t * AudioOutputADAT::block_ch8_2nd = NULL;

uint16_t  AudioOutputADAT::ch1_offset = 0;
uint16_t  AudioOutputADAT::ch2_offset = 0;
uint16_t  AudioOutputADAT::ch3_offset = 0;
uint16_t  AudioOutputADAT::ch4_offset = 0;
uint16_t  AudioOutputADAT::ch5_offset = 0;
uint16_t  AudioOutputADAT::ch6_offset = 0;
uint16_t  AudioOutputADAT::ch7_offset = 0;
uint16_t  AudioOutputADAT::ch8_offset = 0;

bool AudioOutputADAT::update_responsibility = false;
//uint32_t  AudioOutputADAT::vucp = VUCP_VALID;

DMAMEM static uint32_t ADAT_tx_buffer[AUDIO_BLOCK_SAMPLES * 8]; //4 KB, AUDIO_BLOCK_SAMPLES is usually 128

DMAChannel AudioOutputADAT::dma(false);

static const uint32_t zerodata[AUDIO_BLOCK_SAMPLES/4] = {0};

// These are the lookup tables. There are four of them so that the remainder of the 32 bit result can easily be XORred with the next 8 bits.
static const uint32_t LookupTable_firstword[256]  = { 0x0, 0x1ffffff, 0x3ffffff, 0x2000000, 0x7ffffff, 0x6000000, 0x4000000, 0x5ffffff, 0xfffffff, 0xe000000, 0xc000000, 0xdffffff, 0x8000000, 0x9ffffff, 0xbffffff, 0xa000000, 0x1fffffff, 0x1e000000, 0x1c000000, 0x1dffffff, 0x18000000, 0x19ffffff, 0x1bffffff, 0x1a000000, 0x10000000, 0x11ffffff, 0x13ffffff, 0x12000000, 0x17ffffff, 0x16000000, 0x14000000, 0x15ffffff, 0x3fffffff, 0x3e000000, 0x3c000000, 0x3dffffff, 0x38000000, 0x39ffffff, 0x3bffffff, 0x3a000000, 0x30000000, 0x31ffffff, 0x33ffffff, 0x32000000, 0x37ffffff, 0x36000000, 0x34000000, 0x35ffffff, 0x20000000, 0x21ffffff, 0x23ffffff, 0x22000000, 0x27ffffff, 0x26000000, 0x24000000, 0x25ffffff, 0x2fffffff, 0x2e000000, 0x2c000000, 0x2dffffff, 0x28000000, 0x29ffffff, 0x2bffffff, 0x2a000000, 0x7fffffff, 0x7e000000, 0x7c000000, 0x7dffffff, 0x78000000, 0x79ffffff, 0x7bffffff, 0x7a000000, 0x70000000, 0x71ffffff, 0x73ffffff, 0x72000000, 0x77ffffff, 0x76000000, 0x74000000, 0x75ffffff, 0x60000000, 0x61ffffff, 0x63ffffff, 0x62000000, 0x67ffffff, 0x66000000, 0x64000000, 0x65ffffff, 0x6fffffff, 0x6e000000, 0x6c000000, 0x6dffffff, 0x68000000, 0x69ffffff, 0x6bffffff, 0x6a000000, 0x40000000, 0x41ffffff, 0x43ffffff, 0x42000000, 0x47ffffff, 0x46000000, 0x44000000, 0x45ffffff, 0x4fffffff, 0x4e000000, 0x4c000000, 0x4dffffff, 0x48000000, 0x49ffffff, 0x4bffffff, 0x4a000000, 0x5fffffff, 0x5e000000, 0x5c000000, 0x5dffffff, 0x58000000, 0x59ffffff, 0x5bffffff, 0x5a000000, 0x50000000, 0x51ffffff, 0x53ffffff, 0x52000000, 0x57ffffff, 0x56000000, 0x54000000, 0x55ffffff, 0xffffffff, 0xfe000000, 0xfc000000, 0xfdffffff, 0xf8000000, 0xf9ffffff, 0xfbffffff, 0xfa000000, 0xf0000000, 0xf1ffffff, 0xf3ffffff, 0xf2000000, 0xf7ffffff, 0xf6000000, 0xf4000000, 0xf5ffffff, 0xe0000000, 0xe1ffffff, 0xe3ffffff, 0xe2000000, 0xe7ffffff, 0xe6000000, 0xe4000000, 0xe5ffffff, 0xefffffff, 0xee000000, 0xec000000, 0xedffffff, 0xe8000000, 0xe9ffffff, 0xebffffff, 0xea000000, 0xc0000000, 0xc1ffffff, 0xc3ffffff, 0xc2000000, 0xc7ffffff, 0xc6000000, 0xc4000000, 0xc5ffffff, 0xcfffffff, 0xce000000, 0xcc000000, 0xcdffffff, 0xc8000000, 0xc9ffffff, 0xcbffffff, 0xca000000, 0xdfffffff, 0xde000000, 0xdc000000, 0xddffffff, 0xd8000000, 0xd9ffffff, 0xdbffffff, 0xda000000, 0xd0000000, 0xd1ffffff, 0xd3ffffff, 0xd2000000, 0xd7ffffff, 0xd6000000, 0xd4000000, 0xd5ffffff, 0x80000000, 0x81ffffff, 0x83ffffff, 0x82000000, 0x87ffffff, 0x86000000, 0x84000000, 0x85ffffff, 0x8fffffff, 0x8e000000, 0x8c000000, 0x8dffffff, 0x88000000, 0x89ffffff, 0x8bffffff, 0x8a000000, 0x9fffffff, 0x9e000000, 0x9c000000, 0x9dffffff, 0x98000000, 0x99ffffff, 0x9bffffff, 0x9a000000, 0x90000000, 0x91ffffff, 0x93ffffff, 0x92000000, 0x97ffffff, 0x96000000, 0x94000000, 0x95ffffff, 0xbfffffff, 0xbe000000, 0xbc000000, 0xbdffffff, 0xb8000000, 0xb9ffffff, 0xbbffffff, 0xba000000, 0xb0000000, 0xb1ffffff, 0xb3ffffff, 0xb2000000, 0xb7ffffff, 0xb6000000, 0xb4000000, 0xb5ffffff, 0xa0000000, 0xa1ffffff, 0xa3ffffff, 0xa2000000, 0xa7ffffff, 0xa6000000, 0xa4000000, 0xa5ffffff, 0xafffffff, 0xae000000, 0xac000000, 0xadffffff, 0xa8000000, 0xa9ffffff, 0xabffffff, 0xaa000000};
static const uint32_t LookupTable_secondword[256] = { 0x0, 0x1ffff, 0x3ffff, 0x20000, 0x7ffff, 0x60000, 0x40000, 0x5ffff, 0xfffff, 0xe0000, 0xc0000, 0xdffff, 0x80000, 0x9ffff, 0xbffff, 0xa0000, 0x1fffff, 0x1e0000, 0x1c0000, 0x1dffff, 0x180000, 0x19ffff, 0x1bffff, 0x1a0000, 0x100000, 0x11ffff, 0x13ffff, 0x120000, 0x17ffff, 0x160000, 0x140000, 0x15ffff, 0x3fffff, 0x3e0000, 0x3c0000, 0x3dffff, 0x380000, 0x39ffff, 0x3bffff, 0x3a0000, 0x300000, 0x31ffff, 0x33ffff, 0x320000, 0x37ffff, 0x360000, 0x340000, 0x35ffff, 0x200000, 0x21ffff, 0x23ffff, 0x220000, 0x27ffff, 0x260000, 0x240000, 0x25ffff, 0x2fffff, 0x2e0000, 0x2c0000, 0x2dffff, 0x280000, 0x29ffff, 0x2bffff, 0x2a0000, 0x7fffff, 0x7e0000, 0x7c0000, 0x7dffff, 0x780000, 0x79ffff, 0x7bffff, 0x7a0000, 0x700000, 0x71ffff, 0x73ffff, 0x720000, 0x77ffff, 0x760000, 0x740000, 0x75ffff, 0x600000, 0x61ffff, 0x63ffff, 0x620000, 0x67ffff, 0x660000, 0x640000, 0x65ffff, 0x6fffff, 0x6e0000, 0x6c0000, 0x6dffff, 0x680000, 0x69ffff, 0x6bffff, 0x6a0000, 0x400000, 0x41ffff, 0x43ffff, 0x420000, 0x47ffff, 0x460000, 0x440000, 0x45ffff, 0x4fffff, 0x4e0000, 0x4c0000, 0x4dffff, 0x480000, 0x49ffff, 0x4bffff, 0x4a0000, 0x5fffff, 0x5e0000, 0x5c0000, 0x5dffff, 0x580000, 0x59ffff, 0x5bffff, 0x5a0000, 0x500000, 0x51ffff, 0x53ffff, 0x520000, 0x57ffff, 0x560000, 0x540000, 0x55ffff, 0xffffff, 0xfe0000, 0xfc0000, 0xfdffff, 0xf80000, 0xf9ffff, 0xfbffff, 0xfa0000, 0xf00000, 0xf1ffff, 0xf3ffff, 0xf20000, 0xf7ffff, 0xf60000, 0xf40000, 0xf5ffff, 0xe00000, 0xe1ffff, 0xe3ffff, 0xe20000, 0xe7ffff, 0xe60000, 0xe40000, 0xe5ffff, 0xefffff, 0xee0000, 0xec0000, 0xedffff, 0xe80000, 0xe9ffff, 0xebffff, 0xea0000, 0xc00000, 0xc1ffff, 0xc3ffff, 0xc20000, 0xc7ffff, 0xc60000, 0xc40000, 0xc5ffff, 0xcfffff, 0xce0000, 0xcc0000, 0xcdffff, 0xc80000, 0xc9ffff, 0xcbffff, 0xca0000, 0xdfffff, 0xde0000, 0xdc0000, 0xddffff, 0xd80000, 0xd9ffff, 0xdbffff, 0xda0000, 0xd00000, 0xd1ffff, 0xd3ffff, 0xd20000, 0xd7ffff, 0xd60000, 0xd40000, 0xd5ffff, 0x800000, 0x81ffff, 0x83ffff, 0x820000, 0x87ffff, 0x860000, 0x840000, 0x85ffff, 0x8fffff, 0x8e0000, 0x8c0000, 0x8dffff, 0x880000, 0x89ffff, 0x8bffff, 0x8a0000, 0x9fffff, 0x9e0000, 0x9c0000, 0x9dffff, 0x980000, 0x99ffff, 0x9bffff, 0x9a0000, 0x900000, 0x91ffff, 0x93ffff, 0x920000, 0x97ffff, 0x960000, 0x940000, 0x95ffff, 0xbfffff, 0xbe0000, 0xbc0000, 0xbdffff, 0xb80000, 0xb9ffff, 0xbbffff, 0xba0000, 0xb00000, 0xb1ffff, 0xb3ffff, 0xb20000, 0xb7ffff, 0xb60000, 0xb40000, 0xb5ffff, 0xa00000, 0xa1ffff, 0xa3ffff, 0xa20000, 0xa7ffff, 0xa60000, 0xa40000, 0xa5ffff, 0xafffff, 0xae0000, 0xac0000, 0xadffff, 0xa80000, 0xa9ffff, 0xabffff, 0xaa0000};
static const uint32_t LookupTable_thirdword[256]  = { 0x0, 0x1ff, 0x3ff, 0x200, 0x7ff, 0x600, 0x400, 0x5ff, 0xfff, 0xe00, 0xc00, 0xdff, 0x800, 0x9ff, 0xbff, 0xa00, 0x1fff, 0x1e00, 0x1c00, 0x1dff, 0x1800, 0x19ff, 0x1bff, 0x1a00, 0x1000, 0x11ff, 0x13ff, 0x1200, 0x17ff, 0x1600, 0x1400, 0x15ff, 0x3fff, 0x3e00, 0x3c00, 0x3dff, 0x3800, 0x39ff, 0x3bff, 0x3a00, 0x3000, 0x31ff, 0x33ff, 0x3200, 0x37ff, 0x3600, 0x3400, 0x35ff, 0x2000, 0x21ff, 0x23ff, 0x2200, 0x27ff, 0x2600, 0x2400, 0x25ff, 0x2fff, 0x2e00, 0x2c00, 0x2dff, 0x2800, 0x29ff, 0x2bff, 0x2a00, 0x7fff, 0x7e00, 0x7c00, 0x7dff, 0x7800, 0x79ff, 0x7bff, 0x7a00, 0x7000, 0x71ff, 0x73ff, 0x7200, 0x77ff, 0x7600, 0x7400, 0x75ff, 0x6000, 0x61ff, 0x63ff, 0x6200, 0x67ff, 0x6600, 0x6400, 0x65ff, 0x6fff, 0x6e00, 0x6c00, 0x6dff, 0x6800, 0x69ff, 0x6bff, 0x6a00, 0x4000, 0x41ff, 0x43ff, 0x4200, 0x47ff, 0x4600, 0x4400, 0x45ff, 0x4fff, 0x4e00, 0x4c00, 0x4dff, 0x4800, 0x49ff, 0x4bff, 0x4a00, 0x5fff, 0x5e00, 0x5c00, 0x5dff, 0x5800, 0x59ff, 0x5bff, 0x5a00, 0x5000, 0x51ff, 0x53ff, 0x5200, 0x57ff, 0x5600, 0x5400, 0x55ff, 0xffff, 0xfe00, 0xfc00, 0xfdff, 0xf800, 0xf9ff, 0xfbff, 0xfa00, 0xf000, 0xf1ff, 0xf3ff, 0xf200, 0xf7ff, 0xf600, 0xf400, 0xf5ff, 0xe000, 0xe1ff, 0xe3ff, 0xe200, 0xe7ff, 0xe600, 0xe400, 0xe5ff, 0xefff, 0xee00, 0xec00, 0xedff, 0xe800, 0xe9ff, 0xebff, 0xea00, 0xc000, 0xc1ff, 0xc3ff, 0xc200, 0xc7ff, 0xc600, 0xc400, 0xc5ff, 0xcfff, 0xce00, 0xcc00, 0xcdff, 0xc800, 0xc9ff, 0xcbff, 0xca00, 0xdfff, 0xde00, 0xdc00, 0xddff, 0xd800, 0xd9ff, 0xdbff, 0xda00, 0xd000, 0xd1ff, 0xd3ff, 0xd200, 0xd7ff, 0xd600, 0xd400, 0xd5ff, 0x8000, 0x81ff, 0x83ff, 0x8200, 0x87ff, 0x8600, 0x8400, 0x85ff, 0x8fff, 0x8e00, 0x8c00, 0x8dff, 0x8800, 0x89ff, 0x8bff, 0x8a00, 0x9fff, 0x9e00, 0x9c00, 0x9dff, 0x9800, 0x99ff, 0x9bff, 0x9a00, 0x9000, 0x91ff, 0x93ff, 0x9200, 0x97ff, 0x9600, 0x9400, 0x95ff, 0xbfff, 0xbe00, 0xbc00, 0xbdff, 0xb800, 0xb9ff, 0xbbff, 0xba00, 0xb000, 0xb1ff, 0xb3ff, 0xb200, 0xb7ff, 0xb600, 0xb400, 0xb5ff, 0xa000, 0xa1ff, 0xa3ff, 0xa200, 0xa7ff, 0xa600, 0xa400, 0xa5ff, 0xafff, 0xae00, 0xac00, 0xadff, 0xa800, 0xa9ff, 0xabff, 0xaa00};
static const uint32_t LookupTable_fourthword[256] = { 0x0, 0x1, 0x3, 0x2, 0x7, 0x6, 0x4, 0x5, 0xf, 0xe, 0xc, 0xd, 0x8, 0x9, 0xb, 0xa, 0x1f, 0x1e, 0x1c, 0x1d, 0x18, 0x19, 0x1b, 0x1a, 0x10, 0x11, 0x13, 0x12, 0x17, 0x16, 0x14, 0x15, 0x3f, 0x3e, 0x3c, 0x3d, 0x38, 0x39, 0x3b, 0x3a, 0x30, 0x31, 0x33, 0x32, 0x37, 0x36, 0x34, 0x35, 0x20, 0x21, 0x23, 0x22, 0x27, 0x26, 0x24, 0x25, 0x2f, 0x2e, 0x2c, 0x2d, 0x28, 0x29, 0x2b, 0x2a, 0x7f, 0x7e, 0x7c, 0x7d, 0x78, 0x79, 0x7b, 0x7a, 0x70, 0x71, 0x73, 0x72, 0x77, 0x76, 0x74, 0x75, 0x60, 0x61, 0x63, 0x62, 0x67, 0x66, 0x64, 0x65, 0x6f, 0x6e, 0x6c, 0x6d, 0x68, 0x69, 0x6b, 0x6a, 0x40, 0x41, 0x43, 0x42, 0x47, 0x46, 0x44, 0x45, 0x4f, 0x4e, 0x4c, 0x4d, 0x48, 0x49, 0x4b, 0x4a, 0x5f, 0x5e, 0x5c, 0x5d, 0x58, 0x59, 0x5b, 0x5a, 0x50, 0x51, 0x53, 0x52, 0x57, 0x56, 0x54, 0x55, 0xff, 0xfe, 0xfc, 0xfd, 0xf8, 0xf9, 0xfb, 0xfa, 0xf0, 0xf1, 0xf3, 0xf2, 0xf7, 0xf6, 0xf4, 0xf5, 0xe0, 0xe1, 0xe3, 0xe2, 0xe7, 0xe6, 0xe4, 0xe5, 0xef, 0xee, 0xec, 0xed, 0xe8, 0xe9, 0xeb, 0xea, 0xc0, 0xc1, 0xc3, 0xc2, 0xc7, 0xc6, 0xc4, 0xc5, 0xcf, 0xce, 0xcc, 0xcd, 0xc8, 0xc9, 0xcb, 0xca, 0xdf, 0xde, 0xdc, 0xdd, 0xd8, 0xd9, 0xdb, 0xda, 0xd0, 0xd1, 0xd3, 0xd2, 0xd7, 0xd6, 0xd4, 0xd5, 0x80, 0x81, 0x83, 0x82, 0x87, 0x86, 0x84, 0x85, 0x8f, 0x8e, 0x8c, 0x8d, 0x88, 0x89, 0x8b, 0x8a, 0x9f, 0x9e, 0x9c, 0x9d, 0x98, 0x99, 0x9b, 0x9a, 0x90, 0x91, 0x93, 0x92, 0x97, 0x96, 0x94, 0x95, 0xbf, 0xbe, 0xbc, 0xbd, 0xb8, 0xb9, 0xbb, 0xba, 0xb0, 0xb1, 0xb3, 0xb2, 0xb7, 0xb6, 0xb4, 0xb5, 0xa0, 0xa1, 0xa3, 0xa2, 0xa7, 0xa6, 0xa4, 0xa5, 0xaf, 0xae, 0xac, 0xad, 0xa8, 0xa9, 0xab, 0xaa};

void AudioOutputADAT::begin(void)
{

	dma.begin(true); // Allocate the DMA channel first

	block_ch1_1st = NULL;
	block_ch2_1st = NULL;
	block_ch3_1st = NULL;
	block_ch4_1st = NULL;
	block_ch5_1st = NULL;
	block_ch6_1st = NULL;
	block_ch7_1st = NULL;
	block_ch8_1st = NULL;
	// TODO: should we set & clear the I2S_TCSR_SR bit here?
	config_ADAT();
	CORE_PIN22_CONFIG = PORT_PCR_MUX(6); // pin 22, PTC1, I2S0_TXD0

	const int nbytes_mlno = 2 * 8; // 16 Bytes per minor loop

	dma.TCD->SADDR = ADAT_tx_buffer;
	dma.TCD->SOFF = 4;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2); //transfersize 2 = 32 bit, 5 = 32 byte
	dma.TCD->NBYTES_MLNO = nbytes_mlno;
	dma.TCD->SLAST = -sizeof(ADAT_tx_buffer);
	dma.TCD->DADDR = &I2S0_TDR0;
	dma.TCD->DOFF = 0;
	dma.TCD->CITER_ELINKNO = sizeof(ADAT_tx_buffer) / nbytes_mlno;
	dma.TCD->DLASTSGA = 0;
	dma.TCD->BITER_ELINKNO = sizeof(ADAT_tx_buffer) / nbytes_mlno;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_TX);
	update_responsibility = update_setup();
	dma.enable();

	I2S0_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE | I2S_TCSR_FR;
	dma.attachInterrupt(isr);

}

/*

	https://ackspace.nl/wiki/ADAT_project
	https://github.com/xcore/sc_adat/blob/master/module_adat_tx/src/adat_tx_port.xc

	for information about the clock:

	http://cache.freescale.com/files/32bit/doc/app_note/AN4800.pdf

	We need a bitrate twice as high as the SPDIF example.
	Because BITCLK can not be the same as MCLK, but only half of MCLK, we need a 2*MCLK (so for 44100 samplerate we need 88200 MCLK)
*/

void AudioOutputADAT::isr(void)
{
	const int16_t *src1, *src2, *src3, *src4,*src5, *src6, *src7, *src8;
	const int16_t *zeros = (const int16_t *)zerodata;

	uint32_t *end, *dest;
	uint32_t saddr;
	uint32_t sample1, sample2, sample3, sample4, sample5, sample6, sample7, sample8;

	static uint32_t previousnrzi_highlow = 0; //this is used for the NZRI encoding to remember the last state.
	// if the result of the lookup table LSB is other than this lastbit, the result of the lookup table must be inverted.

	//static bool previousframeinverted=false;

	saddr = (uint32_t)(dma.TCD->SADDR);
	dma.clearInterrupt();
	if (saddr < (uint32_t)ADAT_tx_buffer + sizeof(ADAT_tx_buffer) / 2) {
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		dest = (uint32_t *)&ADAT_tx_buffer[AUDIO_BLOCK_SAMPLES * 8/2];
		end = (uint32_t *)&ADAT_tx_buffer[AUDIO_BLOCK_SAMPLES * 8];
		if (AudioOutputADAT::update_responsibility) AudioStream::update_all();
	} else {
		// DMA is transmitting the second half of the buffer
		// so we must fill the first half
		dest = (uint32_t *)ADAT_tx_buffer;
		end = (uint32_t *)&ADAT_tx_buffer[AUDIO_BLOCK_SAMPLES * 8/2];
	}

	src1 = (block_ch1_1st) ? block_ch1_1st->data + ch1_offset : zeros;
	src2 = (block_ch2_1st) ? block_ch2_1st->data + ch2_offset : zeros;
	src3 = (block_ch3_1st) ? block_ch3_1st->data + ch3_offset : zeros;
	src4 = (block_ch4_1st) ? block_ch4_1st->data + ch4_offset : zeros;
	src5 = (block_ch5_1st) ? block_ch5_1st->data + ch5_offset : zeros;
	src6 = (block_ch6_1st) ? block_ch6_1st->data + ch6_offset : zeros;
	src7 = (block_ch7_1st) ? block_ch7_1st->data + ch7_offset : zeros;
	src8 = (block_ch8_1st) ? block_ch8_1st->data + ch8_offset : zeros;


	//Non-NZRI encoded 'empty' ADAT Frame
	/*     
	*(dest+0) = 0b10000100001000010000100001000010; // bit 0-31
	*(dest+1) = 0b00010000100001000010000100001000; // bit 32-63
	*(dest+2) = 0b01000010000100001000010000100001; // bit 64-95
	*(dest+3) = 0b00001000010000100001000010000100; // bit 96-127

	*(dest+4) = 0b00100001000010000100001000010000; // bit 128-159
	*(dest+5) = 0b10000100001000010000100001000010; // bit 160-191
	*(dest+6) = 0b00010000100001000010000100001000; // bit 192-223
	*(dest+7) = 0b01000010000100001000000000010000; // bit 224-255

	dest+=8;

	*/

	/*
	//NZRI encoded 'empty' ADAT Frame

	*(dest+0) = 0b11111000001111100000111110000011; // bit 0-31
	*(dest+1) = 0b11100000111110000011111000001111; // bit 32-63
	*(dest+2) = 0b10000011111000001111100000111110; // bit 64-95
	*(dest+3) = 0b00001111100000111110000011111000; // bit 96-127

	*(dest+4) = 0b00111110000011111000001111100000; // bit 128-159
	*(dest+5) = 0b11111000001111100000111110000011; // bit 160-191
	*(dest+6) = 0b11100000111110000011111000001111; // bit 192-223
	*(dest+7) = 0b10000011111000001111111111100000; // bit 224-255

	dest+=8;

	*/

	do
	{
		sample1 = (*src1++);
		sample2 = (*src2++);
		sample3 = (*src3++);
		sample4 = (*src4++);
		sample5 = (*src5++);
		sample6 = (*src6++);
		sample7 = (*src7++);
		sample8 = (*src8++);

		uint32_t value;
		uint32_t nzrivalue;

		value=				 0b10000100001000010000100001000010 /* bit  0-31 */ 
//							  b00000000000000001000000000000000 // start of 16 bit sample
			| ((sample1<<15)&0b01111000000000000000000000000000) 
			| ((sample1<<14)&0b00000011110000000000000000000000) 
			| ((sample1<<13)&0b00000000000111100000000000000000)   
			| ((sample1<<12)&0b00000000000000001111000000000000) 
			| ((sample2>>15)&0b00000000000000000000000000000001)
			; 
		nzrivalue = previousnrzi_highlow ^ (LookupTable_firstword[(byte)(value >> 24)] ^ LookupTable_secondword[(byte)(value >> 16)] ^ LookupTable_thirdword[(byte)(value >> 8)] ^ LookupTable_fourthword[(byte)value]);
		*(dest+0) = nzrivalue;
        previousnrzi_highlow = ((nzrivalue & 0b1) == 0b1) ? ~0U : 0U;
		
		value =				 0b00010000100001000010000100001000 /* bit 32-63 */ 
//							  b00000000000000001000100010001000 // start of 16 bit sample
			| ((sample2<<17)&0b11100000000000000000000000000000) 
			| ((sample2<<16)&0b00001111000000000000000000000000) 
			| ((sample2<<15)&0b00000000011110000000000000000000)   
			| ((sample2<<14)&0b00000000000000111100000000000000) 
			| ((sample3>>13)&0b00000000000000000000000000000111)
			; 
		nzrivalue = previousnrzi_highlow ^ (LookupTable_firstword[(byte)(value >> 24)] ^ LookupTable_secondword[(byte)(value >> 16)] ^ LookupTable_thirdword[(byte)(value >> 8)] ^ LookupTable_fourthword[(byte)value]);
		*(dest+1) = nzrivalue;
        previousnrzi_highlow = ((nzrivalue & 0b1) == 0b1) ? ~0U : 0U;


		value =				 0b01000010000100001000010000100001 /* bit 64-95 */ 
//							  b00000000000000001000000000000000 // start of 16 bit sample
			| ((sample3<<19)&0b10000000000000000000000000000000) 
			| ((sample3<<18)&0b00111100000000000000000000000000)  
			| ((sample3<<17)&0b00000001111000000000000000000000)  
			| ((sample3<<16)&0b00000000000011110000000000000000) 
			| ((sample4>>11)&0b00000000000000000000000000011110)
			;
		nzrivalue = previousnrzi_highlow ^ (LookupTable_firstword[(byte)(value >> 24)] ^ LookupTable_secondword[(byte)(value >> 16)] ^ LookupTable_thirdword[(byte)(value >> 8)] ^ LookupTable_fourthword[(byte)value]);
		*(dest+2) = nzrivalue;
        previousnrzi_highlow = ((nzrivalue & 0b1) == 0b1) ? ~0U : 0U;

		value =				 0b00001000010000100001000010000100 /* bit 96-127 */ 
//							  b00000000000000001000100010001000 // start of 16 bit sample
			| ((sample4<<20)&0b11110000000000000000000000000000) 
			| ((sample4<<19)&0b00000111100000000000000000000000) 
			| ((sample4<<18)&0b00000000001111000000000000000000) 
			| ((sample5>>9 )&0b00000000000000000000000001111000) 
			| ((sample5>>10)&0b00000000000000000000000000000011) 
			;
		nzrivalue = previousnrzi_highlow ^ (LookupTable_firstword[(byte)(value >> 24)] ^ LookupTable_secondword[(byte)(value >> 16)] ^ LookupTable_thirdword[(byte)(value >> 8)] ^ LookupTable_fourthword[(byte)value]);
		*(dest+3) = nzrivalue;
        previousnrzi_highlow = ((nzrivalue & 0b1) == 0b1) ? ~0U : 0U;

		value =				 0b00100001000010000100001000010000 /* bit 128-159 */ 
//							  b00000000000000001000100010001000 // start of 16 bit sample
			| ((sample5<<22)&0b11000000000000000000000000000000) 
			| ((sample5<<21)&0b00011110000000000000000000000000) 
			| ((sample5<<20)&0b00000000111100000000000000000000) 
			| ((sample6>>7 )&0b00000000000000000000000111100000) 
			| ((sample6>>8 )&0b00000000000000000000000000001111) 
			;
		nzrivalue = previousnrzi_highlow ^ (LookupTable_firstword[(byte)(value >> 24)] ^ LookupTable_secondword[(byte)(value >> 16)] ^ LookupTable_thirdword[(byte)(value >> 8)] ^ LookupTable_fourthword[(byte)value]);
		*(dest+4) = nzrivalue;
        previousnrzi_highlow = ((nzrivalue & 0b1) == 0b1) ? ~0U : 0U;

		value =				 0b10000100001000010000100001000010 /* bit 160-191 */
//							  b00000000000000001000100010001000 // start of 16 bit sample
			| ((sample6<<23)&0b01111000000000000000000000000000) 
			| ((sample6<<22)&0b00000011110000000000000000000000) 
			| ((sample7>>5 )&0b00000000000000000000011110000000) 
			| ((sample7>>6 )&0b00000000000000000000000000111100) 
			| ((sample7>>7 )&0b00000000000000000000000000000001) 
			;

		nzrivalue = previousnrzi_highlow ^ (LookupTable_firstword[(byte)(value >> 24)] ^ LookupTable_secondword[(byte)(value >> 16)] ^ LookupTable_thirdword[(byte)(value >> 8)] ^ LookupTable_fourthword[(byte)value]);
		*(dest+5) = nzrivalue;
        previousnrzi_highlow = ((nzrivalue & 0b1) == 0b1) ? ~0U : 0U;

		value =				 0b00010000100001000010000100001000 /* bit 192-223 */
//							  b00000000000000001000100010001000 // start of 16 bit sample
			| ((sample7<<25)&0b11100000000000000000000000000000) 
			| ((sample7<<24)&0b00001111000000000000000000000000) 
			| ((sample8>>3 )&0b00000000000000000001111000000000) 
			| ((sample8>>4 )&0b00000000000000000000000011110000) 
			| ((sample8>>5 )&0b00000000000000000000000000000111)
			;

		nzrivalue = previousnrzi_highlow ^ (LookupTable_firstword[(byte)(value >> 24)] ^ LookupTable_secondword[(byte)(value >> 16)] ^ LookupTable_thirdword[(byte)(value >> 8)] ^ LookupTable_fourthword[(byte)value]);
		*(dest+6) = nzrivalue;
        previousnrzi_highlow = ((nzrivalue & 0b1) == 0b1) ? ~0U : 0U;

		value =				 0b01000010000100001000000000010000 /* bit 224-255 */
//							  b00000000000000001000100010001000 // start of 16 bit sample
			| ((sample8<<27)&0b10000000000000000000000000000000) 
			| ((sample8<<26)&0b00111100000000000000000000000000) 
			;

		nzrivalue = previousnrzi_highlow ^ (LookupTable_firstword[(byte)(value >> 24)] ^ LookupTable_secondword[(byte)(value >> 16)] ^ LookupTable_thirdword[(byte)(value >> 8)] ^ LookupTable_fourthword[(byte)value]);
		*(dest+7) = nzrivalue;
        previousnrzi_highlow = ((nzrivalue & 0b1) == 0b1) ? ~0U : 0U;

		dest+=8;
	} while (dest < end);
	/*
	block = AudioOutputADAT::block_ch1_1st;
	if (block) {
		offset = AudioOutputADAT::ch1_offset;
		src = &block->data[offset];
		do {

			sample = (uint32_t)*src++;

		} while (dest < end);
		offset += AUDIO_BLOCK_SAMPLES/2;
		if (offset < AUDIO_BLOCK_SAMPLES) {
			AudioOutputADAT::ch1_offset = offset;
		} else {
			AudioOutputADAT::ch1_offset = 0;
			AudioStream::release(block);
			AudioOutputADAT::block_ch1_1st = AudioOutputADAT::block_ch1_2nd;
			AudioOutputADAT::block_ch1_2nd = NULL;
		}
	} else {
		sample = 0;
	
		do {
			// *(dest+0) = 0b11111000001111100000111110000011; // bit 0-31
			// *(dest+1) = 0b11100000111110000011111000001111; // bit 32-63
			// *(dest+2) = 0b10000011111000001111100000111110; // bit 64-95
			// *(dest+3) = 0b00001111100000111110000011111000; // bit 96-127

			// *(dest+4) = 0b00111110000011111000001111100000; // bit 128-159
			// *(dest+5) = 0b11111000001111100000111110000011; // bit 160-191
			// *(dest+6) = 0b11100000111110000011111000001111; // bit 192-223
			// *(dest+7) = 0b10000011111000001111111111100000; // bit 224-255
			dest+=8;
		} while (dest < end);
	}

	
	dest -= AUDIO_BLOCK_SAMPLES * 8/2 - 8/2;
	block = AudioOutputADAT::block_ch2_1st;
	if (block) {
		offset = AudioOutputADAT::ch2_offset;
		src = &block->data[offset];

		do {
			sample = *src++;
			
			// *(dest+0) = 0b00111110000011111000001111100000; // bit 128-159
			// *(dest+1) = 0b11111000001111100000111110000011; // bit 160-191
			// *(dest+2) = 0b11100000111110000011111000001111; // bit 192-223
			// *(dest+3) = 0b10000011111000001111111111100000; // bit 224-255
			dest+=8;
		} while (dest < end);

		offset += AUDIO_BLOCK_SAMPLES/2;
		if (offset < AUDIO_BLOCK_SAMPLES) {
			AudioOutputADAT::ch2_offset = offset;
		} else {
			AudioOutputADAT::ch2_offset = 0;
			AudioStream::release(block);
			AudioOutputADAT::block_ch2_1st = AudioOutputADAT::block_ch2_2nd;
			AudioOutputADAT::block_ch2_2nd = NULL;
		}
	} else {
			// *(dest+0) = 0b00111110000011111000001111100000; // bit 128-159
			// *(dest+1) = 0b11111000001111100000111110000011; // bit 160-191
			// *(dest+2) = 0b11100000111110000011111000001111; // bit 192-223
			// *(dest+3) = 0b10000011111000001111111111100000; // bit 224-255
			dest+=8;
	}*/

	if (block_ch1_1st) {
		if (ch1_offset == 0) {
			ch1_offset = AUDIO_BLOCK_SAMPLES/2;
		} else {
			ch1_offset = 0;
			release(block_ch1_1st);
			block_ch1_1st = block_ch1_2nd;
			block_ch1_2nd = NULL;
		}
	}
	if (block_ch2_1st) {
		if (ch2_offset == 0) {
			ch2_offset = AUDIO_BLOCK_SAMPLES/2;
		} else {
			ch2_offset = 0;
			release(block_ch2_1st);
			block_ch2_1st = block_ch2_2nd;
			block_ch2_2nd = NULL;
		}
	}
	if (block_ch3_1st) {
		if (ch3_offset == 0) {
			ch3_offset = AUDIO_BLOCK_SAMPLES/2;
		} else {
			ch3_offset = 0;
			release(block_ch3_1st);
			block_ch3_1st = block_ch3_2nd;
			block_ch3_2nd = NULL;
		}
	}
	if (block_ch4_1st) {
		if (ch4_offset == 0) {
			ch4_offset = AUDIO_BLOCK_SAMPLES/2;
		} else {
			ch4_offset = 0;
			release(block_ch4_1st);
			block_ch4_1st = block_ch4_2nd;
			block_ch4_2nd = NULL;
		}
	}
	if (block_ch5_1st) {
		if (ch5_offset == 0) {
			ch5_offset = AUDIO_BLOCK_SAMPLES/2;
		} else {
			ch5_offset = 0;
			release(block_ch5_1st);
			block_ch5_1st = block_ch5_2nd;
			block_ch5_2nd = NULL;
		}
	}
	if (block_ch6_1st) {
		if (ch6_offset == 0) {
			ch6_offset = AUDIO_BLOCK_SAMPLES/2;
		} else {
			ch6_offset = 0;
			release(block_ch6_1st);
			block_ch6_1st = block_ch6_2nd;
			block_ch6_2nd = NULL;
		}
	}
	if (block_ch7_1st) {
		if (ch7_offset == 0) {
			ch7_offset = AUDIO_BLOCK_SAMPLES/2;
		} else {
			ch7_offset = 0;
			release(block_ch7_1st);
			block_ch7_1st = block_ch7_2nd;
			block_ch7_2nd = NULL;
		}
	}
	if (block_ch8_1st) {
		if (ch8_offset == 0) {
			ch8_offset = AUDIO_BLOCK_SAMPLES/2;
		} else {
			ch8_offset = 0;
			release(block_ch8_1st);
			block_ch8_1st = block_ch8_2nd;
			block_ch8_2nd = NULL;
		}
	}
}

void AudioOutputADAT::mute_PCM(const bool mute)
{
	//vucp = mute?VUCP_INVALID:VUCP_VALID;
	//#TODO
}

void AudioOutputADAT::update(void)
{

	audio_block_t *block, *tmp;

	block = receiveReadOnly(0); // input 0 = channel 1
	if (block) {
		__disable_irq();
		if (block_ch1_1st == NULL) {
			block_ch1_1st = block;
			ch1_offset = 0;
			__enable_irq();
		} else if (block_ch1_2nd == NULL) {
			block_ch1_2nd = block;
			__enable_irq();
		} else {
			tmp = block_ch1_1st;
			block_ch1_1st = block_ch1_2nd;
			block_ch1_2nd = block;
			ch1_offset = 0;
			__enable_irq();
			release(tmp);
		}
	}
	block = receiveReadOnly(1); // input 1 = channel 2
	if (block) {
		__disable_irq();
		if (block_ch2_1st == NULL) {
			block_ch2_1st = block;
			ch2_offset = 0;
			__enable_irq();
		} else if (block_ch2_2nd == NULL) {
			block_ch2_2nd = block;
			__enable_irq();
		} else {
			tmp = block_ch2_1st;
			block_ch2_1st = block_ch2_2nd;
			block_ch2_2nd = block;
			ch2_offset = 0;
			__enable_irq();
			release(tmp);
		}
	}
	block = receiveReadOnly(2); // channel 3
	if (block) {
		__disable_irq();
		if (block_ch3_1st == NULL) {
			block_ch3_1st = block;
			ch3_offset = 0;
			__enable_irq();
		} else if (block_ch3_2nd == NULL) {
			block_ch3_2nd = block;
			__enable_irq();
		} else {
			tmp = block_ch3_1st;
			block_ch3_1st = block_ch3_2nd;
			block_ch3_2nd = block;
			ch3_offset = 0;
			__enable_irq();
			release(tmp);
		}
	}
	block = receiveReadOnly(3); // channel 4
	if (block) {
		__disable_irq();
		if (block_ch4_1st == NULL) {
			block_ch4_1st = block;
			ch4_offset = 0;
			__enable_irq();
		} else if (block_ch4_2nd == NULL) {
			block_ch4_2nd = block;
			__enable_irq();
		} else {
			tmp = block_ch4_1st;
			block_ch4_1st = block_ch4_2nd;
			block_ch4_2nd = block;
			ch4_offset = 0;
			__enable_irq();
			release(tmp);
		}
	}
	block = receiveReadOnly(4); // channel 5
	if (block) {
		__disable_irq();
		if (block_ch5_1st == NULL) {
			block_ch5_1st = block;
			ch5_offset = 0;
			__enable_irq();
		} else if (block_ch5_2nd == NULL) {
			block_ch5_2nd = block;
			__enable_irq();
		} else {
			tmp = block_ch5_1st;
			block_ch5_1st = block_ch5_2nd;
			block_ch5_2nd = block;
			ch5_offset = 0;
			__enable_irq();
			release(tmp);
		}
	}
	block = receiveReadOnly(5); // channel 6
	if (block) {
		__disable_irq();
		if (block_ch6_1st == NULL) {
			block_ch6_1st = block;
			ch6_offset = 0;
			__enable_irq();
		} else if (block_ch6_2nd == NULL) {
			block_ch6_2nd = block;
			__enable_irq();
		} else {
			tmp = block_ch6_1st;
			block_ch6_1st = block_ch6_2nd;
			block_ch6_2nd = block;
			ch6_offset = 0;
			__enable_irq();
			release(tmp);
		}
	}
	block = receiveReadOnly(6); // channel 7
	if (block) {
		__disable_irq();
		if (block_ch7_1st == NULL) {
			block_ch7_1st = block;
			ch7_offset = 0;
			__enable_irq();
		} else if (block_ch7_2nd == NULL) {
			block_ch7_2nd = block;
			__enable_irq();
		} else {
			tmp = block_ch7_1st;
			block_ch7_1st = block_ch7_2nd;
			block_ch7_2nd = block;
			ch7_offset = 0;
			__enable_irq();
			release(tmp);
		}
	}
	block = receiveReadOnly(7); // channel 8
	if (block) {
		__disable_irq();
		if (block_ch8_1st == NULL) {
			block_ch8_1st = block;
			ch8_offset = 0;
			__enable_irq();
		} else if (block_ch8_2nd == NULL) {
			block_ch8_2nd = block;
			__enable_irq();
		} else {
			tmp = block_ch8_1st;
			block_ch8_1st = block_ch8_2nd;
			block_ch8_2nd = block;
			ch8_offset = 0;
			__enable_irq();
			release(tmp);
		}
	}
}


#ifndef MCLK_SRC
#if F_CPU >= 20000000
  #define MCLK_SRC  3  // the PLL
#else
  #define MCLK_SRC  0  // system clock
#endif
#endif


void AudioOutputADAT::config_ADAT(void)
{
	SIM_SCGC6 |= SIM_SCGC6_I2S;
	SIM_SCGC7 |= SIM_SCGC7_DMA;
	SIM_SCGC6 |= SIM_SCGC6_DMAMUX;

	// enable MCLK output
	I2S0_MCR = I2S_MCR_MICS(MCLK_SRC) | I2S_MCR_MOE;
	//while (I2S0_MCR & I2S_MCR_DUF) ;
	//I2S0_MDR = I2S_MDR_FRACT((MCLK_MULT-1)) | I2S_MDR_DIVIDE((MCLK_DIV-1));

	AudioOutputADAT::setI2SFreq(88200);

	// configure transmitter
	I2S0_TMR = 0;
	I2S0_TCR1 = I2S_TCR1_TFW(1);  // watermark
	I2S0_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_MSEL(1) | I2S_TCR2_BCD | I2S_TCR2_DIV(0);
	I2S0_TCR3 = I2S_TCR3_TCE;

	//8 Words per Frame 32 Bit Word-Length -> 256 Bit Frame-Length, MSB First:
	I2S0_TCR4 = I2S_TCR4_FRSZ(7) | I2S_TCR4_SYWD(0) | I2S_TCR4_MF | I2S_TCR4_FSP | I2S_TCR4_FSD;
	I2S0_TCR5 = I2S_TCR5_WNW(31) | I2S_TCR5_W0W(31) | I2S_TCR5_FBT(31);

	I2S0_RCSR = 0;

#if 0
	// configure pin mux for 3 clock signals (debug only)
	CORE_PIN23_CONFIG = PORT_PCR_MUX(6); // pin 23, PTC2, I2S0_TX_FS (LRCLK)
	CORE_PIN9_CONFIG  = PORT_PCR_MUX(6); // pin  9, PTC3, I2S0_TX_BCLK
//	CORE_PIN11_CONFIG = PORT_PCR_MUX(6); // pin 11, PTC6, I2S0_MCLK
#endif
}

/*

https://forum.pjrc.com/threads/38753-Discussion-about-a-simple-way-to-change-the-sample-rate

*/

void AudioOutputADAT::setI2SFreq(int freq) {
  typedef struct {
    uint8_t mult;
    uint16_t div;
  } tmclk;

  const int numfreqs = 14;
  const int samplefreqs[numfreqs] = { 8000, 11025, 16000, 22050, 32000, 44100, (int) 44117.64706 , 48000, 88200, (int) (44117.64706 * 2.0), 96000, 176400,(int) (44117.64706 * 4.0), 192000};

#if (F_PLL==16000000)
  const tmclk clkArr[numfreqs] = {{16, 125}, {148, 839}, {32, 125}, {145, 411}, {64, 125}, {151, 214}, {12, 17}, {96, 125}, {151, 107}, {24, 17}, {192, 125}, {127, 45}, {48, 17}, {255, 83} };
#elif (F_PLL==72000000)
  const tmclk clkArr[numfreqs] = {{32, 1125}, {49, 1250}, {64, 1125}, {49, 625}, {128, 1125}, {98, 625}, {8, 51}, {64, 375}, {196, 625}, {16, 51}, {128, 375}, {249, 397}, {32, 51}, {185, 271} };
#elif (F_PLL==96000000)
  const tmclk clkArr[numfreqs] = {{8, 375}, {73, 2483}, {16, 375}, {147, 2500}, {32, 375}, {147, 1250}, {2, 17}, {16, 125}, {147, 625}, {4, 17}, {32, 125}, {151, 321}, {8, 17}, {64, 125} };
#elif (F_PLL==120000000)
  const tmclk clkArr[numfreqs] = {{32, 1875}, {89, 3784}, {64, 1875}, {147, 3125}, {128, 1875}, {205, 2179}, {8, 85}, {64, 625}, {89, 473}, {16, 85}, {128, 625}, {178, 473}, {32, 85}, {145, 354} };
#elif (F_PLL==144000000)
  const tmclk clkArr[numfreqs] = {{16, 1125}, {49, 2500}, {32, 1125}, {49, 1250}, {64, 1125}, {49, 625}, {4, 51}, {32, 375}, {98, 625}, {8, 51}, {64, 375}, {196, 625}, {16, 51}, {128, 375} };
#elif (F_PLL==168000000)
  const tmclk clkArr[numfreqs] = {{32, 2625}, {21, 1250}, {64, 2625}, {21, 625}, {128, 2625}, {42, 625}, {8, 119}, {64, 875}, {84, 625}, {16, 119}, {128, 875}, {168, 625}, {32, 119}, {189, 646} };	
#elif (F_PLL==180000000)
  const tmclk clkArr[numfreqs] = {{46, 4043}, {49, 3125}, {73, 3208}, {98, 3125}, {183, 4021}, {196, 3125}, {16, 255}, {128, 1875}, {107, 853}, {32, 255}, {219, 1604}, {214, 853}, {64, 255}, {219, 802} };
#elif (F_PLL==192000000)
  const tmclk clkArr[numfreqs] = {{4, 375}, {37, 2517}, {8, 375}, {73, 2483}, {16, 375}, {147, 2500}, {1, 17}, {8, 125}, {147, 1250}, {2, 17}, {16, 125}, {147, 625}, {4, 17}, {32, 125} };
#elif (F_PLL==216000000)
  const tmclk clkArr[numfreqs] = {{32, 3375}, {49, 3750}, {64, 3375}, {49, 1875}, {128, 3375}, {98, 1875}, {8, 153}, {64, 1125}, {196, 1875}, {16, 153}, {128, 1125}, {226, 1081}, {32, 153}, {147, 646} };
#elif (F_PLL==240000000)
  const tmclk clkArr[numfreqs] = {{16, 1875}, {29, 2466}, {32, 1875}, {89, 3784}, {64, 1875}, {147, 3125}, {4, 85}, {32, 625}, {205, 2179}, {8, 85}, {64, 625}, {89, 473}, {16, 85}, {128, 625} };
#elif (F_PLL==256000000)
  // TODO: fix these...
  const tmclk clkArr[numfreqs] = {{16, 1875}, {29, 2466}, {32, 1875}, {89, 3784}, {64, 1875}, {147, 3125}, {4, 85}, {32, 625}, {205, 2179}, {8, 85}, {64, 625}, {89, 473}, {16, 85}, {128, 625} };
#endif

  for (int f = 0; f < numfreqs; f++) {
    if ( freq == samplefreqs[f] ) {
      while (I2S0_MCR & I2S_MCR_DUF) ;
      I2S0_MDR = I2S_MDR_FRACT((clkArr[f].mult - 1)) | I2S_MDR_DIVIDE((clkArr[f].div - 1));
      return;
    }
  }
}

#elif defined(KINETISL)

void AudioOutputADAT::update(void)
{

	audio_block_t *block;
	block = receiveReadOnly(0); // input 0 = ch1 channel
	if (block) release(block);
	block = receiveReadOnly(1); // input 1 = ch2 channel
	if (block) release(block);
	block = receiveReadOnly(2); // input 2 = ch3 channel
	if (block) release(block);
	block = receiveReadOnly(3); // input 3 = ch4 channel
	if (block) release(block);
	block = receiveReadOnly(4); // input 4 = ch5 channel
	if (block) release(block);
	block = receiveReadOnly(5); // input 5 = ch6 channel
	if (block) release(block);
	block = receiveReadOnly(6); // input 6 = ch7 channel
	if (block) release(block);
	block = receiveReadOnly(7); // input 7 = ch8 channel
	if (block) release(block);
}

#endif

