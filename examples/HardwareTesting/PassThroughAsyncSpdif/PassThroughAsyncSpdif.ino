
#include <Audio.h>

AsyncAudioInputSPDIF3     spdifIn(true, true, 100, 20);	//dither = true, noiseshaping = true, anti-aliasing attenuation=100dB, minimum resampling filter length=20
AudioOutputSPDIF3   spdifOut;

AudioConnection          patchCord1(spdifIn, 0, spdifOut, 0);
AudioConnection          patchCord2(spdifIn, 1, spdifOut, 1);

void setup() {
  AudioMemory(12);
  Serial.begin(57600);
  while (!Serial);

}

void loop() {
  double bufferedTime=spdifIn.getBufferedTime();
  Serial.print("buffered time [micro seconds]: ");
  Serial.println(bufferedTime*1e6,2);
  
  
  Serial.print("locked: ");
  Serial.println(spdifIn.isLocked());
  Serial.print("input frequency: ");
  double inputFrequency=spdifIn.getInputFrequency();
  Serial.println(inputFrequency);
  Serial.print("anti-aliasing attenuation: ");
  Serial.println(spdifIn.getAttenuation());
  
  Serial.print("resampling goup delay [milli seconds]: ");
  Serial.println(spdifIn.getHalfFilterLength()/inputFrequency*1e3,2);
 
  
  double pUsageIn=spdifIn.processorUsage(); 
  Serial.print("processor usage [%]: ");
  Serial.println(pUsageIn);

  Serial.print("max number of used blocks: ");
  Serial.println(AudioMemoryUsageMax()); 

  delay(500);
}