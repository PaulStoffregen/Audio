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

#ifndef AudioAnalyzeNoteFrequency_h_
#define AudioAnalyzeNoteFrequency_h_

#include "Arduino.h"
#include "AudioStream.h"
/***********************************************************************
 *              Safe to adjust these values below                      *
 *                                                                     *
 *  This parameter defines the size of the buffer.                     *
 *                                                                     *
 *  1.  AUDIO_GUITARTUNER_BLOCKS -  Buffer size is 128 * AUDIO_BLOCKS. *
 *                      The more AUDIO_GUITARTUNER_BLOCKS the lower    *
 *                      the frequency you can detect. The default      *
 *                      (24) is set to measure down to 29.14 Hz        *
 *                      or B(flat)0.                                   *
 *                                                                     *
 ***********************************************************************/
#define AUDIO_GUITARTUNER_BLOCKS  24
/***********************************************************************/
class AudioAnalyzeNoteFrequency : public AudioStream {
public:
    /**
     *  constructor to setup Audio Library and initialize
     *
     *  @return none
     */
    AudioAnalyzeNoteFrequency( void ) : AudioStream( 1, inputQueueArray ), enabled( false ), new_output(false) {
        
    }
    
    /**
     *  initialize variables and start conversion
     *
     *  @param threshold Allowed uncertainty
     *  @param cpu_max   How much cpu usage before throttling
     *
     *  @return none
     */
    void begin( float threshold );
    
    /**
     *  sets threshold value
     *
     *  @param thresh
     *  @return none
     */
    void threshold( float p );
    
    /**
     *  triggers true when valid frequency is found
     *
     *  @return flag to indicate valid frequency is found
     */
    bool available( void );
    /**
     *  get frequency
     *
     *  @return frequency in hertz
     */
    float read( void );
    
    /**
     *  get predicitity
     *
     *  @return probability of frequency found
     */
    float probability( void );
    
    /**
     *  Audio Library calls this update function ~2.9ms
     *
     *  @return none
     */
    virtual void update( void );
    
private:
    /**
     *  check the sampled data for fundamental frequency
     *
     *  @param yin  buffer to hold sum*tau value
     *  @param rs   buffer to hold running sum for sampled window
     *  @param head buffer index
     *  @param tau  lag we are currently working on this gets incremented
     *
     *  @return tau
     */
    uint16_t estimate( uint64_t *yin, uint64_t *rs, uint16_t head, uint16_t tau );
    
    /**
     *  process audio data
     *
     *  @return none
     */
    void process( void );
    
    /**
     *  Variables
     */
    uint64_t running_sum;
    uint16_t tau_global;
    uint64_t  yin_buffer[5];
    uint64_t  rs_buffer[5];
    int16_t  AudioBuffer[AUDIO_GUITARTUNER_BLOCKS*128] __attribute__ ( ( aligned ( 4 ) ) );
    uint8_t  yin_idx, state;
    float    periodicity, yin_threshold, cpu_usage_max, data;
    bool     enabled, next_buffer, first_run;
    volatile bool new_output, process_buffer;
    audio_block_t *blocklist1[AUDIO_GUITARTUNER_BLOCKS];
    audio_block_t *blocklist2[AUDIO_GUITARTUNER_BLOCKS];
    audio_block_t *inputQueueArray[1];
};
#endif
