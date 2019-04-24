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

#if 0

#if TEENSYDUINO < 120
#error "Teensyduino version 1.20 or later is required to compile the Audio library."
#endif
#ifdef __AVR__
#error "The Audio Library only works with Teensy 3.X.  Teensy 2.0 is unsupported."
#endif

#include "DMAChannel.h"
#if !defined(DMACHANNEL_HAS_BEGIN) || !defined(DMACHANNEL_HAS_BOOLEAN_CTOR)
#error "You need to update DMAChannel.h & DMAChannel.cpp"
#error "https://github.com/PaulStoffregen/cores/blob/master/teensy3/DMAChannel.h"
#error "https://github.com/PaulStoffregen/cores/blob/master/teensy3/DMAChannel.cpp"
#endif

#endif //0

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
#include "analyze_fft1024.h"
#include "analyze_print.h"
#include "analyze_tonedetect.h"
#include "analyze_notefreq.h"
#include "analyze_peak.h"
#include "analyze_rms.h"
#include "control_sgtl5000.h"
#include "control_wm8731.h"
#include "control_ak4558.h"
#include "control_cs4272.h"
#include "control_cs42448.h"
#include "effect_bitcrusher.h"
#include "effect_chorus.h"
#include "effect_fade.h"
#include "effect_flange.h"
#include "effect_envelope.h"
#include "effect_multiply.h"
#include "effect_delay.h"
#include "effect_delay_ext.h"

#include "effect_midside.h"
#include "effect_reverb.h"
#include "effect_waveshaper.h"
#include "filter_biquad.h"
#include "filter_fir.h"
#include "filter_variable.h"
#include "input_adcs.h"

#if I2S_INTERFACES_COUNT > 0
#include "input_i2s.h"
#endif

#if 0
#include "input_adc.h"
#include "input_i2s_quad.h"
#include "input_tdm.h"
#endif //0

#include "mixer.h"
#include "output_dacs.h"

#if I2S_INTERFACES_COUNT > 0
#include "output_i2s.h"
#endif

#if 0
#include "output_dac.h"
#include "output_i2s_quad.h"
#include "output_pwm.h"
#include "output_spdif.h"
#include "output_pt8211.h"
#include "output_tdm.h"
#include "play_sd_raw.h"
#include "play_sd_wav.h"
#include "play_qspi_wav.h"
#endif //0

#include "play_memory.h"
#include "play_queue.h"


#if 0
#include "play_serialflash_raw.h"
#endif //0

#include "record_queue.h"
#include "synth_tonesweep.h"
#include "synth_sine.h"
#include "synth_waveform.h"
#include "synth_dc.h"
#include "synth_whitenoise.h"
#include "synth_pinknoise.h"
#include "synth_karplusstrong.h"
#include "synth_simple_drum.h"
#include "synth_pwm.h"

#endif
