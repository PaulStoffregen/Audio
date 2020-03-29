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
#define NO_EXACT_KAISER_SAMPLES 1025
#define MAX_HALF_FILTER_LENGTH 80
#define MAX_NO_CHANNELS 8
class Resampler {
    public:

        struct StepAdaptionParameters {
            StepAdaptionParameters(){}
            double alpha =0.2;  //exponential smoothing parameter
            double maxAdaption = 0.01; //maximum relative allowed adaption of resampler step 0.01 = 1%
            double kp= 0.6;
            double ki=0.00012;
            double kd= 1.8;
        };
        Resampler(StepAdaptionParameters settings=StepAdaptionParameters());
        void reset();
        ///@param attenuation target attenuation [dB] of the anti-aliasing filter. Only used if newFs<fs. The attenuation can't be reached if the needed filter length exceeds 2*MAX_FILTER_SAMPLES+1
        ///@param minHalfFilterLength If newFs >= fs, the filter length of the resampling filter is 2*minHalfFilterLength+1. If fs y newFs the filter is maybe longer to reach the desired attenuation
        void configure(float fs, float newFs, float attenuation=100, int32_t minHalfFilterLength=20);
        ///@param input0 first input array/ channel
        ///@param input1 second input array/ channel
        ///@param inputLength length of each input array
        ///@param processedLength number of samples of the input that were resampled to fill the output array
        ///@param output0 first output array/ channel
        ///@param output1 second output array/ channel
        ///@param outputLength length of each output array
        ///@param outputCount number of samples of each output array, that were filled with data
        void resample(float* input0, float* input1, uint16_t inputLength, uint16_t& processedLength, float* output0, float* output1,uint16_t outputLength, uint16_t& outputCount);
        bool addToSampleDiff(double diff);
        double getXPos() const;
        double getStep() const;
        void addToPos(double val);
        void fixStep();
        bool initialized() const;
        
        //resampling NOCHANNELS channels. Performance is increased a lot if the number of channels is known at compile time -> the number of channels is a template argument
        template <uint8_t NOCHANNELS>
        inline void resample(float** inputs, uint16_t inputLength, uint16_t& processedLength, float** outputs, uint16_t outputLength, uint16_t& outputCount){
            outputCount=0;
            int32_t successorIndex=(int32_t)(ceil(_cPos));  //negative number -> currently the _buffer0 of the last iteration is used
            float* ip[NOCHANNELS];
            float* fPtr;
        
            float si0[NOCHANNELS];
            float* si0Ptr;
            float si1[NOCHANNELS];
            float* si1Ptr;
            while (floor(_cPos + _halfFilterLength) < inputLength && outputCount < outputLength){
                float dist=successorIndex-_cPos;
                    
                float distScaled=dist*_overSamplingFactor;
                int32_t rightIndex=abs((int32_t)(ceil(distScaled))-_overSamplingFactor*_halfFilterLength);   
                const int32_t indexData=successorIndex-_halfFilterLength;
                if (indexData>=0){
                    for (uint8_t i =0; i< NOCHANNELS; i++){
                        ip[i]=inputs[i]+indexData;
                    }
                }  
                else {            
                    for (uint8_t i =0; i< NOCHANNELS; i++){
                        ip[i]=_buffer[i]+indexData+_filterLength;
                    }
                }       
                fPtr=filter+rightIndex;
                memset(si0, 0, NOCHANNELS*sizeof(float));
                if (rightIndex==_overSamplingFactor*_halfFilterLength){
                    si1Ptr=si1;
                    for (uint8_t i=0; i< NOCHANNELS; i++){
                        *(si1Ptr++)=*ip[i]++**fPtr;
                    }
                    fPtr-=_overSamplingFactor;          
                    rightIndex=(int32_t)(ceil(distScaled))+_overSamplingFactor;     //needed below  
                }
                else {
                    memset(si1, 0, NOCHANNELS*sizeof(float));
                    rightIndex=(int32_t)(ceil(distScaled));     //needed below
                }
                for (uint16_t i =0 ; i<_halfFilterLength; i++){
                    if(ip[0]==_endOfBuffer[0]){
                        for (uint8_t i =0; i< NOCHANNELS; i++){
                            ip[i]=inputs[i];
                        }
                    }
                    const float fPtrSucc=*(fPtr+1);
                    si0Ptr=si0;
                    si1Ptr=si1;
                    for (uint8_t i =0; i< NOCHANNELS; i++){
                        *(si0Ptr++)+=*ip[i]*fPtrSucc; 
                        *(si1Ptr++)+=*ip[i]**fPtr; 
                        ++ip[i];
                    }       
                    fPtr-=_overSamplingFactor; 
                }
                fPtr=filter+rightIndex-1;
                for (uint16_t i =0 ; i<_halfFilterLength; i++){  
                    if(ip[0]==_endOfBuffer[0]){
                        for (uint8_t i =0; i< NOCHANNELS; i++){
                            ip[i]=inputs[i];
                        }
                    }
                    const float fPtrSucc=*(fPtr+1);
                    si0Ptr=si0;
                    si1Ptr=si1;
                    for (uint8_t i =0; i< NOCHANNELS; i++){
                        *(si0Ptr++)+=*ip[i]**fPtr; 
                        *(si1Ptr++)+=*ip[i]*fPtrSucc;  
                        ++ip[i];
                    }
                    fPtr+=_overSamplingFactor;
                }
                const float w0=ceil(distScaled)-distScaled;
                const float w1=1.-w0;
                si0Ptr=si0;
                si1Ptr=si1;
                for (uint8_t i =0; i< NOCHANNELS; i++){
                    *outputs[i]++=*(si0Ptr++)*w0 + *(si1Ptr++)*w1;
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
                processedLength=min(inputLength, (int16_t)floor(_cPos + _halfFilterLength));
            }
            //fill _buffer
            const int32_t indexData=processedLength-_filterLength;
            if (indexData>=0){
                const unsigned long long bytesToCopy= _filterLength*sizeof(float);
                float** inPtr=inputs;
                for (uint8_t i =0; i< NOCHANNELS; i++){
                    memcpy((void *)_buffer[i], (void *)((*inPtr)+indexData), bytesToCopy);
                    ++inPtr;
                }   
            }  
            else {
                float** inPtr=inputs;
                for (uint8_t i =0; i< NOCHANNELS; i++){
                    float* b=_buffer[i];
                    float* ip=b+indexData+_filterLength; 
                    for (uint16_t j =0; j< _filterLength; j++){
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
    private:
        void getKaiserExact(float beta);
        void setKaiserWindow(float beta, int32_t noSamples);
        void setFilter(int32_t halfFiltLength,int32_t overSampling, float cutOffFrequ, float kaiserBeta);
        float filter[MAX_FILTER_SAMPLES];
        double kaiserWindowSamples[NO_EXACT_KAISER_SAMPLES];
        double tempRes[NO_EXACT_KAISER_SAMPLES-1];
        double kaiserWindowXsq[NO_EXACT_KAISER_SAMPLES-1];
        float _buffer[MAX_NO_CHANNELS][MAX_HALF_FILTER_LENGTH*2];
        float* _endOfBuffer[MAX_NO_CHANNELS];

        int32_t _overSamplingFactor;
        int32_t _halfFilterLength;
        int32_t _filterLength;     
        bool _initialized=false;  
        
        const double _settledThrs = 1e-6;
        StepAdaptionParameters _settings;
        double _configuredStep;
        double _step;
        double _stepAdapted;
        double _cPos;
        double _sum;
        double _oldDiffs[2]; 
};

#endif