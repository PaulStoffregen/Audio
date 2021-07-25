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
#ifndef biquad_coeffs_h_
#define biquad_coeffs_h_

#include "Arduino.h"
#include <arm_math.h>

enum class BiquadType {
	LOW_PASS, HIGH_PASS, BAND_PASS, NOTCH, ALL_PASS, PEAKING, LOW_SHELF, HIGH_SHELF
};

template <typename T>
void getCoefficients(T* coeffs, BiquadType type, double dbGain, double freq, double srate, double bandwidthOrQOrS, bool isBandwidthOrS=false){
	const double A =(type == BiquadType::PEAKING || type == BiquadType::LOW_SHELF || type == BiquadType::HIGH_SHELF) ? pow(10., dbGain / 40.) : pow(10, dbGain / 20);
	
	const double omega = 2 * M_PI * freq / srate;
	const double sn = sin(omega);
	const double cs = cos(omega);
	double alpha;

	if (!isBandwidthOrS) // Q
		alpha = sn / (2 * bandwidthOrQOrS);
	else if (type == BiquadType::LOW_SHELF || type == BiquadType::HIGH_SHELF) // S
		alpha = sn / 2 * sqrt((A + 1 / A) * (1 / bandwidthOrQOrS - 1) + 2);
	else // BW
		alpha = sn * sinh(_M_LN2 / 2 * bandwidthOrQOrS * omega / sn);

	const double beta = 2 * sqrt(A) * alpha;

	double b0, b1, b2, a0Inv, a1, a2;

	switch (type)
	{
	case BiquadType::LOW_PASS:
		b0 = (1 - cs) / 2;
		b1 = 1 - cs;
		b2 = (1 - cs) / 2;
		a0Inv = 1/(1 + alpha);
		a1 = -2 * cs;
		a2 = 1 - alpha;
		break;
	case BiquadType::HIGH_PASS:
		b0 = (1 + cs) / 2;
		b1 = -(1 + cs);
		b2 = (1 + cs) / 2;
		a0Inv = 1/(1 + alpha);
		a1 = -2 * cs;
		a2 = 1 - alpha;
		break;
	case BiquadType::BAND_PASS:
		b0 = alpha;
		b1 = 0;
		b2 = -alpha;
		a0Inv = 1/(1 + alpha);
		a1 = -2 * cs;
		a2 = 1 - alpha;
		break;
	case BiquadType::NOTCH:
		b0 = 1;
		b1 = -2 * cs;
		b2 = 1;
		a0Inv = 1/(1 + alpha);
		a1 = -2 * cs;
		a2 = 1 - alpha;
		break;
	case BiquadType::ALL_PASS:
		b0 = 1 - alpha;
		b1 = -2 * cs;
		b2 = 1 + alpha;
		a0Inv = 1/(1 + alpha);
		a1 = -2 * cs;
		a2 = 1 - alpha;
		break;
	case BiquadType::PEAKING:
		b0 = 1 + (alpha * A);
		b1 = -2 * cs;
		b2 = 1 - (alpha * A);
		a0Inv = 1/(1 + (alpha / A));
		a1 = -2 * cs;
		a2 = 1 - (alpha / A);
		break;
	case BiquadType::LOW_SHELF:
		b0 = A * ((A + 1) - (A - 1) * cs + beta);
		b1 = 2 * A * ((A - 1) - (A + 1) * cs);
		b2 = A * ((A + 1) - (A - 1) * cs - beta);
		a0Inv = (A + 1) + (A - 1) * cs + beta;
		a1 = -2 * ((A - 1) + (A + 1) * cs);
		a2 = (A + 1) + (A - 1) * cs - beta;
		break;
	case BiquadType::HIGH_SHELF:
		b0 = A * ((A + 1) + (A - 1) * cs + beta);
		b1 = -2 * A * ((A - 1) + (A + 1) * cs);
		b2 = A * ((A + 1) + (A - 1) * cs - beta);
		a0Inv = 1/((A + 1) - (A - 1) * cs + beta);
		a1 = 2 * ((A - 1) - (A + 1) * cs);
		a2 = (A + 1) - (A - 1) * cs - beta;
		break;
	}

	*coeffs++=(T)(b0 * a0Inv);
	*coeffs++=(T)(b1 * a0Inv);
	*coeffs++=(T)(b2 * a0Inv);
	*coeffs++=(T)(-a1 * a0Inv);
	*coeffs=(T)(-a2 * a0Inv);
}

	template <typename T, typename BIQUAD, typename BTYPE>
	void biquad_cascade_df2T(const BIQUAD* S, T* pSrc, T* pDst, uint32_t blockSize) {
        BTYPE* b0 =S->pCoeffs;
        BTYPE* b1=S->pCoeffs+1;
        BTYPE* b2=S->pCoeffs+2;
        BTYPE* a1Neg=S->pCoeffs+3;
        BTYPE* a2Neg=S->pCoeffs+4;

        BTYPE* state=S->pState;
        if(S->numStages==1){
            BTYPE yn;
            for (uint32_t j=0; j<blockSize; j++ ){
                yn = *b0 * *pSrc + *state;
                *state = *b1 * *pSrc + *a1Neg * yn + *(state+1);
                *(state+1) = *b2 * *pSrc++ + *a2Neg * yn;
                *pDst++=(T)yn;
            }
        }
        else {

            BTYPE pDstD[blockSize];
            BTYPE* pDstDP=pDstD;
            for (uint32_t j=0; j<blockSize; j++ ){
                *pDstDP = *b0 * *pSrc + *state;
                *state = *b1 * *pSrc + *a1Neg * *pDstDP + *(state+1);
                *(state+1) = *b2 * *pSrc++ + *a2Neg * *pDstDP++;
            }
            b0+=5;
            b1+=5;
            b2+=5;
            a1Neg+=5;
            a2Neg+=5;
            state+=2;
            for (uint8_t i =0; i< S->numStages - 2;i++){
                pDstDP=pDstD;
                BTYPE xn;
                for (uint32_t j=0; j<blockSize; j++ ){
                    xn=*pDstDP;
                    *pDstDP = *b0 * xn + *state;
                    *state = *b1 * xn + *a1Neg * *pDstDP + *(state+1);
                    *(state+1) = *b2 * xn + *a2Neg * *pDstDP++;
                }
                b0+=5;
                b1+=5;
                b2+=5;
                a1Neg+=5;
                a2Neg+=5;
                state+=2;
            }
            BTYPE yn;
            pDstDP=pDstD;
            for (uint32_t j=0; j<blockSize; j++ ){
                yn = *b0 * *pDstDP + *state;
                *state = *b1 * *pDstDP + *a1Neg * yn + *(state+1);
                *(state+1) = *b2 * *pDstDP++ + *a2Neg * yn;
                *pDst++=(T)yn;
            }
        }
    }	

	template <typename B>
	void preload(const B*  S, double val=0.){
        // double* b1=B->pCoeffs+1;
        // double* b2=B->pCoeffs+2;
        // double* a1Neg=B->pCoeffs+3;
        // double* a2Neg=B->pCoeffs+4;
        *(S->pState+1) = (*(S->pCoeffs+2) + *(S->pCoeffs+4)) * val;
		*(S->pState) = (*(S->pCoeffs+1)  + *(S->pCoeffs+3)) * val + *(S->pState+1);
	}

#endif