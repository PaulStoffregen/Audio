
#include <Audio.h>

AudioOutputSPDIF3   spdifOut;
AsyncAudioInputSPDIF3     spdifIn(true, true, 100, 20);	//dither = true, noiseshaping = true, anti-aliasing attenuation=100dB, minimum resampling filter length=20
//

AudioConnection          patchCord1(spdifIn, 0, spdifOut, 0);
AudioConnection          patchCord2(spdifIn, 1, spdifOut, 1);

void setup() {

  // put your setup code here, to run once:
  AudioMemory(12);
  while (!Serial);

}

void loop() {
	double bufferedTine=spdifIn.getBufferedTime();
	//double targetLatency = spdifIn.getTargetLantency();
	Serial.print("buffered time [micro seconds]: ");
	Serial.println(bufferedTine*1e6,2);
	// Serial.print(", target: ");
	// Serial.println(targetLatency*1e6,2);
	
	double pUsageIn=spdifIn.processorUsage(); 
	Serial.print("processor usage [%]: ");
	Serial.println(pUsageIn);

	// bool islocked=spdifIn.isLocked(); 
	// Serial.print("isLocked: ");
	// Serial.println(islocked);

	// double f=spdifIn.getInputFrequency();
	// Serial.print("frequency: ");
	// Serial.println(f);
	// Serial.print("Memory max: ");
  	// Serial.println(AudioMemoryUsageMax());
	delay(500);
}
