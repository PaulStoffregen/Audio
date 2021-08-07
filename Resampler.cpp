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

#include "Resampler.h"
#include <math.h>

Resampler::Resampler(float attenuation, int32_t minHalfFilterLength, int32_t maxHalfFilterLength, StepAdaptionParameters settings): _targetAttenuation(attenuation)
{
	_maxHalfFilterLength=max(1, min(MAX_HALF_FILTER_LENGTH, maxHalfFilterLength));
	_minHalfFilterLength=max(1, min(maxHalfFilterLength, minHalfFilterLength));
#ifdef DEBUG_RESAMPLER
	while (!Serial);
#endif
    _settings=settings;
    kaiserWindowSamples[0]=1.;
    double step=1./(NO_EXACT_KAISER_SAMPLES-1);
    double* xSq=kaiserWindowXsq;
    for (uint16_t i = 1; i <NO_EXACT_KAISER_SAMPLES; i++){
        double x=(double)i*step;
        *xSq++=(1.-x*x);
    }
}
void Resampler::getKaiserExact(double beta){
    const double thres=1e-10;   
    double* winS=&kaiserWindowSamples[1];
    double* t=tempRes;
    for (uint16_t i = 1; i <NO_EXACT_KAISER_SAMPLES; i++){
        *winS++=1.;
        *t++=1.;
    }
    double denomLastSummand=1.;
    const double halfBetaSq=beta*beta/4.;
    double denom=1.;
    double i=1.;
    while(i < 1000){
        denomLastSummand*=(halfBetaSq/(i*i));
        i+=1.;
        denom+=denomLastSummand;
        t=tempRes;
        winS=&kaiserWindowSamples[1];
        double* xSq=kaiserWindowXsq;
        for (uint16_t j=0; j<  NO_EXACT_KAISER_SAMPLES-1;j++){
            (*t)*=(*xSq);
            double summand=(denomLastSummand*(*t));
            (*winS)+=summand;
            if (summand< thres){
                break;
            }
            ++winS;
            ++t;
            ++xSq;
        }  
        if (denomLastSummand< thres){
            break;
        }
    }
    winS=&kaiserWindowSamples[1];
    for (int32_t i = 0; i <NO_EXACT_KAISER_SAMPLES-1; i++){
        *winS++/=denom;
    }
}
    
void Resampler::setKaiserWindow(double beta, int32_t noSamples){
    getKaiserExact(beta);
    double step=(double)(NO_EXACT_KAISER_SAMPLES-1)/(double)(noSamples-1);
    double xPos=step;
    float* filterCoeff=filter;
    *filterCoeff=1.;
    ++filterCoeff;
    int32_t lower=(int)(xPos);
    double* windowLower=&kaiserWindowSamples[lower];
    double* windowUpper=&kaiserWindowSamples[lower+1];
    for (int32_t i =0; i< noSamples-2; i++){
        double lambda=xPos-lower;
        if (lambda > 1.){
            lambda-=1.;
            ++windowLower;
            ++windowUpper;
            lower++;
        }
        *filterCoeff++=(float)(lambda*(*windowUpper)+(1.-lambda)*(*windowLower));
        xPos+=step;
        if (xPos>=NO_EXACT_KAISER_SAMPLES-1 || lower >=NO_EXACT_KAISER_SAMPLES-1){
            break;
        }
    }
    *filterCoeff=*windowUpper;
}

void Resampler::setFilter(int32_t halfFiltLength,int32_t overSampling, double cutOffFrequ, double kaiserBeta){

    const int32_t noSamples=halfFiltLength*overSampling+1;
    setKaiserWindow(kaiserBeta, noSamples);  
    
    float* filterCoeff=filter;
    *filterCoeff++=(float)cutOffFrequ;
    double step=halfFiltLength/(noSamples-1.);
    double xPos=step;
    double factor=M_PI*cutOffFrequ;
    for (int32_t i = 0; i<noSamples-1; i++ ){
        *filterCoeff++*=(float)((sin(xPos*factor)/(xPos*M_PI)));        
        xPos+=step;
    } 
}

double Resampler::getStep() const {
    return  _stepAdapted;
}
double Resampler::getAttenuation() const {
    return  _attenuation;
}
int32_t Resampler::getHalfFilterLength() const{
	return  _halfFilterLength;
}
void Resampler::reset(){
    _initialized=false;
}
void Resampler::configure(float fs, float newFs){
    // Serial.print("configure, fs: ");
    // Serial.println(fs);
    if (fs<=0.f || newFs <=0.f){
		_attenuation=0.;
		_halfFilterLength=0;
        _initialized=false;
        return;
    }
	_attenuation=_targetAttenuation;
    _step=(double)fs/(double)newFs;
    _configuredStep=_step;
    _stepAdapted=_step;
    _sum=0.;
    _oldDiffs[0]=0.;
    _oldDiffs[1]=0.;
    for (uint8_t i =0; i< MAX_NO_CHANNELS; i++){
        memset(_buffer[i], 0, sizeof(float)*_maxHalfFilterLength*2);
    }

    double kaiserBeta, cutOffFrequ;
    _overSamplingFactor=1024;
    if (fs <= newFs){
		_attenuation=0;
        cutOffFrequ=1.;
        kaiserBeta=10.;
        _halfFilterLength=_minHalfFilterLength;
    }
    else{
        cutOffFrequ=newFs/fs;
        double b=(2.*(0.5*(double)newFs-20000.)/(double)fs);   //this transition band width causes aliasing. However the generated frequencies are above 20kHz
#ifdef DEBUG_RESAMPLER
        Serial.print("b: ");
        Serial.println(b);
#endif
        int32_t hfl=(int32_t)((_attenuation-8.)/(2.*2.285*TWO_PI*b)+0.5);
        if (hfl >= _minHalfFilterLength && hfl <= _maxHalfFilterLength){
            _halfFilterLength=hfl;
        }
        else if (hfl < _minHalfFilterLength){
            _halfFilterLength=_minHalfFilterLength;
            _attenuation=((2.*(double)_halfFilterLength+1.)-1.)*(2.285*TWO_PI*b)+8.;            
        }
        else{
            _halfFilterLength=_maxHalfFilterLength;
            _attenuation=((2.*(double)_halfFilterLength+1.)-1.)*(2.285*TWO_PI*b)+8.;
        }
        if (_attenuation>50.){
            kaiserBeta=0.1102*(_attenuation-8.7);
        }
        else if (21.<=_attenuation && _attenuation<=50.){
            kaiserBeta=0.5842*pow(_attenuation-21.,0.4)+0.07886*(_attenuation-21.);
        }
        else{
            kaiserBeta=0.;
        }
        int32_t noSamples=_halfFilterLength*_overSamplingFactor+1;
        if (noSamples > MAX_FILTER_SAMPLES){
            int32_t f = (noSamples-1)/(MAX_FILTER_SAMPLES-1)+1;
            _overSamplingFactor/=f;
        }
    }

#ifdef DEBUG_RESAMPLER
    Serial.print("fs: ");
    Serial.println(fs);
    Serial.print("cutOffFrequ: ");
    Serial.println(cutOffFrequ);
    Serial.print("filter length: ");
    Serial.println(2*_halfFilterLength+1);
    Serial.print("overSampling: ");
    Serial.println(_overSamplingFactor);
    Serial.print("kaiserBeta: ");
    Serial.println(kaiserBeta, 12);
    Serial.print("_step: ");
    Serial.println(_step, 12);
#endif
    setFilter(_halfFilterLength, _overSamplingFactor, cutOffFrequ, kaiserBeta);
    _filterLength=_halfFilterLength*2;
    for (uint8_t i =0; i< MAX_NO_CHANNELS; i++){
        _endOfBuffer[i]=&_buffer[i][_filterLength];
    }
    _cPos=-_halfFilterLength;   //marks the current center position of the filter
    _initialized=true;
}
bool Resampler::initialized() const {
    return _initialized;
}

void Resampler::resample(float* input0, float* input1, uint16_t inputLength, uint16_t& processedLength, float* output0, float* output1,uint16_t outputLength, uint16_t& outputCount) {
    outputCount=0;
    int32_t successorIndex=(int32_t)(ceil(_cPos));  //negative number -> currently the _buffer0 of the last iteration is used
    float* ip0, *ip1, *fPtr;
    float filterC;
    float si0[2];
    float si1[2];
    while (floor(_cPos + _halfFilterLength) < inputLength && outputCount < outputLength){
        float dist=successorIndex-_cPos;
            
        const float distScaled=dist*_overSamplingFactor;
        int32_t rightIndex=abs((int32_t)(ceilf(distScaled))-_overSamplingFactor*_halfFilterLength);   
        const int32_t indexData=successorIndex-_halfFilterLength;
        if (indexData>=0){
            ip0=input0+indexData;
            ip1=input1+indexData;
        }  
        else {
            ip0=_buffer[0]+indexData+_filterLength; 
            ip1=_buffer[1]+indexData+_filterLength; 
        }       
        fPtr=filter+rightIndex;
        if (rightIndex==_overSamplingFactor*_halfFilterLength){
            si1[0]=*ip0++**fPtr;
            si1[1]=*ip1++**fPtr;
            memset(si0, 0, 2*sizeof(float));   
            fPtr-=_overSamplingFactor;          
            rightIndex=(int32_t)(ceilf(distScaled))+_overSamplingFactor;     //needed below  
        }
        else {
            memset(si0, 0, 2*sizeof(float));
            memset(si1, 0, 2*sizeof(float));
            rightIndex=(int32_t)(ceilf(distScaled));     //needed below
        }
        for (uint16_t i =0 ; i<_halfFilterLength; i++){
            if(ip0==_endOfBuffer[0]){
                ip0=input0;
                ip1=input1;
            }
            si1[0]+=*ip0**fPtr;            
            si1[1]+=*ip1**fPtr;
            filterC=*(fPtr+1);
            si0[0]+=*ip0*filterC;
            si0[1]+=*ip1*filterC;     
            fPtr-=_overSamplingFactor;      
            ++ip0;
            ++ip1;
        }
        fPtr=filter+rightIndex-1;
        for (uint16_t i =0 ; i<_halfFilterLength; i++){  
            if(ip0==_endOfBuffer[0]){
                ip0=input0;
                ip1=input1;
            }
            si0[0]+=*ip0**fPtr;
            si0[1]+=*ip1**fPtr;
            filterC=*(fPtr+1);
            si1[0]+=*ip0*filterC;            
            si1[1]+=*ip1*filterC;
            fPtr+=_overSamplingFactor;
            ++ip0;
            ++ip1;
        }
        const float w0=ceilf(distScaled)-distScaled;
        const float w1=1.f-w0;
        *output0++=si0[0]*w0 + si1[0]*w1;
        *output1++=si0[1]*w0 + si1[1]*w1;

        outputCount++;

        _cPos+=_stepAdapted;
        while (_cPos >successorIndex){
            successorIndex++;
        }
    }
    if(outputCount < outputLength){
        //ouput vector not full -> we ran out of input samples
        processedLength=inputLength;
    }
    else{
        processedLength=min(inputLength, (int16_t)floor(_cPos + _halfFilterLength));
    }
    //fill _buffer
    const int32_t indexData=processedLength-_filterLength;
    if (indexData>=0){
        ip0=input0+indexData;
        ip1=input1+indexData;
        const unsigned long long bytesToCopy= _filterLength*sizeof(float);
        memcpy((void *)_buffer[0], (void *)ip0, bytesToCopy);
        memcpy((void *)_buffer[1], (void *)ip1, bytesToCopy);    
    }  
    else {
        float* b0=_buffer[0];
        float* b1=_buffer[1];
        ip0=_buffer[0]+indexData+_filterLength; 
        ip1=_buffer[1]+indexData+_filterLength;
        for (uint16_t i =0; i< _filterLength; i++){
            if(ip0==_endOfBuffer[0]){
                ip0=input0;
                ip1=input1;
            }        
            *b0++ = *ip0++;
            *b1++ = *ip1++;
        } 
    }
    _cPos-=processedLength;
    if (_cPos < -_halfFilterLength){
        _cPos=-_halfFilterLength;
    }
}

void Resampler::fixStep(){
    if (!_initialized){
        return;
    }
    _step=_stepAdapted;
    _sum=0.;
    _oldDiffs[0]=0.;
    _oldDiffs[1]=0.;
}
void Resampler::addToPos(double val){
    if(val < 0){
        return;
    }
    _cPos+=val;
}

bool Resampler::addToSampleDiff(double diff){
   
    _oldDiffs[0]=_oldDiffs[1];
    _oldDiffs[1]=(1.-_settings.alpha)*_oldDiffs[1]+_settings.alpha*diff;
    const double slope=_oldDiffs[1]-_oldDiffs[0];
    _sum+=diff;
    double correction=_settings.kp*diff+_settings.kd*slope+_settings.ki*_sum;
    const double oldStepAdapted=_stepAdapted;
    _stepAdapted=_step+correction;
   
    if (abs(_stepAdapted/_configuredStep-1.) > _settings.maxAdaption){
        _initialized=false;
        return false;
    }
   
    bool settled=false;
    
    if ((abs(oldStepAdapted- _stepAdapted)/_stepAdapted < _settledThrs*abs(diff) && abs(diff) > 1.5*1e-6)) {
        settled=true;
    }
    return settled;
}

double Resampler::getXPos() const{
    return _cPos+(double)_halfFilterLength;
}
