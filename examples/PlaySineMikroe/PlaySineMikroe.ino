#include <Audio.h>
#include <Wire.h>
#include <SD.h>

AudioSynthWaveform mysine(AudioWaveformSine);
AudioOutputI2Sslave dac;

AudioControlWM8731master codec;

AudioConnection c1(mysine,dac);

int volume = 0;

void setup() {
  codec.enable();
  delay(100);
  while (!Serial) ;
  Serial.println("Begin AudioTest");
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(15);

  mysine.frequency(440);
  mysine.amplitude(0.9);

  codec.volume(70);

  Serial.println("setup done");
}




void loop() {
  /*
  Serial.print("cpu: ");
  Serial.print(AudioProcessorUsage());
  Serial.print(", max: ");
  Serial.print(AudioProcessorUsageMax());
  Serial.print(", memory: ");
  Serial.print(AudioMemoryUsage());
  Serial.print(", max: ");
  Serial.print(AudioMemoryUsageMax());
  
  Serial.println("");
  */
  
  //int n;
  //n = analogRead(15);
  //Serial.println(n);
  //if (n != volume) {
    //volume = n;
    //codec.volume((float)n / 10.23);
  //}
  //n = analogRead(16) / 8;
  //Serial.println(n);
  //mysine.frequency(200 + n * 4);
  
  //delay(5);
}
