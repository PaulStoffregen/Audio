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

//note:
//envelope generator effect.
//the module generates an attack/decay/sustain/release-envelope and applies it to the incoming signal.
//to use as standalone

#ifndef env_h
#define env_h
#include "audiostream.h"
#include "arm_math.h"
#include "utility/dspinst.h"

#define ENVSTATE_SILENT 0
#define ENVSTATE_ATTACK 1
#define ENVSTATE_DECAY 2
#define ENVSTATE_SUSTAIN 3
#define ENVSTATE_RELEASE 4

const uint32_t  ENV_MAX_LEVEL = 4294967295;//2^32 - 1
const uint32_t  ENV_MAX_ATTACK = 20000;//in milliseconds
const uint32_t  ENV_MAX_DECAY = 20000;//in milliseconds
const uint32_t  ENV_MAX_RELEASE = 20000;//in milliseconds
//noteOn -> attack state
//attack -> decay
//decay -> sustain
//noteOff -> release
//release -> silent
class AudioEffectEnvelope : public AudioStream
{
public:
    AudioEffectEnvelope() : AudioStream(1, inputQueueArray)
    {
        envLevel = 0;
        //some init values.
        currentState = ENVSTATE_SILENT;
        setAttack(1.0);
        setDecay(20.0);
        setSustain(0.6);
        setRelease(20.0);
    }
    void noteOn()
    {
        envLevel = 0;//starts at zero, start the attack phase
        currentState = ENVSTATE_ATTACK;
    }
    void noteOff()
    {
        currentState = ENVSTATE_RELEASE;
    }
    void setAttack(float mSeconds);//1 - 20000 milliseconds
    void setDecay(float mSeconds);//1 - 20000 milliseconds
    void setSustain(float level);//float form 0.0 to 1.0
    void setRelease(float mSeconds);
    virtual void update(void);
private:
    audio_block_t *inputQueueArray[1];
    uint8_t currentState;
    uint32_t envLevel;
    uint32_t attackInc;
    uint32_t decayDec;
    uint32_t sustainLevel;
    uint32_t releaseDec;
};
#endif
