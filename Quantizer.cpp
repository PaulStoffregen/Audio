/* Audio Library for Teensy 3.X
 * Copyright (c) 2019, Paul Stoffregen, paul@pjrc.com
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
/*
 by Alexander Walch
 */
#include "Quantizer.h"

#define SAMPLEINVALID(sample) (!isfinite(sample) || abs(sample) >= 1.2)	//use only for floating point samples (\in [-1.,1.])

Quantizer::Quantizer(float audio_sample_rate){
#ifdef DEBUG_QUANTIZER
    while(!Serial);
#endif
    randomSeed(1);
    if(audio_sample_rate==44100.f){
        _noiseSFilter[0]=-0.06935825f;
        _noiseSFilter[1]=0.52540845f;
        _noiseSFilter[2]= -1.20537028f;
        _noiseSFilter[3]=  2.09422811f;
        _noiseSFilter[4]= -3.2177438f;
        _noiseSFilter[5]= 4.04852027f;
        _noiseSFilter[6]= -3.83872701f;
        _noiseSFilter[7]= 3.30584589f;
        _noiseSFilter[8]= -2.38682527f;
    //  all coefficients in correct order:
    //      {1.        , -2.38682527,  3.30584589, -3.83872701,  4.04852027,
    //       -3.2177438 ,  2.09422811, -1.20537028,  0.52540845, -0.06935825};
    } else if(audio_sample_rate==48000.f){
        _noiseSFilter[0]=0.1967454f;
        _noiseSFilter[1]=-0.30086406f;
        _noiseSFilter[2]= 0.09575588f;
        _noiseSFilter[3]=  0.58209648f;
        _noiseSFilter[4]= -1.88579617f;
        _noiseSFilter[5]= 3.37325788f;
        _noiseSFilter[6]= -3.88076402f;
        _noiseSFilter[7]= 3.58558504f;
        _noiseSFilter[8]= -2.54334066f;
        //  all coefficients in correct order:
    //      {1.        , -2.54334066,  3.58558504, -3.88076402,  3.37325788,
    //      -1.88579617,  0.58209648,  0.09575588, -0.30086406,  0.1967454};
    //    }
    }
	else {
		_noiseSFilter[0]=0.f;
        _noiseSFilter[1]=0.f;
        _noiseSFilter[2]=0.f;
        _noiseSFilter[3]=0.f;
        _noiseSFilter[4]=0.f;
        _noiseSFilter[5]=0.f;
        _noiseSFilter[6]=0.f;
        _noiseSFilter[7]=0.f;
        _noiseSFilter[8]=0.f;
	}
    _bufferEnd0=&_buffer0[NOISE_SHAPE_F_LENGTH];
    reset();
}

void Quantizer::configure(bool noiseShaping, bool dither, float factor){
     _noiseShaping=noiseShaping;
     _dither=dither;
     _factor=factor;
     reset();
}
void Quantizer::reset(){
     _bPtr0=_buffer0;
     _bPtr1=_buffer1;
     _fOutputLastIt0=0.;
     _fOutputLastIt1=0.;
     memset(_buffer0, 0, NOISE_SHAPE_F_LENGTH*sizeof(float));
     memset(_buffer1, 0, NOISE_SHAPE_F_LENGTH*sizeof(float));
}
void Quantizer::quantize(float* input, int16_t* output, uint16_t length){
    float xn, xnD, error;
    const float f2 = 1.f/1000000.f;
#ifdef DEBUG_QUANTIZER
    float debugFF=1024.f;
    const float factor=(powf(2.f, 15.f)-1.f)/debugFF;
#endif

    for (uint16_t i =0; i< length; i++){
        xn= SAMPLEINVALID(*input) ? 0. : *input*_factor; //-_fOutputLastIt0 according to paper     
        ++input;
        if (_noiseShaping){
            xn+=_fOutputLastIt0;
        }
        if(_dither){
            const uint32_t r0=random(1000000);
            const uint32_t r1=random(1000000);
            xnD=xn + (r0 + r1)*f2-1.f;
        }
        else {
            xnD=xn;
        }
        float xnDR=round(xnD);
        if (_noiseShaping){
            //compute quatization error:
            error=xnDR- xn;
            *_bPtr0++=error;
            if (_bPtr0==_bufferEnd0){
                _bPtr0=_buffer0;
            }
            float* f=_noiseSFilter;
            _fOutputLastIt0=(*_bPtr0++ * *f++);
            if (_bPtr0==_bufferEnd0){
                _bPtr0=_buffer0;
            }
            for (uint16_t j =1; j< NOISE_SHAPE_F_LENGTH; j++){
                _fOutputLastIt0+=(*_bPtr0++ * *f++);
                if (_bPtr0==_bufferEnd0){
                    _bPtr0=_buffer0;
                }
            }
        }
#ifdef DEBUG_QUANTIZER
        xnDR*=debugFF;
#endif
        if (xnDR > _factor){
            *output=(int16_t)_factor;
        }
        else if (xnDR < -_factor){
            *output=(int16_t)_factor;
        }
        else {
            *output=(int16_t)xnDR;
        }

        ++output;
    }
}

void Quantizer::quantize(float* input0, float* input1, int32_t* outputInterleaved, uint16_t length){
    float xn0, xnD0, error0,xnDR0, xn1, xnD1, error1,xnDR1;
    const float f2 = 1.f/1000000.f;
#ifdef DEBUG_QUANTIZER
    float debugFF=1024.f;
    const float factor=(powf(2.f, 15.f)-1.f)/debugFF;
#endif
    for (uint16_t i =0; i< length; i++){
        xn0= SAMPLEINVALID(*input0) ? 0. : *input0*_factor; //-_fOutputLastIt0 according to paper        
        ++input0;
        xn1= SAMPLEINVALID(*input1) ? 0. : *input1*_factor; //-_fOutputLastIt0 according to paper  
        ++input1;
        if (_noiseShaping){
            xn0+=_fOutputLastIt0;
            xn1+=_fOutputLastIt1;
        }
        if(_dither){
            uint32_t r0=random(1000000);
            uint32_t r1=random(1000000);
            xnD0=xn0 + (r0 + r1)*f2-1.f;
            r0=random(1000000);
            r1=random(1000000);
            xnD1=xn1 + (r0 + r1)*f2-1.f;
        }
        else {
            xnD0=xn0;
            xnD1=xn1;
        }
        xnDR0=round(xnD0);
        xnDR1=round(xnD1);
        if (_noiseShaping){
            //compute quatization error0:
            error0=xnDR0- xn0;
            error1=xnDR1- xn1;
            *_bPtr0++=error0;
            *_bPtr1++=error1;
            if (_bPtr0==_bufferEnd0){
                _bPtr0=_buffer0;
                _bPtr1=_buffer1;
            }
            float* f=_noiseSFilter;
            _fOutputLastIt0=(*_bPtr0++ * *f);
            _fOutputLastIt1=(*_bPtr1++ * *f++);
            if (_bPtr0==_bufferEnd0){
                _bPtr0=_buffer0;
                _bPtr1=_buffer1;
            }
            for (uint16_t j =1 ; j< NOISE_SHAPE_F_LENGTH; j++){
                _fOutputLastIt0+=(*_bPtr0++ * *f);
                _fOutputLastIt1+=(*_bPtr1++ * *f++);
                if (_bPtr0==_bufferEnd0){
                    _bPtr0=_buffer0;
                    _bPtr1=_buffer1;
                }
            }
        }
#ifdef DEBUG_QUANTIZER
        xnDR0*=debugFF;
#endif
        if (xnDR0 > _factor){
            *outputInterleaved++=(int32_t)_factor;
        }
        else if (xnDR0 < -_factor){
            *outputInterleaved++=-(int32_t)_factor;
        }
        else {
            *outputInterleaved++=(int32_t)xnDR0;
        }
        if (xnDR1 > _factor){
            *outputInterleaved++=(int32_t)_factor;
        }
        else if (xnDR1 < -_factor){
            *outputInterleaved++=-(int32_t)_factor;
        }
        else {
            *outputInterleaved++=(int32_t)xnDR1;
        }
    }
}