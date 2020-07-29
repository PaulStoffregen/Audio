/*
	control_tlv320aic3206

	Created: Brendan Flynn (http://www.flexvoltbiosensor.com/) for Tympan, Jan-Feb 2017
	Modified: Chip Audette (https://openaudio.blogspot.com) for Tympan 2017-2018
	Purpose: Control module for Texas Instruments TLV320AIC3206 compatible with Teensy Audio Library

	License: MIT License.  Use at your own risk.

	Note that the TLV320AIC3206 has a reset that should be connected to microcontroller.
		This code defaults to Teensy Pin 21, which was used on Tympan Rev C.
		You can set the reset pin in the constructor
		
	Note that this can configure the AIC for 32-bit per sample I2S transfers or 16-bit per sample.
		Defaults to 16 bits per sample to allow it to work with default Teensy audio library.
	
	Example Usage for Teensy:
	
		#include <Audio.h>	//Teensy Audio Library
		int AIC3206_RESET_PIN=21;     //What pin is your AIC3206 reset pin connected to?
		AudioControlTLV320AIC3206     audioHardware(AIC3206_RESET_PIN);
		AudioInputI2S                 i2s_in;        //Digital audio *from* the AIC.
		AudioOutputI2S                i2s_out;       //Digital audio *to* the AIC.
		
		AudioConnection               patchCord1(i2s_in, 0, i2s_out, 0); //connect left input to left output
		AudioConnection               patchCord2(i2s_in, 1, i2s_out, 1); //connect right input to right output
		
		void setup(void)
		{
		  //begin the serial comms (for debugging)
		  Serial.begin(115200);  delay(500);
		  Serial.println("AudioPassThru: Starting setup()...");

		  //allocate the dynamic memory for audio processing blocks
		  AudioMemory(10); 

		  //Enable the TLV32AIC3206 to start the audio flowing!
		  audioHardware.enable(); // activate AIC

		  //Choose the desired input
		  audioHardware.inputSelect(AIC3206_INPUT_IN1);       // use Input 1

		  //Set the desired volume levels
		  audioHardware.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
		  audioHardware.setInputGain_dB(10.0); // set input volume, 0-47.5dB in 0.5dB setps
		}

		void loop(void)
		{
		  // Nothing to do
		}
 */

#ifndef control_tlv320aic3206_h_
#define control_tlv320aic3206_h_

#include "AudioControl.h"
#include <Arduino.h>

//convenience names to use with inputSelect() to set whnch analog inputs to use
#define AIC3206_INPUT_IN1            1   //uses IN1
#define AIC3206_INPUT_IN2      		 2   //uses IN2 analog inputs
#define AIC3206_INPUT_IN3    		 3   //uses IN3 analog inputs
#define AIC3206_INPUT_IN3_MICBIAS	 4   //uses IN3 analog inputs *and* enables mic bias

//convenience names to use with outputSelect()
#define AIC3206_OUTPUT_HEADPHONE_JACK_OUT 1
#define AIC3206_OUTPUT_LINE_OUT 2
#define AIC3206_OUTPUT_HEADPHONE_AND_LINE_OUT 3

//names to use with setMicBias() to set the amount of bias voltage to use
#define AIC3206_MIC_BIAS_OFF             0
#define AIC3206_MIC_BIAS_1_25            1
#define AIC3206_MIC_BIAS_1_7             2
#define AIC3206_MIC_BIAS_2_5             3
#define AIC3206_MIC_BIAS_VSUPPLY         4
#define AIC3206_DEFAULT_MIC_BIAS AIC3206_MIC_BIAS_2_5

#define AIC3206_BOTH_CHAN 0
#define AIC3206_LEFT_CHAN 1
#define AIC3206_RIGHT_CHAN 2

class AudioControlTLV320AIC3206: public AudioControl
{
	public:
		//GUI: inputs:0, outputs:0  //this line used for automatic generation of GUI node
		AudioControlTLV320AIC3206(void) { debugToSerial = false; };
		AudioControlTLV320AIC3206(bool _debugToSerial) { debugToSerial = _debugToSerial; };
		AudioControlTLV320AIC3206(int _resetPin) { debugToSerial = false; resetPinAIC = _resetPin; }
		AudioControlTLV320AIC3206(int _resetPin, bool _debugToSerial) {  resetPinAIC = _resetPin; debugToSerial = _debugToSerial; };
		bool enable(void);
		bool disable(void);
		bool outputSelect(int n);  //use AIC3206_OUTPUT_HEADPHONE_JACK_OUT or one of other choices defined earlier
		bool volume(float n);
		bool volume_dB(float n);
		bool inputLevel(float n);  //dummy to be compatible with Teensy Audio Library
		bool inputSelect(int n);   //use AIC3206_INPUT_IN1 or one of other choices defined earlier
		bool setInputGain_dB(float n);
		bool setMicBias(int n);  //use AIC3206_MIC_BIAS_OFF or AIC3206_MIC_BIAS_2_5 or one of other choices defined earlier
		bool updateInputBasedOnMicDetect(int setting = AIC3206_INPUT_IN1); //which input to monitor
		bool enableMicDetect(bool);
		int  readMicDetect(void);
		bool debugToSerial;
		unsigned int aic_readPage(uint8_t page, uint8_t reg);
		bool aic_writePage(uint8_t page, uint8_t reg, uint8_t val);
		void setHPFonADC(bool enable, float cutoff_Hz, float fs_Hz);
		float getHPCutoff_Hz(void) { return HP_cutoff_Hz; }
		float getSampleRate_Hz(void) { return sample_rate_Hz; }
		void setIIRCoeffOnADC(int chan, uint32_t *coeff);  //for chan, use AIC3206_BOTH_CHAN or AIC3206_LEFT_CHAN or AIC3206_RIGHT_CHAN
		bool enableAutoMuteDAC(bool, uint8_t);
	private:
	  void aic_reset(void);
	  void aic_init(void);
	  void aic_initDAC(void);
	  void aic_initADC(void);

	  bool aic_writeAddress(uint16_t address, uint8_t val);
	  bool aic_goToPage(uint8_t page);
	  int prevMicDetVal = -1;
	  int resetPinAIC = 21;  //AIC reset pin, Tympan Rev C
	  float HP_cutoff_Hz = 0.0f;
	  float sample_rate_Hz = 44100; //only used with HP_cutoff_Hz to design HP filter on ADC, if used
	  void setIIRCoeffOnADC_Left(uint32_t *coeff);
	  void setIIRCoeffOnADC_Right(uint32_t *coeff);
};


#endif
