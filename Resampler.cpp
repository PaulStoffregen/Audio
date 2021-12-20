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
   kaiserWindowXsq = new float[NO_EXACT_KAISER_SAMPLES-1];

	_maxHalfFilterLength=max(1, min(MAX_HALF_FILTER_LENGTH, maxHalfFilterLength));
	_minHalfFilterLength=max(1, min(maxHalfFilterLength, minHalfFilterLength));
#ifdef DEBUG_RESAMPLER
	while (!Serial);
#endif
    _settings=settings;

    double step=1./(NO_EXACT_KAISER_SAMPLES-1.);
    float* xSq=kaiserWindowXsq;
    for (uint16_t i = 1; i <NO_EXACT_KAISER_SAMPLES; i++){
        double x=i*step;
        *xSq++= (float)(1.-x*x);
    }
}
Resampler::~Resampler(){
    delete [] kaiserWindowXsq;
}
void Resampler::getKaiserExact(float*kaiserWindowSamples, double beta){
    const float thres=0.00000001f;   
    float* winS=kaiserWindowSamples;
    *winS++ =1.f;
    float tempRes[NO_EXACT_KAISER_SAMPLES-1];
    float* t=tempRes;
    for (uint16_t i = 1; i <NO_EXACT_KAISER_SAMPLES; i++){
        *winS++=1.f;
        *t++=1.f;
    }
    float denomLastSummand=1.f;
    const float halfBetaSq=beta*beta*0.25f;
    float denom=1.f;
    for (uint32_t i =1; i < 1000; i++){
        denomLastSummand*=(halfBetaSq/(float)(i*i));
        denom+=denomLastSummand;
        t=tempRes;
        winS=kaiserWindowSamples+1;
        float* xSq=kaiserWindowXsq;
        for (uint16_t j=1; j<  NO_EXACT_KAISER_SAMPLES;j++){
            (*t)*=(*xSq);
            float summand=(denomLastSummand* *t++);
            *winS+=summand;
            if (summand< thres){
                break;
            }
            ++winS;
            ++xSq;
        }  
        if (denomLastSummand< thres){
            break;
        }
    }
    winS=kaiserWindowSamples+1;
    denom=1.f/denom;
    for (int32_t i = 0; i <NO_EXACT_KAISER_SAMPLES-1; i++){
        *winS++*=denom;
    }
}
    
void Resampler::setKaiserWindow(double beta, int32_t noSamples){
    float kaiserWindowSamples[NO_EXACT_KAISER_SAMPLES];
    getKaiserExact(kaiserWindowSamples, beta);
    double step=(double)(NO_EXACT_KAISER_SAMPLES-1)/(double)(noSamples-1);
    double xPos=step;
    float* filterCoeff=filter;
    *filterCoeff++=1.f;
    int32_t lower=(int32_t)(xPos);
    float* windowLower=&kaiserWindowSamples[lower];
    float* windowUpper=&kaiserWindowSamples[lower+1];
    for (int32_t i =0; i< noSamples-2; i++){
        float lambda=(float)xPos-(float)lower;
        if (lambda > 1.f){
            lambda-=1.f;
            ++windowLower;
            ++windowUpper;
            lower++;
        }
        *filterCoeff++=(lambda*(*windowUpper)+(1.f-lambda)*(*windowLower));
        xPos+=step;
        if (xPos>=NO_EXACT_KAISER_SAMPLES-1 || lower >=NO_EXACT_KAISER_SAMPLES-1){
            break;
        }
    }
    *filterCoeff=*windowUpper;
}

void Resampler::setFilter(int32_t halfFiltLength,int32_t overSampling, double cutOffFrequ, double kaiserBeta){

    const int32_t noSamples=halfFiltLength*overSampling+1;
    
	//uint32_t beforKaiser=ARM_DWT_CYCCNT;
    setKaiserWindow(kaiserBeta, noSamples); 
	// uint32_t afterKaiser=ARM_DWT_CYCCNT; 
    // if(Serial){
    //    Serial.print("time for Kaiser: ");
    //    double d=(afterKaiser-beforKaiser)/(double)F_CPU_ACTUAL;
    //    Serial.println(d*1e6);
    // }
    
    float* filterCoeff=filter;
    *filterCoeff++=(float)cutOffFrequ;
    double step=halfFiltLength/(noSamples-1.);
    double xPos=step;
    double factor=M_PI*cutOffFrequ;
    for (int32_t i = 1; i<noSamples; i++ ){
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
void Resampler::configure(double fs, double newFs){
    
    if (fs<=0. || newFs * 0.5 <= 20000.){
#ifdef DEBUG_RESAMPLER
        if (Serial){
            Serial.print("initialization of resampler failed");
        }
#endif
		_attenuation=0.;
		_halfFilterLength=0;
        _initialized=false;
        return;
    }
	_attenuation=(double)_targetAttenuation;
    _step=fs/newFs;
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
        cutOffFrequ=1.;
        kaiserBeta=_settings.kaiserBetaDefault;//20
        _attenuation=kaiserBeta/0.1102+8.7;
        if (fs*0.5 > 20000){
            //no aliasing in the audible frequency range
            _halfFilterLength=_minHalfFilterLength;
        }
        else {
            //there will be aliasing in the audible frequency range, but we try to minimize it and choose the filter as long as possible
            _halfFilterLength=_maxHalfFilterLength;    
        }
    }
    else{
        cutOffFrequ=newFs/fs;
        double b=(2.*(0.5*newFs-20000.)/fs);   //this transition band width causes aliasing. However the generated frequencies are above 20kHz
#ifdef DEBUG_RESAMPLER
        if (Serial){
            Serial.print("b: ");
            Serial.println(b);
        }
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
        if (newFs*0.5 - 20000 > (fs -newFs)*0.5 && kaiserBeta < _settings.kaiserBetaDefault){
            //newFs is smaller than fs, but no aliasing below 20kHz
            double l=((fs -newFs)*0.5)/(newFs*0.5 - 20000.);  //we know that numerator and denominator are larger than zero
            kaiserBeta=l*kaiserBeta +(1.-l)*_settings.kaiserBetaDefault;
            _attenuation=kaiserBeta/0.1102+8.7;  
        }
        int32_t noSamples=_halfFilterLength*_overSamplingFactor+1;
        if (noSamples > MAX_FILTER_SAMPLES){
            int32_t f = (noSamples-1)/(MAX_FILTER_SAMPLES-1)+1;
            _overSamplingFactor/=f;
        }
    }

#ifdef DEBUG_RESAMPLER    
    if (Serial){
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
    }
#endif
    // uint32_t beforKaiser=ARM_DWT_CYCCNT;
    setFilter(_halfFilterLength, _overSamplingFactor, cutOffFrequ, kaiserBeta);
	// uint32_t afterKaiser=ARM_DWT_CYCCNT; 
    // if(Serial){
    //    Serial.print("time for filter: ");
    //    double d=(afterKaiser-beforKaiser)/(double)F_CPU_ACTUAL;
    //    Serial.println(d*1e6);
    // }
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
#ifdef OLDIMPLEMENTATION
void Resampler::resample(float* input0, float* input1, int32_t inputLength, int32_t& processedLength, float* output0, float* output1,int32_t outputLength, int32_t& outputCount) {
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
            ip0=_endOfBuffer[0]+indexData; 
            ip1=_endOfBuffer[1]+indexData; 
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
        processedLength=min(inputLength, (int32_t)floor(_cPos + _halfFilterLength));
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
#else
void Resampler::resample(float* input0, float* input1, int32_t inputLength, int32_t& processedLength, float* output0, float* output1, int32_t outputLength, int32_t& outputCount) {
    outputCount=0;
    int32_t successorIndex=(int32_t)(ceil(_cPos));  //negative number -> currently the _buffer0 of the last iteration is used
   
    const int32_t halfFLengthUpsampledM1 = (_halfFilterLength-1)*_overSamplingFactor;
    const int32_t overSamplingFactorM1=_overSamplingFactor-1;
    const float *ipR0, *ipR1, *ipL0, *ipL1;

    float res0, res1;
    const uint16_t hfl = _halfFilterLength;
    
    while (floor(_cPos + hfl) < inputLength && outputCount < outputLength){        
       
        // construction of the filter
        float dist=(float)(successorIndex-_cPos);
        int32_t rightWingIndex = successorIndex+hfl-1;
        int32_t leftWingIndex = successorIndex-hfl;

        if (dist == 0.f){     
            ipR0=input0+rightWingIndex+1;               
            ipR1=input1+rightWingIndex+1;       
        }
        else { 
            ipR0=input0+rightWingIndex;               
            ipR1=input1+rightWingIndex; 
        }  
        res0=0.f;
        res1=0.f;
        
        if (_cPos < 0.){
            //leftWingIndex + _halfFilterLength must still be negative -> the left wing sum only uses data from the buffer
            //rightWingIndex is always greater or equal to zero -> after rightWingIndex iterations, the right wing reaches the first sample of the input data             
            ipL0=_endOfBuffer[0]+leftWingIndex; 
            ipL1=_endOfBuffer[1]+leftWingIndex; 
            
            leftWingIndex=1; //important: prevents entering the if-statement below
        }
        else if (leftWingIndex < 0){
            //rightWingIndex is always greater or equal to zero -> after rightWingIndex iterations, the right wing reaches the first sample of the input data 
            ipL0=_endOfBuffer[0]+leftWingIndex; 
            ipL1=_endOfBuffer[1]+leftWingIndex; 
            rightWingIndex = -2;//important: prevents entering the if-statement below
        }
        else {
            //we left the buffer region completely
            ipL0=input0+leftWingIndex; 
            ipL1=input1+leftWingIndex; 
            rightWingIndex = -2;//important: prevents entering the if-statement below
            leftWingIndex=1; //important: prevents entering the if-statement below
        }

        if (dist == 0.f){
            if (_cPos >= 0. || rightWingIndex != hfl-1){
                rightWingIndex++;
            }
            const float* fPtr = filter + hfl*_overSamplingFactor;
            for (uint16_t i =0; i< hfl; i++){
                res0 += (*ipL0++ + *ipR0--) * *fPtr;
                res1 += (*ipL1++ + *ipR1--) * *fPtr;
                
                fPtr-=_overSamplingFactor;  
                if (i == rightWingIndex){
                    //from now on both wings use data from the buffer
                    ipR0=_endOfBuffer[0]-1;               
                    ipR1=_endOfBuffer[1]-1;
                }
                else if (i==(-leftWingIndex-1)){
                    //from now on both wings use data from the buffer
                    ipL0=input0; 
                    ipL1=input1; 
                }
                
            }
            res0 += *filter * *ipR0;
            res1 += *filter * *ipR1;     
            *output0++ =res0;
            *output1++ =res1;       
        }
        else{ 
            float distScaled=dist*_overSamplingFactor;
            int32_t successorIndexFilterR = (int32_t)ceilf(distScaled);
            int32_t successorIndexFilterL = -(successorIndexFilterR - _overSamplingFactor -1);              
            float w0 = (float)successorIndexFilterR - distScaled;
            float w1 = 1.f-w0;
            
            const float* fPtrRight = filter + successorIndexFilterR+halfFLengthUpsampledM1;
            const float* fPtrLeft = filter + successorIndexFilterL+halfFLengthUpsampledM1;
        
            for (uint16_t i =0; i< hfl; i++){
                float rightWingCoeff= w1 * *fPtrRight--;
                rightWingCoeff += w0 * *fPtrRight;
                float leftWingCoeff= w0 * *fPtrLeft--;
                leftWingCoeff += w1 * *fPtrLeft;

                res0 += *ipL0++ * leftWingCoeff;
                res0 += *ipR0-- * rightWingCoeff;
                res1 += *ipL1++ * leftWingCoeff;
                res1 += *ipR1-- * rightWingCoeff;
                
                fPtrRight-=overSamplingFactorM1;
                fPtrLeft-=overSamplingFactorM1;
                if (i == rightWingIndex){
                    //from now on both wings use data from the buffer
                    ipR0=_endOfBuffer[0]-1;               
                    ipR1=_endOfBuffer[1]-1;
                }
                else if (i==(-leftWingIndex-1)){
                    //from now on both wings use data from the buffer
                    ipL0=input0; 
                    ipL1=input1; 
                }
            }            
            
            *output0++ =res0;//*w0 +res01*w1;
            *output1++ =res1;//*w0 +res11*w1;
        }
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
        processedLength=min(inputLength, (int32_t)floor(_cPos + hfl));
    }
    //fill _buffer
    const int32_t indexData=processedLength-_filterLength;
    if (indexData>=0){
        float* ip0=input0+indexData;
        float* ip1=input1+indexData;
        const unsigned long long bytesToCopy= _filterLength*sizeof(float);
        memcpy((void *)_buffer[0], (void *)ip0, bytesToCopy);
        memcpy((void *)_buffer[1], (void *)ip1, bytesToCopy);    
    }  
    else {
        float* b0=_buffer[0];
        float* b1=_buffer[1];
        float* ip0=_endOfBuffer[0]+indexData; 
        float* ip1=_endOfBuffer[1]+indexData;
        for (int32_t i =0; i< _filterLength; i++){
            if(ip0==_endOfBuffer[0]){
                ip0=input0;
                ip1=input1;
            }        
            *b0++ = *ip0++;
            *b1++ = *ip1++;
        }  
    }
    _cPos-=processedLength;
    if (_cPos < -hfl){
        _cPos=-hfl;
    }
}
#endif

void Resampler::fixIncrement(){
    if (!_initialized){
        return;
    }
    _step=_stepAdapted;
    _sum=0.;
    _oldDiffs[0]=0.;
    _oldDiffs[1]=0.;
}
void Resampler::setPos(double val){
    if(val < 0.){
        val=0.;
    }
    else if (val >=1.){     
        val=0.999;
    }   
     _cPos=val-(double)_halfFilterLength;
}

bool Resampler::updateIncrement(double diff){
   
    _sum+=diff;
    double correction=_settings.kp*diff+_settings.ki*_sum;
     if (abs(diff) > 2.*_settings.kpIncreaseThrs){
        correction+= 2.*_settings.kp*(diff-_settings.kpIncreaseThrs);
    }
    else if (abs(diff) > _settings.kpIncreaseThrs){
        correction+= _settings.kp*(diff-_settings.kpIncreaseThrs);
    }
    _stepAdapted=_step+correction;
   
    if (abs(_stepAdapted/_configuredStep-1.) > _settings.maxAdaption){
        _initialized=false;
        return false;
    }
   
    bool settled=false;
    _oldDiffs[0]=_oldDiffs[1];
    _oldDiffs[1]=(1.-_settings.alpha)*_oldDiffs[1]+_settings.alpha*diff;
    const double slope=_oldDiffs[1]-_oldDiffs[0];
    if ((abs(slope/(double)_settings.periodeLength) < _settledThrs*abs(diff) &&
        abs(diff) > 2.*1e-6)) { 
        settled=true;
    }
   
    return settled;
}

double Resampler::getXPos() const{
    //0. <= _cPos < 1.  unit: input sample
    return _cPos+(double)_halfFilterLength;
}
