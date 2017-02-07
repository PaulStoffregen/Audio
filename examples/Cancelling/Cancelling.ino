/* Audio cancelling using a regular mixer object, just by selecting negative gain.

It is worth noting that this can be done in the analog domain reasonably easy.

Send numbers 0 to 9 for 0.0 to 0.9, A for 1.0, and if listening out loud send a 'B', via serial, observe
difference(s) on DAC out pin and consider application of gain in mix1.gain(..) calls below.

*/

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

AudioSynthWaveformSine sine1500;
AudioSynthWaveformSine sine3000;

AudioMixer4 mix1;

AudioOutputAnalog dac1;

AudioConnection Patch1(sine1500,0,mix1,0);
AudioConnection Patch2(sine3000,0,mix1,1);
AudioConnection Patch3(sine1500,0,mix1,2);
AudioConnection Patch4(sine3000,0,mix1,3);

AudioConnection Patch5(mix1,dac1);

void setup()
{
	AudioMemory(6);
	Serial.begin(9600);

	sine1500.frequency(1500);
	sine3000.frequency(3000);
	sine1500.amplitude(0.45);
	sine3000.amplitude(0.45);

	mix1.gain(0,1);
	mix1.gain(1,1);
}


float mGain=0.5, oGain=0;

void loop()
{
	if(Serial.available())
	{
		uint8_t n=Serial.read();
		if(n>47&&n<58) // numbers 0 to 9
		{
			mGain=(float)(n-48)/10;
			Serial.print("Gain set to: ");
			Serial.println(mGain);
		} else {
			switch(n|32) {
			case 'a':
				mGain=1;
				Serial.println("Gain Set to: 1");
			break;
			case 'b':
				sine1500.frequency(150);
				sine3000.frequency(300);
				Serial.println("Output friendlier to ears now :)");
			}
		}
	}
	if(mGain!=oGain)
	{
		mix1.gain(2,-(1-mGain));
		mix1.gain(3,-mGain);
		oGain=mGain;
	}
}
