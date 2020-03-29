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
#if defined(__IMXRT1052__) || defined(__IMXRT1062__)

#include "async_input_spdif3.h"
#include "biquad.h"
#include <utility/imxrt_hw.h>
//Parameters
namespace {
	#define SPDIF_RX_BUFFER_LENGTH AUDIO_BLOCK_SAMPLES
	const int32_t bufferLength=8*AUDIO_BLOCK_SAMPLES;
	const uint16_t noSamplerPerIsr=SPDIF_RX_BUFFER_LENGTH/4;
}
volatile bool AsyncAudioInputSPDIF3::resetResampler=true;

#ifdef DEBUG_SPDIF_IN
volatile bool AsyncAudioInputSPDIF3::bufferOverflow=false;
#endif

volatile uint32_t AsyncAudioInputSPDIF3::microsLast;

DMAMEM __attribute__((aligned(32)))
static int32_t spdif_rx_buffer[SPDIF_RX_BUFFER_LENGTH];
static float bufferR[bufferLength];
static float bufferL[bufferLength];

volatile int32_t AsyncAudioInputSPDIF3::buffer_offset = 0;	// read by resample/ written in spdif input isr -> copied at the beginning of 'resmaple' protected by __disable_irq() in resample
int32_t AsyncAudioInputSPDIF3::resample_offset = 0; // read/written by resample/ read in spdif input isr -> no protection needed?

volatile bool AsyncAudioInputSPDIF3::lockChanged=false;
volatile bool AsyncAudioInputSPDIF3::locked=false;
DMAChannel AsyncAudioInputSPDIF3::dma(false);

AsyncAudioInputSPDIF3::~AsyncAudioInputSPDIF3(){
	delete [] _bufferLPFilter.pCoeffs;
	delete [] _bufferLPFilter.pState;
	delete quantizer[0];
	delete quantizer[1];
}

PROGMEM
AsyncAudioInputSPDIF3::AsyncAudioInputSPDIF3(bool dither, bool noiseshaping,float attenuation, int32_t minHalfFilterLength) : AudioStream(0, NULL) {
	_attenuation=attenuation;
	_minHalfFilterLength=minHalfFilterLength;
	const float factor = powf(2, 15)-1.f; // to 16 bit audio
	quantizer[0]=new Quantizer(AUDIO_SAMPLE_RATE_EXACT);
	quantizer[0]->configure(noiseshaping, dither, factor);
	quantizer[1]=new Quantizer(AUDIO_SAMPLE_RATE_EXACT);
	quantizer[1]->configure(noiseshaping, dither, factor);
	begin();
	}
PROGMEM
void AsyncAudioInputSPDIF3::begin()
{
	dma.begin(true); // Allocate the DMA channel first
	const uint32_t noByteMinorLoop=2*4;
	dma.TCD->SOFF = 4;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
	dma.TCD->NBYTES_MLNO = DMA_TCD_NBYTES_MLOFFYES_NBYTES(noByteMinorLoop) | DMA_TCD_NBYTES_SMLOE | 
						DMA_TCD_NBYTES_MLOFFYES_MLOFF(-8);
	dma.TCD->SLAST = -8;
	dma.TCD->DOFF = 4;
	dma.TCD->CITER_ELINKNO = sizeof(spdif_rx_buffer) / noByteMinorLoop;
	dma.TCD->DLASTSGA = -sizeof(spdif_rx_buffer);
	dma.TCD->BITER_ELINKNO = sizeof(spdif_rx_buffer) / noByteMinorLoop;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;	
	dma.TCD->SADDR = (void *)((uint32_t)&SPDIF_SRL);
	dma.TCD->DADDR = spdif_rx_buffer;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SPDIF_RX);

	SPDIF_SCR |=SPDIF_SCR_DMA_RX_EN;		//DMA Receive Request Enable
	dma.enable();
	dma.attachInterrupt(isr);
	config_spdifIn();
#ifdef DEBUG_SPDIF_IN
	while (!Serial);
#endif
	_bufferLPFilter.pCoeffs=new float[5];
	_bufferLPFilter.numStages=1;
	_bufferLPFilter.pState=new float[2];
	getCoefficients(_bufferLPFilter.pCoeffs, BiquadType::LOW_PASS, 0., 5., AUDIO_SAMPLE_RATE_EXACT/AUDIO_BLOCK_SAMPLES, 0.5);
}
bool AsyncAudioInputSPDIF3::isLocked() const {
	__disable_irq();
	bool l=locked;
	__enable_irq();
	return l;
}
void AsyncAudioInputSPDIF3::spdif_interrupt(){
	if(SPDIF_SIS & SPDIF_SIS_LOCK){
		if (!locked){
			locked=true;
			lockChanged=true;
		}
	}
	else if(SPDIF_SIS & SPDIF_SIS_LOCKLOSS){
		if (locked){
			locked=false;
			lockChanged=true;
			resetResampler=true;
		}
	}
	SPDIF_SIC |= SPDIF_SIC_LOCKLOSS;//clear SPDIF_SIC_LOCKLOSS interrupt
	SPDIF_SIC |= SPDIF_SIC_LOCK;	//clear SPDIF_SIC_LOCK interrupt
}

void AsyncAudioInputSPDIF3::resample(int16_t* data_left, int16_t* data_right, int32_t& block_offset){
	block_offset=0;
	if(!_resampler.initialized()){
		return;
	}
	__disable_irq();
	if(!locked){
		__enable_irq();
		return;
	}
	int32_t bOffset=buffer_offset;
	int32_t resOffset=resample_offset;
	__enable_irq();
		
	uint16_t inputBufferStop = bOffset >= resOffset ? bOffset-resOffset : bufferLength-resOffset;
	if (inputBufferStop==0){
		return;
	}
	uint16_t processedLength;
	uint16_t outputCount=0;
	uint16_t outputLength=AUDIO_BLOCK_SAMPLES;

	float resampledBufferL[AUDIO_BLOCK_SAMPLES];
	float resampledBufferR[AUDIO_BLOCK_SAMPLES];
	_resampler.resample(&bufferL[resOffset],&bufferR[resOffset], inputBufferStop, processedLength, resampledBufferL, resampledBufferR, outputLength, outputCount);
		
	resOffset=(resOffset+processedLength)%bufferLength;
	block_offset=outputCount;	

	if (bOffset > resOffset && block_offset< AUDIO_BLOCK_SAMPLES){
		inputBufferStop= bOffset-resOffset;
		outputLength=AUDIO_BLOCK_SAMPLES-block_offset;
		_resampler.resample(&bufferL[resOffset],&bufferR[resOffset], inputBufferStop, processedLength, resampledBufferL+block_offset, resampledBufferR+block_offset, outputLength, outputCount);
		resOffset=(resOffset+processedLength)%bufferLength;
		block_offset+=outputCount;
	}
	quantizer[0]->quantize(resampledBufferL, data_left, block_offset);
	quantizer[1]->quantize(resampledBufferR, data_right, block_offset);
	__disable_irq();
	resample_offset=resOffset;
	__enable_irq();		
}

void AsyncAudioInputSPDIF3::isr(void)
{
	dma.clearInterrupt();
	microsLast=micros();
	const int32_t *src, *end;
	uint32_t daddr = (uint32_t)(dma.TCD->DADDR);

	if (daddr < (uint32_t)spdif_rx_buffer + sizeof(spdif_rx_buffer) / 2) {
		// DMA is receiving to the first half of the buffer
		// need to remove data from the second half
		src = (int32_t *)&spdif_rx_buffer[SPDIF_RX_BUFFER_LENGTH/2];
		end = (int32_t *)&spdif_rx_buffer[SPDIF_RX_BUFFER_LENGTH];
		//if (AsyncAudioInputSPDIF3::update_responsibility) AudioStream::update_all();
	} else {
		// DMA is receiving to the second half of the buffer
		// need to remove data from the first half
		src = (int32_t *)&spdif_rx_buffer[0];
		end = (int32_t *)&spdif_rx_buffer[SPDIF_RX_BUFFER_LENGTH/2];
	}
	if (buffer_offset >=resample_offset ||
		(buffer_offset + SPDIF_RX_BUFFER_LENGTH/4) < resample_offset) {
		#if IMXRT_CACHE_ENABLED >=1
		arm_dcache_delete((void*)src, sizeof(spdif_rx_buffer) / 2);
		#endif
		float *destR = &(bufferR[buffer_offset]);
		float *destL = &(bufferL[buffer_offset]);
		const float factor= pow(2., 23.)+1;
		do {			
			int32_t n=(*src) & 0x800000 ? (*src)|0xFF800000  : (*src) & 0xFFFFFF;
			*destL++ = (float)(n)/factor;
			++src;

			n=(*src) & 0x800000 ? (*src)|0xFF800000  : (*src) & 0xFFFFFF;
			*destR++ = (float)(n)/factor;
			++src;
		} while (src < end);
		buffer_offset=(buffer_offset+SPDIF_RX_BUFFER_LENGTH/4)%bufferLength;
	}
#ifdef DEBUG_SPDIF_IN
	else {
		bufferOverflow=true;
	}
#endif
}
double AsyncAudioInputSPDIF3::getNewValidInputFrequ(){
	//page 2129: FrequMeas[23:0]=FreqMeas_CLK / BUS_CLK * 2^10 * GAIN
	if (SPDIF_SRPC & SPDIF_SRPC_LOCK){
		const double f=(float)F_BUS_ACTUAL/(1024.*1024.*24.*128.);// bit clock = 128 * sampling frequency
		const double freqMeas=(SPDIF_SRFM & 0xFFFFFF)*f;
		if (_lastValidInputFrequ != freqMeas){//frequency not stable yet;
			_lastValidInputFrequ=freqMeas;
			return -1.;	
		}
		return _lastValidInputFrequ;
	}
	return -1.;
}

double AsyncAudioInputSPDIF3::getBufferedTime() const{
	__disable_irq();
	double n=_bufferedTime;
	__enable_irq();
	return n;
}

void AsyncAudioInputSPDIF3::configure(){
	__disable_irq();
	if(resetResampler){
		_resampler.reset();
		resetResampler=false;
	}
	if(!locked){
		__enable_irq();
#ifdef DEBUG_SPDIF_IN
		Serial.println("lock lost");
#endif
		return;
	}	
#ifdef DEBUG_SPDIF_IN
	const bool bOverf=bufferOverflow;
	bufferOverflow=false;
#endif
	const bool lc=lockChanged;
	__enable_irq();

#ifdef DEBUG_SPDIF_IN
	if (bOverf){
		Serial.print("buffer overflow, buffer offset: ");
		Serial.print(buffer_offset);
		Serial.print(", resample_offset: ");
		Serial.println(resample_offset);
		if (!_resampler.initialized()){
			Serial.println("_resampler not initialized. ");
		}
	}
#endif
	if (lc || !_resampler.initialized()){
		const double inputF=getNewValidInputFrequ();	//returns: -1 ... invalid frequency
		if (inputF > 0.){
			__disable_irq();
			lockChanged=false;	//only reset lockChanged if a valid frequency was received (inputFrequ > 0.)
			__enable_irq();
			//we got a valid sample frequency
			const double frequDiff=inputF/_inputFrequency-1.;
			if (abs(frequDiff) > 0.01 || !_resampler.initialized()){
				//the new sample frequency differs from the last one -> configure the _resampler again
				_inputFrequency=inputF;		
				const int32_t targetLatency=round(_targetLatencyS*inputF);
				_targetLatencyS=max(0.001,(noSamplerPerIsr*3./2./_inputFrequency));
				__disable_irq();
				resample_offset =  targetLatency <= buffer_offset ? buffer_offset - targetLatency : bufferLength -(targetLatency-buffer_offset);
				__enable_irq();
				_resampler.configure(inputF, AUDIO_SAMPLE_RATE_EXACT, _attenuation, _minHalfFilterLength);
		#ifdef DEBUG_SPDIF_IN
				Serial.print("_maxLatency: ");
				Serial.println(_maxLatency);
				Serial.print("targetLatency: ");
				Serial.println(targetLatency);
				Serial.print("relative frequ diff: ");
				Serial.println(frequDiff, 8);
				Serial.print("configure _resampler with frequency ");
				Serial.println(inputF,8);			
		#endif			
			}
		}
	}
}

void AsyncAudioInputSPDIF3::monitorResampleBuffer(){
	if(!_resampler.initialized()){
		return;
	}
	__disable_irq();
	const double dmaOffset=(micros()-microsLast)*1e-6;	//[seconds]
	double bTime = resample_offset <= buffer_offset ? (buffer_offset-resample_offset-_resampler.getXPos())/_lastValidInputFrequ+dmaOffset : (bufferLength-resample_offset +buffer_offset-_resampler.getXPos())/_lastValidInputFrequ+dmaOffset;	//[seconds]
	
	double diff = bTime- (_blockDuration+ _targetLatencyS);	//seconds

	biquad_cascade_df2T<double, arm_biquad_cascade_df2T_instance_f32, float>(&_bufferLPFilter, &diff, &diff, 1);
	
	bool settled=_resampler.addToSampleDiff(diff);
	
	if (bTime > _maxLatency || bTime-dmaOffset<= _blockDuration || settled) {	
		double distance=(_blockDuration+_targetLatencyS-dmaOffset)*_lastValidInputFrequ+_resampler.getXPos();
		diff=0.;
		if (distance > bufferLength-noSamplerPerIsr){
			diff=bufferLength-noSamplerPerIsr-distance;
			distance=bufferLength-noSamplerPerIsr;
		}
		if (distance < 0.){
			distance=0.;
			diff=- (_blockDuration+ _targetLatencyS);
		}
		double resample_offsetF=buffer_offset-distance;
		resample_offset=(int32_t)floor(resample_offsetF);
		_resampler.addToPos(resample_offsetF-resample_offset);
		while (resample_offset<0){
			resample_offset+=bufferLength;
		}	
		//int32_t b_offset=buffer_offset;
#ifdef DEBUG_SPDIF_IN
		double bTimeFixed = resample_offset <= buffer_offset ? (buffer_offset-resample_offset-_resampler.getXPos())/_lastValidInputFrequ+dmaOffset : (bufferLength-resample_offset +buffer_offset-_resampler.getXPos())/_lastValidInputFrequ+dmaOffset;	//[seconds]
#endif
		__enable_irq();
#ifdef DEBUG_SPDIF_IN
		// Serial.print("settled: ");
		// Serial.println(settled);
		Serial.print("bTime: ");
		Serial.print(bTime*1e6,3);
		Serial.print("_maxLatency: ");
		Serial.println(_maxLatency*1e6,3);
		Serial.print("bTime-dmaOffset: ");
		Serial.print((bTime-dmaOffset)*1e6,3);
		Serial.print(", _blockDuration: ");
		Serial.print(_blockDuration*1e6,3);
		Serial.print("bTimeFixed: ");
		Serial.print(bTimeFixed*1e6,3);

#endif
		preload(&_bufferLPFilter, diff);
		_resampler.fixStep();		
	}
	else {		
		__enable_irq();
	}
	_bufferedTime=_targetLatencyS+diff;
}

void AsyncAudioInputSPDIF3::update(void)
{
	configure();
	monitorResampleBuffer();	//important first call 'monitorResampleBuffer' then 'resample'
	audio_block_t *block_left =allocate();
	audio_block_t *block_right =nullptr;
	if (block_left!= nullptr) {
		block_right = allocate();
		if (block_right == nullptr) {
			release(block_left);
			block_left = nullptr;
		}
	}
	if (block_left && block_right) {
		int32_t block_offset;
		resample(block_left->data, block_right->data,block_offset);
		if(block_offset < AUDIO_BLOCK_SAMPLES){
			memset(block_left->data+block_offset, 0, (AUDIO_BLOCK_SAMPLES-block_offset)*sizeof(int16_t)); 
			memset(block_right->data+block_offset, 0, (AUDIO_BLOCK_SAMPLES-block_offset)*sizeof(int16_t)); 
#ifdef DEBUG_SPDIF_IN	
			Serial.print("filled only ");
			Serial.print(block_offset);
			Serial.println(" samples.");
#endif
		}
		transmit(block_left, 0);
		release(block_left);
		block_left=nullptr;
		transmit(block_right, 1);
		release(block_right);
		block_right=nullptr;	
	}
#ifdef DEBUG_SPDIF_IN
	else {		
		Serial.println("Not enough blocks available. Too few audio memory?");
	}
#endif
}
double AsyncAudioInputSPDIF3::getInputFrequency() const{
	__disable_irq();
	double f=_lastValidInputFrequ;
	__enable_irq();
	return f;
}
double AsyncAudioInputSPDIF3::getTargetLantency() const {
	__disable_irq();
	double l=_targetLatencyS;
	__enable_irq();
	return l ;
}
void AsyncAudioInputSPDIF3::config_spdifIn(){
	//CCM Clock Gating Register 5, imxrt1060_rev1.pdf page 1145
	CCM_CCGR5 |=CCM_CCGR5_SPDIF(CCM_CCGR_ON); //turn spdif clock on - necessary for receiver!
	
	SPDIF_SCR |=SPDIF_SCR_RXFIFO_OFF_ON;	//turn receive fifo off 1->off, 0->on

	SPDIF_SCR&=~(SPDIF_SCR_RXFIFO_CTR);		//reset rx fifo control: normal opertation

	SPDIF_SCR&=~(SPDIF_SCR_RXFIFOFULL_SEL(3));	//reset rx full select
	SPDIF_SCR|=SPDIF_SCR_RXFIFOFULL_SEL(2);	//full interrupt if at least 8 sample in Rx left and right FIFOs

	SPDIF_SCR|=SPDIF_SCR_RXAUTOSYNC; //Rx FIFO auto sync on

	SPDIF_SCR&=(~SPDIF_SCR_USRC_SEL(3));	//No embedded U channel

    CORE_PIN15_CONFIG  = 3;  //pin 15 set to alt3 -> spdif input

	/// from eval board sample code
	//   IOMUXC_SetPinConfig(
	//       IOMUXC_GPIO_AD_B1_03_SPDIF_IN,        /* GPIO_AD_B1_03 PAD functional properties : */
	//       0x10B0u);                               /* Slew Rate Field: Slow Slew Rate
	//                                                  Drive Strength Field: R0/6
	//                                                  Speed Field: medium(100MHz)
	//                                                  Open Drain Enable Field: Open Drain Disabled
	//                                                  Pull / Keep Enable Field: Pull/Keeper Enabled
	//                                                  Pull / Keep Select Field: Keeper
	//                                                  Pull Up / Down Config. Field: 100K Ohm Pull Down
	//                                                  Hyst. Enable Field: Hysteresis Disabled */
	CORE_PIN15_PADCONFIG=0x10B0;
	SPDIF_SCR &=(~SPDIF_SCR_RXFIFO_OFF_ON);	//receive fifo is turned on again


	SPDIF_SRPC &= ~SPDIF_SRPC_CLKSRC_SEL(15);	//reset clock selection page 2136
	//SPDIF_SRPC |=SPDIF_SRPC_CLKSRC_SEL(6);		//if (DPLL Locked) SPDIF_RxClk else tx_clk (SPDIF0_CLK_ROOT)
	//page 2129: FrequMeas[23:0]=FreqMeas_CLK / BUS_CLK * 2^10 * GAIN
	SPDIF_SRPC &=~SPDIF_SRPC_GAINSEL(7);	//reset gain select 0 -> gain = 24*2^10
	//SPDIF_SRPC |= SPDIF_SRPC_GAINSEL(3);	//gain select: 8*2^10
	//==============================================

	//interrupts
	SPDIF_SIE |= SPDIF_SIE_LOCK;	//enable spdif receiver lock interrupt
	SPDIF_SIE |=SPDIF_SIE_LOCKLOSS;

	lockChanged=true;
	attachInterruptVector(IRQ_SPDIF, spdif_interrupt);
	NVIC_SET_PRIORITY(IRQ_SPDIF, 208); // 255 = lowest priority, 208 = priority of update
	NVIC_ENABLE_IRQ(IRQ_SPDIF);

	SPDIF_SIC |= SPDIF_SIC_LOCK;	//clear SPDIF_SIC_LOCK interrupt
	SPDIF_SIC |= SPDIF_SIC_LOCKLOSS;//clear SPDIF_SIC_LOCKLOSS interrupt
	locked=(SPDIF_SRPC & SPDIF_SRPC_LOCK);
}
#endif

