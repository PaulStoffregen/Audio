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

#ifndef resampler_h_
#define resampler_h_


#include "Arduino.h"
//#define DEBUG_RESAMPLER  //activates debug output

#define MAX_FILTER_SAMPLES 40961 //=1024*20 +1
#define NO_EXACT_KAISER_SAMPLES 4097
#define MAX_HALF_FILTER_LENGTH 80
#define MAX_NO_CHANNELS 8

//#define OLDIMPLEMENTATION

class Resampler {
    public:

        struct StepAdaptionParameters {
            StepAdaptionParameters(){}
            double alpha =0.2;  //exponential smoothing parameter for derivative computation
            double maxAdaption = 0.01; //maximum relative allowed adaption of resampler step 0.01 = 1%
            double kp= 0.6;                     //gain of the proportional part of the PI controller
            double ki=0.00012;                  //paramter of the integral part of the PI controller 
            double kpIncreaseThrs= 10.*1e-6;    //if the difference to the target latency is larger kpIncreaseThrs, the kp parameter is increased
            double kaiserBetaDefault=18.;
            int32_t periodeLength = 128;        // number of output samples between consecutive calls of 'updateIncrement'
        };
        Resampler(float attenuation=100, int32_t minHalfFilterLength=25, int32_t maxHalfFilterLength=80, StepAdaptionParameters settings=StepAdaptionParameters());
        ~Resampler();
        void reset();
        ///@param attenuation target attenuation [dB] of the anti-aliasing filter. Only used if newFs<fs. The attenuation can't be reached if the needed filter length exceeds 2*MAX_FILTER_SAMPLES+1
        ///@param minHalfFilterLength If newFs >= fs, the filter length of the resampling filter is 2*minHalfFilterLength+1. If fs y newFs the filter is maybe longer to reach the desired attenuation
        void configure(double fs, double newFs);
        ///@param input0 first input array/ channel
        ///@param input1 second input array/ channel
        ///@param inputLength length of each input array
        ///@param processedLength number of samples of the input that were resampled to fill the output array
        ///@param output0 first output array/ channel
        ///@param output1 second output array/ channel
        ///@param outputLength length of each output array
        ///@param outputCount number of samples of each output array, that were filled with data
        void resample(float* input0, float* input1, int32_t inputLength, int32_t& processedLength, float* output0, float* output1,int32_t outputLength, int32_t& outputCount);
        
        ///@param diff difference between target buffer length and actual length in seconds
        bool updateIncrement(double diff);
        

        //0. <= _cPos < 1.  unit: input sample
        double getXPos() const;
        double getStep() const;

        //allows to initialize the resample at sub-sample accuracy. important: 0. <=val < 1.
        void setPos(double val);
        void fixIncrement();
        bool initialized() const;
		double getAttenuation() const;
		int32_t getHalfFilterLength() const;

        template <uint32_t NOCHANNELS>
        void resample(float** inputs, int32_t inputLength, int32_t& processedLength, float** outputs, int32_t outputLength, int32_t& outputCount);
        

    private:
        void getKaiserExact(float* kaiserWindowSamples, double beta);
        void setKaiserWindow(double beta, int32_t noSamples);
        void setFilter(int32_t halfFiltLength,int32_t overSampling, double cutOffFrequ, double kaiserBeta);
        
        float filter[MAX_FILTER_SAMPLES];

        float* kaiserWindowXsq;

        float _buffer[MAX_NO_CHANNELS][MAX_HALF_FILTER_LENGTH*2];
        float* _endOfBuffer[MAX_NO_CHANNELS];

		int32_t _minHalfFilterLength;
		int32_t _maxHalfFilterLength;
        int32_t _overSamplingFactor;
        int32_t _halfFilterLength;
        int32_t _filterLength;     
        bool _initialized=false;  
        
        const double _settledThrs = 1e-7;
        StepAdaptionParameters _settings;
        double _configuredStep;
        double _step;
        double _stepAdapted;
        double _cPos;
        double _sum;
        double _oldDiffs[2]; 
		
		double _attenuation=0;
		float _targetAttenuation=100;
};


//default function for multichannel resampling
//there is a specialization for NOCHANNELS=8 below, that is faster.
template <uint32_t NOCHANNELS>
inline void Resampler::resample(float** inputs, int32_t inputLength, int32_t& processedLength, float** outputs, int32_t outputLength, int32_t& outputCount){
    outputCount=0;
    int32_t successorIndex=(int32_t)(ceil(_cPos));  //negative number -> currently the _buffer0 of the last iteration is used            

    const int32_t halfFLengthUpsampledM1 = (_halfFilterLength-1)*_overSamplingFactor;
    
    float* ipR[NOCHANNELS];
    float* ipL[NOCHANNELS];
    float res[NOCHANNELS];

    const int32_t overSamplingFactorM1=_overSamplingFactor-1;
        
    while (floor(_cPos + _halfFilterLength) < inputLength && outputCount < outputLength){

        // construction of the filter
        float dist=(float)(successorIndex-_cPos);
        int32_t rightWingIndex = successorIndex+_halfFilterLength-1;
        int32_t leftWingIndex = successorIndex-_halfFilterLength;

        if (dist == 0.f){                    
            for (uint32_t i =0; i< NOCHANNELS; i++){
                ipR[i]=inputs[i]+rightWingIndex+1;
            }
        }
        else {                    
            for (uint32_t i =0; i< NOCHANNELS; i++){
                ipR[i]=inputs[i]+rightWingIndex;
            }
        }
        memset(res, 0, NOCHANNELS*sizeof(float));
                
        if (_cPos < 0.){
            //leftWingIndex + _halfFilterLength must still be negative -> the left wing sum only uses data from the buffer
            //rightWingIndex is always greater or equal to zero -> after rightWingIndex iterations, the right wing reaches the first sample of the input data 
            
            for (uint32_t i =0; i< NOCHANNELS; i++){
                ipL[i]=_buffer[i]+leftWingIndex+_filterLength; 
            }
            leftWingIndex=1; //important: prevents entering the if-statement below
        }
        else if (leftWingIndex < 0){
            //rightWingIndex is always greater or equal to zero -> after rightWingIndex iterations, the right wing reaches the first sample of the input data 
            for (uint32_t i =0; i< NOCHANNELS; i++){
                ipL[i]=_buffer[i]+leftWingIndex+_filterLength; 
            }
            rightWingIndex = -2;//important: prevents entering the if-statement below
        }
        else {
            //we left the buffer region completely
            for (uint32_t i =0; i< NOCHANNELS; i++){
                ipL[i]=inputs[i] + leftWingIndex; 
            }
            rightWingIndex = -2;//important: prevents entering the if-statement below
            leftWingIndex=1; //important: prevents entering the if-statement below
        }


        if (dist == 0.f){
            if (_cPos >= 0. || rightWingIndex != _halfFilterLength-1){
                rightWingIndex++;
            }
            const float* fPtr = filter + _halfFilterLength*_overSamplingFactor;
            for (int32_t i =0; i< _halfFilterLength; i++){
                    
                for (uint32_t j=0; j< NOCHANNELS; j++){
                    res[j]+=(*ipL[j]++ + *ipR[j]--) * *fPtr;
                }
                
                fPtr-=_overSamplingFactor;  
                if (i == rightWingIndex){
                    //from now on both wings use data from the buffer
                    for (uint32_t j =0; j< NOCHANNELS; j++){
                        ipR[j]=_endOfBuffer[j]-1;
                    }
                }
                else if (i==(-leftWingIndex-1)){
                    //from now on both wings use data from the buffer
                    for (uint32_t j =0; j< NOCHANNELS; j++){
                        ipL[j]=inputs[j]; 
                    }
                }
                
            }
            for (uint32_t j=0; j< NOCHANNELS; j++){
                res[j]+=*filter * *ipR[j];
            }
        }
        else{ 
            float distScaled=dist*_overSamplingFactor;
            int32_t successorIndexFilterR = (int32_t)ceilf(distScaled);
            int32_t successorIndexFilterL = -(successorIndexFilterR - _overSamplingFactor -1);              
            float w0 = (float)successorIndexFilterR - distScaled;
            float w1 = 1.f-w0;
            
            const float* fPtrRight = filter + successorIndexFilterR+halfFLengthUpsampledM1;
            const float* fPtrLeft = filter + successorIndexFilterL+halfFLengthUpsampledM1;

            for (int32_t i =0; i< _halfFilterLength; i++){
                float rightWingCoeff= w1 * *fPtrRight--;
                rightWingCoeff += w0 * *fPtrRight;
                float leftWingCoeff= w0 * *fPtrLeft--;
                leftWingCoeff += w1 * *fPtrLeft;
                fPtrRight-=overSamplingFactorM1;
                fPtrLeft-=overSamplingFactorM1;
                
                for (uint32_t j=0; j< NOCHANNELS; j++){
                    res[j]+=(*ipL[j]++ * leftWingCoeff)+(*ipR[j]--* rightWingCoeff);
                }
                if (i == rightWingIndex){
                    //from now on both wings use data from the buffer
                    for (uint32_t j =0; j< NOCHANNELS; j++){
                        ipR[j]=_endOfBuffer[j]-1;
                        }
                }
                else if (i==(-leftWingIndex-1)){
                    //from now on both wings use data from the buffer
                    for (uint32_t j =0; j< NOCHANNELS; j++){
                        ipL[j]=inputs[j]; 
                    }
                }
            }
        }
        for (uint32_t j=0; j< NOCHANNELS; j++){
            *outputs[j]++ =res[j];
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
        processedLength=min(inputLength, (int32_t)floor(_cPos + _halfFilterLength));
    }
    //fill _buffer
    const int32_t indexData=processedLength-_filterLength;
    if (indexData>=0){
        const unsigned long long bytesToCopy= _filterLength*sizeof(float);
        float** inPtr=inputs;
        for (uint32_t i =0; i< NOCHANNELS; i++){
            memcpy((void *)_buffer[i], (void *)((*inPtr)+indexData), bytesToCopy);
            ++inPtr;
        }   
    }  
    else {
        float** inPtr=inputs;
        for (uint32_t i =0; i< NOCHANNELS; i++){
            float* b=_buffer[i];
            float* ip=b+indexData+_filterLength; 
            for (int32_t j =0; j< _filterLength; j++){
                if(ip==_endOfBuffer[i]){
                    ip=*inPtr;
                }        
                *b++ = *ip++;
            } 
            ++inPtr;
        }
    }
    _cPos-=processedLength;
    if (_cPos < -_halfFilterLength){
        _cPos=-_halfFilterLength;
    }
}


template<>
inline void Resampler::resample<8>(float** inputs, int32_t inputLength, int32_t& processedLength, float** outputs, int32_t outputLength, int32_t& outputCount){
    outputCount=0;
    int32_t successorIndex=(int32_t)(ceil(_cPos));  //negative number -> currently the _buffer0 of the last iteration is used            

    const int32_t halfFLengthUpsampledM1 = (_halfFilterLength-1)*_overSamplingFactor;
    
    float* ipR[8];
    float* ipL[8];
    float res0,res1,res2,res3,res4,res5,res6,res7;

    const int32_t overSamplingFactorM1=_overSamplingFactor-1;
        
    while (floor(_cPos + _halfFilterLength) < inputLength && outputCount < outputLength){

        // construction of the filter
        float dist=(float)(successorIndex-_cPos);
        int32_t rightWingIndex = successorIndex+_halfFilterLength-1;
        int32_t leftWingIndex = successorIndex-_halfFilterLength;

        if (dist == 0.f){                    
            for (uint32_t i =0; i< 8; i++){
                ipR[i]=inputs[i]+rightWingIndex+1;
            }
        }
        else {                    
            for (uint32_t i =0; i< 8; i++){
                ipR[i]=inputs[i]+rightWingIndex;
            }
        }   
        res0=0.f;
        res1=0.f;
        res2=0.f;
        res3=0.f;
        res4=0.f;
        res5=0.f;
        res6=0.f;
        res7=0.f;           
        if (_cPos < 0.){
            //leftWingIndex + _halfFilterLength must still be negative -> the left wing sum only uses data from the buffer
            //rightWingIndex is always greater or equal to zero -> after rightWingIndex iterations, the right wing reaches the first sample of the input data 
            
            for (uint32_t i =0; i< 8; i++){
                ipL[i]=_endOfBuffer[i]+leftWingIndex; 
            }
            leftWingIndex=1; //important: prevents entering the if-statement below
        }
        else if (leftWingIndex < 0){
            //rightWingIndex is always greater or equal to zero -> after rightWingIndex iterations, the right wing reaches the first sample of the input data 
            for (uint32_t i =0; i< 8; i++){
                ipL[i]=_endOfBuffer[i]+leftWingIndex; 
            }
            rightWingIndex = -2;//important: prevents entering the if-statement below
        }
        else {
            //we left the buffer region completely
            for (uint32_t i =0; i< 8; i++){
                ipL[i]=inputs[i] + leftWingIndex; 
            }
            rightWingIndex = -2;//important: prevents entering the if-statement below
            leftWingIndex=1; //important: prevents entering the if-statement below
        }


        if (dist == 0.f){
            if (_cPos >= 0. || rightWingIndex != _halfFilterLength-1){
                rightWingIndex++;
            }
            const float* fPtr = filter + _halfFilterLength*_overSamplingFactor;
            for (int32_t i =0; i< _halfFilterLength; i++){
                res0+=(*ipL[0]++ + *ipR[0]--) * *fPtr;
                res1+=(*ipL[1]++ + *ipR[1]--) * *fPtr;
                res2+=(*ipL[2]++ + *ipR[2]--) * *fPtr;
                res3+=(*ipL[3]++ + *ipR[3]--) * *fPtr;
                res4+=(*ipL[4]++ + *ipR[4]--) * *fPtr;
                res5+=(*ipL[5]++ + *ipR[5]--) * *fPtr;
                res6+=(*ipL[6]++ + *ipR[6]--) * *fPtr;
                res7+=(*ipL[7]++ + *ipR[7]--) * *fPtr;
                
                fPtr-=_overSamplingFactor;  
                if (i == rightWingIndex){
                    //from now on both wings use data from the buffer
                    for (uint32_t j =0; j< 8; j++){
                        ipR[j]=_endOfBuffer[j]-1;
                    }
                }
                else if (i==(-leftWingIndex-1)){
                    //from now on both wings use data from the buffer
                    for (uint32_t j =0; j< 8; j++){
                        ipL[j]=inputs[j]; 
                    }
                }
                
            }
            res0+=*filter * *ipR[0];
            res1+=*filter * *ipR[1];
            res2+=*filter * *ipR[2];
            res3+=*filter * *ipR[3];
            res4+=*filter * *ipR[4];
            res5+=*filter * *ipR[5];
            res6+=*filter * *ipR[6];
            res7+=*filter * *ipR[7];
        }
        else{ 
            float distScaled=dist*_overSamplingFactor;
            int32_t successorIndexFilterR = (int32_t)ceilf(distScaled);
            int32_t successorIndexFilterL = -(successorIndexFilterR - _overSamplingFactor -1);              
            float w0 = (float)successorIndexFilterR - distScaled;
            float w1 = 1.f-w0;
            
            const float* fPtrRight = filter + successorIndexFilterR+halfFLengthUpsampledM1;
            const float* fPtrLeft = filter + successorIndexFilterL+halfFLengthUpsampledM1;

            for (int32_t i =0; i< _halfFilterLength; i++){
                float rightWingCoeff= w1 * *fPtrRight--;
                rightWingCoeff += w0 * *fPtrRight;
                float leftWingCoeff= w0 * *fPtrLeft--;
                leftWingCoeff += w1 * *fPtrLeft;
                fPtrRight-=overSamplingFactorM1;
                fPtrLeft-=overSamplingFactorM1;

                res0+=(*ipL[0]++ * leftWingCoeff)+(*ipR[0]--* rightWingCoeff);
                res1+=(*ipL[1]++ * leftWingCoeff)+(*ipR[1]--* rightWingCoeff);
                res2+=(*ipL[2]++ * leftWingCoeff)+(*ipR[2]--* rightWingCoeff);
                res3+=(*ipL[3]++ * leftWingCoeff)+(*ipR[3]--* rightWingCoeff);
                res4+=(*ipL[4]++ * leftWingCoeff)+(*ipR[4]--* rightWingCoeff);
                res5+=(*ipL[5]++ * leftWingCoeff)+(*ipR[5]--* rightWingCoeff);
                res6+=(*ipL[6]++ * leftWingCoeff)+(*ipR[6]--* rightWingCoeff);
                res7+=(*ipL[7]++ * leftWingCoeff)+(*ipR[7]--* rightWingCoeff);
                if (i == rightWingIndex){
                    //from now on both wings use data from the buffer
                    for (uint32_t j =0; j< 8; j++){
                        ipR[j]=_endOfBuffer[j]-1;
                    }
                }
                else if (i==(-leftWingIndex-1)){
                    //from now on both wings use data from the buffer
                    for (uint32_t j =0; j< 8; j++){
                        ipL[j]=inputs[j]; 
                    }
                }
            }
        }
        
        *outputs[0]++ =res0;
        *outputs[1]++ =res1;
        *outputs[2]++ =res2;
        *outputs[3]++ =res3;
        *outputs[4]++ =res4;
        *outputs[5]++ =res5;
        *outputs[6]++ =res6;
        *outputs[7]++ =res7;
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
        const unsigned long long bytesToCopy= _filterLength*sizeof(float);
        float** inPtr=inputs;
        for (uint32_t i =0; i< 8; i++){
            memcpy((void *)_buffer[i], (void *)((*inPtr)+indexData), bytesToCopy);
            ++inPtr;
        }   
    }  
    else {
        float** inPtr=inputs;
        for (uint32_t i =0; i< 8; i++){
            float* b=_buffer[i];
            float* ip=b+indexData+_filterLength; 
            for (int32_t j =0; j< _filterLength; j++){
                if(ip==_endOfBuffer[i]){
                    ip=*inPtr;
                }        
                *b++ = *ip++;
            } 
            ++inPtr;
        }
    }
    _cPos-=processedLength;
    if (_cPos < -_halfFilterLength){
        _cPos=-_halfFilterLength;
    }
}

//The version of 'resample' below is nearly the same version as above. It is not that much optimized and it is therefore easier to understand.
        // template <uint8_t NOCHANNELS>
        // inline void resample(float** inputs, uint16_t inputLength, uint16_t& processedLength, float** outputs, uint16_t outputLength, uint16_t& outputCount){
        //     outputCount=0;
        //     int32_t successorIndex=(int32_t)(ceil(_cPos));  //negative number -> currently the _buffer0 of the last iteration is used
        
        
        //     float leftWing[MAX_HALF_FILTER_LENGTH];
        //     float rightWing[MAX_HALF_FILTER_LENGTH];
        //     const int32_t halfFLengthUpsampledM1 = (_halfFilterLength-1)*_overSamplingFactor;
        //     const int32_t hfl=_halfFilterLength;
        //     while (floor(_cPos + hfl) < inputLength && outputCount < outputLength){
                
        //         // construction of the filter
        //         float dist=(float)(successorIndex-_cPos);
        //         int32_t leftWingIndex = successorIndex-hfl;
        //         int32_t rightWingIndex = successorIndex+hfl-1;
        //         if (dist == 0.f){
        //             rightWingIndex++;
        //             const float* fPtr = filter + hfl*_overSamplingFactor;
        //             float* rWPtr = rightWing;
        //             float* lWPtr = leftWing;
        //             for (int32_t i =0; i< hfl; i++){
        //                 *rWPtr++= *fPtr;
        //                 *lWPtr++= *fPtr;
        //                 fPtr-=_overSamplingFactor;
        //             }
        //         }
        //         else{ 
        //             float distScaled=dist*_overSamplingFactor;
        //             int32_t successorIndexFilterR = (int32_t)ceilf(distScaled);
        //             int32_t successorIndexFilterL = -(successorIndexFilterR - _overSamplingFactor -1);              
        //             float w0 = (float)successorIndexFilterR - distScaled;
        //             float w1 = 1.f-w0;
                    
        //             const float* fPtrRight = filter + successorIndexFilterR+halfFLengthUpsampledM1;
        //             const float* fPtrLeft = filter +successorIndexFilterL+halfFLengthUpsampledM1;
        //             float* rWPtr = rightWing;
        //             float* lWPtr = leftWing;
        //             for (int32_t i =0; i< hfl; i++){
        //                 *rWPtr++= w0 * *(fPtrRight-1) +w1 * *fPtrRight;
        //                 *lWPtr++= w1 * *(fPtrLeft-1) +w0 * *fPtrLeft;
        //                 fPtrRight-=_overSamplingFactor;
        //                 fPtrLeft-=_overSamplingFactor;
        //             }
        //         }
                
        //         for (int32_t j =0; j< NOCHANNELS; j++) {
        //             *(outputs[j])=0.f;
        //         } 

        //         float* lw=leftWing;
        //         float* rw=rightWing;        
        //         const float* ipR[NOCHANNELS];
        //         for (uint32_t i =0; i< NOCHANNELS; i++){
        //             ipR[i]=inputs[i]+rightWingIndex;
        //         }
        //         if (_cPos < 0.){
        //             //leftWingIndex + _halfFilterLength must still be negative -> the left wing sum only uses data from the buffer
        //             //rightWingIndex is always greater or equal to zero -> after rightWingIndex iterations, the right wing reaches the first sample of the input data 
        //             const float* ipL[NOCHANNELS];
        //             for (uint32_t i =0; i< NOCHANNELS; i++){
        //                 ipL[i]=_buffer[i]+leftWingIndex+_filterLength; 
        //             }
                    
        //             if (rightWingIndex == hfl){
        //                 // this could happen if dist == 0.f && _cPos is very, very close to zero 
        //                 rightWingIndex--;
        //             }

        //             for (int32_t i =0; i<= rightWingIndex; i++){                
        //                 for (int32_t j =0; j< NOCHANNELS; j++) {
        //                     *(outputs[j])+=(*ipL[j]++ * *lw)+(*ipR[j]--* *rw);
        //                 }  
        //                 ++lw;
        //                 ++rw;  
        //             }
                    
        //             //from now on both wings use data from the buffer
        //             for (uint32_t i =0; i< NOCHANNELS; i++){
        //                 ipR[i]=_endOfBuffer[i]-1;
        //             }

        //             for (int32_t i =1; i < hfl - rightWingIndex; i++){                
        //                 for (int32_t j =0; j< NOCHANNELS; j++) {
        //                     *(outputs[j])+=(*ipL[j]++ * *lw)+(*ipR[j]--* *rw);
        //                 }  
        //                 ++lw;
        //                 ++rw;  
        //             }           
        //         }
        //         else if (leftWingIndex < 0){

        //             //rightWingIndex is always greater or equal to zero -> after rightWingIndex iterations, the right wing reaches the first sample of the input data 
        //             const float* ipL[NOCHANNELS];
        //             for (uint32_t i =0; i< NOCHANNELS; i++){
        //                 ipL[i]=_buffer[i]+leftWingIndex+_filterLength; 
        //             }
                    
        //             for (int32_t i =0; i< -leftWingIndex; i++){                
        //                 for (int32_t j =0; j< NOCHANNELS; j++) {
        //                     *(outputs[j])+=(*ipL[j]++ * *lw)+(*ipR[j]--* *rw);
        //                 }  
        //                 ++lw;
        //                 ++rw;  
        //             }
                    
        //             //from now on both wings use data from the buffer
        //             for (uint32_t i =0; i< NOCHANNELS; i++){
        //                 ipL[i]=inputs[i]; 
        //             }
                    
        //             for (int32_t i =0; i < hfl + leftWingIndex; i++){                
        //                 for (int32_t j =0; j< NOCHANNELS; j++) {
        //                     *(outputs[j])+=(*ipL[j]++ * *lw)+(*ipR[j]--* *rw);
        //                 }  
        //                 ++lw;
        //                 ++rw;  
        //             }  

        //         }
        //         else {
        //             //we left the buffer region completely
        //             const float* ipL[NOCHANNELS];
        //             for (uint32_t i =0; i< NOCHANNELS; i++){
        //                 ipL[i]=inputs[i] + leftWingIndex; 
        //             }
                    
        //             for (int32_t i =0; i< hfl; i++){  
        //                 for (int32_t j =0; j< NOCHANNELS; j++) {
        //                     *(outputs[j])+=(*ipL[j]++ * *lw)+(*ipR[j]--* *rw);
        //                 }  
        //                 ++lw;
        //                 ++rw;           
        //             }

        //         }
        //         if (dist == 0.f){
        //             for (int32_t j =0; j< NOCHANNELS; j++) {
        //                 *(outputs[j])+= *filter * *ipR[j];
        //             }   
        //         }
        //         for (int32_t j =0; j< NOCHANNELS; j++) {
        //             ++outputs[j];
        //         }
        //         outputCount++;

        //         _cPos+=_stepAdapted;
        //         while (_cPos >successorIndex){
        //             successorIndex++;
        //         }
        //     }
        //     if(outputCount < outputLength){
        //         //ouput vector not full -> we ran out of input samples
        //         processedLength=inputLength;
        //     }
        //     else{
        //         processedLength=min(inputLength, (int16_t)floor(_cPos + _halfFilterLength));
        //     }
        //     //fill _buffer
        //     const int32_t indexData=processedLength-_filterLength;
        //     if (indexData>=0){
        //         const unsigned long long bytesToCopy= _filterLength*sizeof(float);
        //         float** inPtr=inputs;
        //         for (uint8_t i =0; i< NOCHANNELS; i++){
        //             memcpy((void *)_buffer[i], (void *)((*inPtr)+indexData), bytesToCopy);
        //             ++inPtr;
        //         }   
        //     }  
        //     else {
        //         float** inPtr=inputs;
        //         for (uint8_t i =0; i< NOCHANNELS; i++){
        //             float* b=_buffer[i];
        //             float* ip=b+indexData+_filterLength; 
        //             for (uint16_t j =0; j< _filterLength; j++){
        //                 if(ip==_endOfBuffer[i]){
        //                     ip=*inPtr;
        //                 }        
        //                 *b++ = *ip++;
        //             } 
        //             ++inPtr;
        //         }
        //     }
        //     _cPos-=processedLength;
        //     if (_cPos < -_halfFilterLength){
        //         _cPos=-_halfFilterLength;
        //     }
        // }
#endif