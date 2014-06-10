/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Roel (mrHectometer)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#include "effect_envelope.h"

//set ADSR values. input in milliseconds for time
//the sustain accepts a float from 0 to 1 (no sustain to full sustain)
void AudioEffectEnvelope::setAttack(float mSeconds)
{
    if(mSeconds < 1 || mSeconds > ENV_MAX_ATTACK) return;
    attackInc = 4294967295/(AUDIO_SAMPLE_RATE_EXACT*mSeconds*0.001);
}
void AudioEffectEnvelope::setDecay(float mSeconds)
{
    if(mSeconds < 1 || mSeconds > ENV_MAX_DECAY) return;
    decayDec = 4294967295/(AUDIO_SAMPLE_RATE_EXACT*mSeconds*0.001);
}
void AudioEffectEnvelope::setSustain(float level)
{
    if(level < 0 || level > 1) return;
    sustainLevel = level*4294967295;
}
void AudioEffectEnvelope::setRelease(float mSeconds)
{
    if(mSeconds < 1 || mSeconds > ENV_MAX_RELEASE) return;
    releaseDec = 4294967295/(AUDIO_SAMPLE_RATE_EXACT*mSeconds*0.001);
}
void AudioEffectEnvelope::update(void)
{
    audio_block_t *block;
    uint32_t i;
    int32_t  val, sample;

    block = receiveWritable();
    if (!block) return;
    for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) 
    {
        switch(currentState)
        {
            //silent state: do nothing
        case ENVSTATE_SILENT:
            break;
            //attack    
        case ENVSTATE_ATTACK:
            envLevel+=attackInc;
            if(envLevel < attackInc)//overflow
            {
                currentState = ENVSTATE_DECAY;
                envLevel = ENV_MAX_LEVEL;//Maximal level
            }
            break;
            //decay phase    
        case ENVSTATE_DECAY:
            envLevel-=decayDec;
            if(envLevel < sustainLevel || envLevel > (ENV_MAX_LEVEL-decayDec))//under sustain level or underflow(if no sustain)
            {
                currentState = ENVSTATE_DECAY;
                envLevel = sustainLevel;//set to the sustain level
            }
            break; 
            //sustain: do nothing until noteOff   
        case ENVSTATE_SUSTAIN:
            break;    
            //release: decrease until we have an underflow
        case ENVSTATE_RELEASE:
            envLevel-=releaseDec;
            if(envLevel > (ENV_MAX_LEVEL-releaseDec))
            {
                currentState = ENVSTATE_SILENT;
                envLevel = 0;
            }
            break;    
            //default: niksdoen    
        default:
            break;
        }
        val = envLevel >> 16;//envLevel is 32 bits, so we have to shift
        
        //if used as envelope effect
        sample = (sample * val) >> 16;
        sample = block->data[i];
        block->data[i] = sample;
        //if used as envelope generator
        //block->data[i] = val;
    }
    transmit(block);
    release(block);
}

