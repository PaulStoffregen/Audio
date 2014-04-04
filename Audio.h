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

#ifndef Audio_h_
#define Audio_h_

// When changing multiple audio object settings that must update at
// the same time, these functions allow the audio library interrupt
// to be disabled.  For example, you may wish to begin playing a note
// in response to reading an analog sensor.  If you have "velocity"
// information, you might start the sample playing and also adjust
// the gain of a mixer channel.  Use AudioNoInterrupts() first, then
// make both changes to the 2 separate objects.  Then allow the audio
// library to update with AudioInterrupts().  Both changes will happen
// at the same time, because AudioNoInterrupts() prevents any updates
// while you make changes.
//
#define AudioNoInterrupts() (NVIC_DISABLE_IRQ(IRQ_SOFTWARE))
#define AudioInterrupts()   (NVIC_ENABLE_IRQ(IRQ_SOFTWARE))


// include all the library headers, so a sketch can use a single
// #include <Audio.h> to get the whole library
//
#include "analyze_fft256.h"
#include "analyze_print.h"
#include "analyze_tonedetect.h"
#include "analyze_peakdetect.h"
#include "control_sgtl5000.h"
#include "control_wm8731.h"
#include "effect_chorus.h"
#include "effect_fade.h"
#include "effect_flange.h"
#include "filter_biquad.h"
#include "filter_fir.h"
#include "input_adc.h"
#include "input_i2s.h"
#include "mixer.h"
#include "output_dac.h"
#include "output_i2s.h"
#include "output_pwm.h"
#include "play_memory.h"
#include "play_sd_raw.h"
#include "play_sd_wav.h"
#include "synth_tonesweep.h"
#include "synth_waveform.h"

#include "record_sd_wav.h"
#include "synth_samples.h"
#include "play_flash_wav.h"
#include "flash_spi.h"

// TODO: more audio processing objects....
//  sine wave with frequency modulation (phase)
//  waveforms with bandwidth limited tables for synth
//  envelope: attack-decay-sustain-release, maybe other more complex?

#endif
