/* Audio Library for Teensy 3.X
 * Copyright (c) 2017, Oren Leiman
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

#include "synth_bytebeat.h"

void AudioSynthByteBeat::update(void)
{
    audio_block_t *out_block;
    out_block = allocate();
    
    if (!out_block) {
	Serial.println("failed to allocate output block");
    }

    uint16_t *data = (uint16_t*) (out_block->data);
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
	uint16_t sample;
	uint8_t pitch = pitch_;
	uint8_t p0 = p0_;
	uint8_t p1 = p1_;
	uint8_t p2 = p2_;

	phase_++;
    
	if (phase_ % bytepitch_ == 0) t_++;
    
	switch(equation_select_) {
	case 0: // hope - pitch OK
          // from http://royal-paw.com/2012/01/bytebeats-in-c-and-python-generative-symphonies-from-extremely-small-programs/
          // (atmospheric, hopeful)
	    sample = ( ( (((t_*pitch)*3) & (t_>>10)) | (((t_*pitch)*p0) & (t_>>10)) | ((t_*10) & ((t_>>8)*p1) & p2) ) & 0xFF);
	    break;

	case 1: // love - pitch OK
	    // equation by stephth via https://www.youtube.com/watch?v=tCRPUv8V22o at 3:38
	    sample = (((((t_*pitch)*p0) & (t_>>4)) | ((t_*p2) & (t_>>7)) | ((t_*p1) & (t_>>10))) & 0xFF);
	    break;
	case 2: // life - pitch OK
	    // This one is the second one listed at from http://xifeng.weebly.com/bytebeats.html
	    sample = ((( ((((((t_*pitch) >> p0) | (t_*pitch)) | ((t_*pitch) >> p0)) * p2) & ((5 * (t_*pitch)) | ((t_*pitch) >> p2)) ) | ((t_*pitch) ^ (t_ % p1)) ) & 0xFF));
	    break;
	case 3:// age - pitch disabled
	    // Arp rotator (equation 9 from Equation Composer Ptah bank)
	    sample = (((t_)>>(p2>>4))&((t_)<<3)/((t_)*p1*((t_)>>11)%(3+(((t_)>>(16-(p0>>4)))%22))));
	    break ;
	case 4: // clysm - pitch almost no effect
	    //  BitWiz Transplant via Equation Composer Ptah bank 
	    sample = ((t_*pitch)-(((t_*pitch)&p0)*p1-1668899)*(((t_*pitch)>>15)%15*(t_*pitch)))>>(((t_*pitch)>>12)%16)>>(p2%15);
	    break ;
	case 5: // monk - pitch OK
	    // Vocaliser from Equation Composer Khepri bank         
	    sample = (((t_*pitch)%p0>>2)&p1)*(t_>>(p2>>5));
	    break;
	case 6: // NERV - horrible!
	    // Chewie from Equation Composer Khepri bank         
	    sample = (p0-(((p2+1)/(t_*pitch))^p0|(t_*pitch)^922+p0))*(p2+1)/p0*(((t_*pitch)+p1)>>p1%19);
	    break;	
	case 7: // Trurl - pitch OK
	    // Tinbot from Equation Composer Sobek bank   
	    sample = ((t_*pitch)/(40+p0)*((t_*pitch)+(t_*pitch)|4-(p1+20)))+((t_*pitch)*(p2>>5));
	    break;
        case 8: // Pirx  - pitch OK
	    // My Loud Friend from Equation Composer Ptah bank   
	    sample = ((((t_*pitch)>>((p0>>12)%12))%(t_>>((p1%12)+1))-(t_>>((t_>>(p2%10))%12)))/((t_>>((p0>>2)%15))%15))<<4;
	    break;
        case 9: //Snaut
	    // GGT2 from Equation Composer Ptah bank
	    // sample = ((p0|(t_>>(t_>>13)%14))*((t_>>(p0%12))-p1&249))>>((t_>>13)%6)>>((p2>>4)%12);
	    // "A bit high-frequency, but keeper anyhow" from Equation Composer Khepri bank.
	    sample = ((t_*pitch)+last_sample_+p1/p0)%(p0|(t_*pitch)+p2);
	    break;
        case 10: // Hari
	    // The Signs, from Equation Composer Ptah bank
	    sample = ((0&(251&((t_*pitch)/(100+p0))))|((last_sample_/(t_*pitch)|((t_*pitch)/(100*(p1+1))))*((t_*pitch)|p2)));
	    break;
        case 11: // Kris - pitch OK
	    // Light Reactor from Equation Composer Ptah bank
	    sample = (((t_*pitch)>>3)*(p0-643|(325%t_|p1)&t_)-((t_>>6)*35/p2%t_))>>6;
	    break;
        case 12: // Tichy
	    sample = (t_*pitch_)>>7 & t_>>7 | t_>>8;
	    // Alpha from Equation Composer Khepri bank
	    // sample = ((((t_*pitch)^(p0>>3)-456)*(p1+1))/((((t_*pitch)>>(p2>>3))%14)+1))+((t_*pitch)*((182>>((t_*pitch)>>15)%16))&1) ;
	    break;
        case 13: // Bregg - pitch OK
	    // Hooks, from Equation Composer Khepri bank.
	    sample = ((t_*pitch)&(p0+2))-(t_/p1)/last_sample_/p2;
	    break;            
        case 14: // Avon - pitch OK
	    // Widerange from Equation Composer Khepri bank
	    sample = (((p0^((t_*pitch)>>(p1>>3)))-(t_>>(p2>>2))-t_%(t_&p1)));
	    break;        
        case 15: // Orac
	    // Abducted, from Equation Composer Ptah bank
	    sample = (p0+(t_*pitch)>>p1%12)|((last_sample_%(p0+(t_*pitch)>>p0%4))+11+p2^t_)>>(p2>>12);
	    break;
	default:
	    sample = 0;
	    break;
	}
	
	last_sample_ = sample;
	*data++ = sample << 8;
    }

    transmit(out_block);
    release(out_block);

}
