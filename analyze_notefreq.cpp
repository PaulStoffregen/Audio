/* Audio Library Note Frequency Detection & Guitar/Bass Tuner
 * Copyright (c) 2015, Colin Duffy
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

#include "analyze_notefreq.h"
#include "utility/dspinst.h"
#include "arm_math.h"

#define HALF_BLOCKS AUDIO_GUITARTUNER_BLOCKS * 64

/**
 *  Copy internal blocks of data to class buffer
 *
 *  @param destination destination address
 *  @param source      source address
 */
static void copy_buffer(void *destination, const void *source) {
    const uint16_t *src = ( const uint16_t * )source;
    uint16_t *dst = (  uint16_t * )destination;
    for (int i=0; i < AUDIO_BLOCK_SAMPLES; i++)  *dst++ = (*src++);
}

/**
 *  Virtual function to override from Audio Library
 */
void AudioAnalyzeNoteFrequency::update( void ) {
    
    audio_block_t *block;
    
    block = receiveReadOnly();
    if (!block) return;
    
    if ( !enabled ) {
        release( block );
        return;
    }
    
    if ( next_buffer ) {
        blocklist1[state++] = block;
        if ( !first_run && process_buffer ) process( );
    } else {
        blocklist2[state++] = block;
        if ( !first_run && process_buffer ) process( );
    }
    
    if ( state >= AUDIO_GUITARTUNER_BLOCKS ) {
        if ( next_buffer ) {
            if ( !first_run && process_buffer ) process( );
            for ( int i = 0; i < AUDIO_GUITARTUNER_BLOCKS; i++ ) copy_buffer( AudioBuffer+( i * 0x80 ), blocklist1[i]->data );
            for ( int i = 0; i < AUDIO_GUITARTUNER_BLOCKS; i++ ) release( blocklist1[i] );
            next_buffer = false;
        } else {
            if ( !first_run && process_buffer ) process( );
            for ( int i = 0; i < AUDIO_GUITARTUNER_BLOCKS; i++ ) copy_buffer( AudioBuffer+( i * 0x80 ), blocklist2[i]->data );
            for ( int i = 0; i < AUDIO_GUITARTUNER_BLOCKS; i++ ) release( blocklist2[i] );
            next_buffer = true;
        }
        process_buffer = true;
        first_run = false;
        state = 0;
    }
    
}

/**
 *  Start the Yin algorithm
 *
 *  TODO: Significant speed up would be to use spectral domain to find fundamental frequency.
 *  This paper explains: https://aubio.org/phd/thesis/brossier06thesis.pdf -> Section 3.2.4
 *  page 79. Might have to downsample for low fundmental frequencies because of fft buffer
 *  size limit.
 */
void AudioAnalyzeNoteFrequency::process( void ) {
    
    const int16_t *p;
    p = AudioBuffer;
    
    uint16_t cycles = 64;
    uint16_t tau = tau_global;
    do {
        uint16_t x   = 0;
        uint64_t  sum = 0;
        do {
            int16_t current, lag, delta;
            lag = *( ( int16_t * )p + ( x+tau ) );
            current = *( ( int16_t * )p+x );
            delta = ( current-lag );
            sum += delta * delta;
            x += 4;
            
            lag = *( ( int16_t * )p + ( x+tau ) );
            current = *( ( int16_t * )p+x );
            delta = ( current-lag );
            sum += delta * delta;
            x += 4;
            
            lag = *( ( int16_t * )p + ( x+tau ) );
            current = *( ( int16_t * )p+x );
            delta = ( current-lag );
            sum += delta * delta;
            x += 4;
            
            lag = *( ( int16_t * )p + ( x+tau ) );
            current = *( ( int16_t * )p+x );
            delta = ( current-lag );
            sum += delta * delta;
            x += 4;
        } while ( x < HALF_BLOCKS );
        
        uint64_t rs = running_sum;
        rs += sum;
        yin_buffer[yin_idx] = sum*tau;
        rs_buffer[yin_idx] = rs;
        running_sum = rs;
        yin_idx = ( ++yin_idx >= 5 ) ? 0 : yin_idx;
        tau = estimate( yin_buffer, rs_buffer, yin_idx, tau );
        
        if ( tau == 0 ) {
            process_buffer  = false;
            new_output      = true;
            yin_idx         = 1;
            running_sum     = 0;
            tau_global      = 1;
            return;
        }
    } while ( --cycles );
    //digitalWriteFast(10, LOW);
    if ( tau >= HALF_BLOCKS ) {
        process_buffer  = false;
        new_output      = false;
        yin_idx         = 1;
        running_sum     = 0;
        tau_global      = 1;
        return;
    }
    tau_global = tau;
}

/**
 *  check the sampled data for fundamental frequency
 *
 *  @param yin  buffer to hold sum*tau value
 *  @param rs   buffer to hold running sum for sampled window
 *  @param head buffer index
 *  @param tau  lag we are currently working on gets incremented
 *
 *  @return tau
 */
uint16_t AudioAnalyzeNoteFrequency::estimate( uint64_t *yin, uint64_t *rs, uint16_t head, uint16_t tau ) {
    const uint64_t *y = ( uint64_t * )yin;
    const uint64_t *r = ( uint64_t * )rs;
    uint16_t _tau, _head;
    const float thresh = yin_threshold;
    _tau = tau;
    _head = head;
    
    if ( _tau > 4 ) {
        
        uint16_t idx0, idx1, idx2;
        idx0 = _head;
        idx1 = _head + 1;
        idx1 = ( idx1 >= 5 ) ? 0 : idx1;
        idx2 = head + 2;
        idx2 = ( idx2 >= 5 ) ? 0 : idx2;
        
        float s0, s1, s2;
        s0 = ( ( float )*( y+idx0 ) / *( r+idx0 ) );
        s1 = ( ( float )*( y+idx1 ) / *( r+idx1 ) );
        s2 = ( ( float )*( y+idx2 ) / *( r+idx2 ) );
        
        if ( s1 < thresh && s1 < s2 ) {
            uint16_t period = _tau - 3;
            periodicity = 1 - s1;
            data = period + 0.5f * ( s0 - s2 ) / ( s0 - 2.0f * s1 + s2 );
            return 0;
        }
    }
    return _tau + 1;
}

/**
 *  Initialise
 *
 *  @param threshold Allowed uncertainty
 */
void AudioAnalyzeNoteFrequency::begin( float threshold ) {
    __disable_irq( );
    process_buffer = false;
    yin_threshold  = threshold;
    periodicity    = 0.0f;
    next_buffer    = true;
    running_sum    = 0;
    tau_global     = 1;
    first_run      = true;
    yin_idx        = 1;
    enabled        = true;
    state          = 0;
    data           = 0.0f;
    __enable_irq( );
}

/**
 *  available
 *
 *  @return true if data is ready else false
 */
bool AudioAnalyzeNoteFrequency::available( void ) {
    __disable_irq( );
    bool flag = new_output;
    if ( flag ) new_output = false;
    __enable_irq( );
    return flag;
}

/**
 *  read processes the data samples for the Yin algorithm.
 *
 *  @return frequency in hertz
 */
float AudioAnalyzeNoteFrequency::read( void ) {
    __disable_irq( );
    float d = data;
    __enable_irq( );
    return AUDIO_SAMPLE_RATE_EXACT / d;
}

/**
 *  Periodicity of the sampled signal from Yin algorithm from read function.
 *
 *  @return periodicity
 */
float AudioAnalyzeNoteFrequency::probability( void ) {
    __disable_irq( );
    float p = periodicity;
    __enable_irq( );
    return p;
}

/**
 *  Initialise parameters.
 *
 *  @param thresh    Allowed uncertainty
 */
void AudioAnalyzeNoteFrequency::threshold( float p ) {
    __disable_irq( );
    yin_threshold = p;
    __enable_irq( );
}
