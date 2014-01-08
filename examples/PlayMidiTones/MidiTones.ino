#include <Audio.h>
#include <Wire.h>
//#include <WM8731.h>
#include <SD.h>
#include <SPI.h>


//AudioInputI2S adc;
//AudioInputAnalog ana(16);
AudioSynthWaveform mysine(AudioWaveformSine);
AudioSynthWaveform sine2(AudioWaveformSine);
//AudioOutputPWM myout;
//AudioPlaySDcardWAV wav;
//AudioPlaySDcardRAW wav;
//AudioMixer4 mix;
AudioOutputI2S dac;


//AudioControl_WM8731 codec;
AudioControlSGTL5000 codec;

AudioConnection c1(mysine, dac);

//AudioConnection c1(wav, dac);
//AudioConnection c2(wav, 1, dac, 1);


int volume = 0;

void setup() {
  Serial1.begin(115200);
  Serial1.println("***************");

  while (!Serial) ;
  delay(100);
  
  codec.enable();
  
  //while(1);
  
  
  delay(200);
  Serial.println("Begin AudioTest");
  delay(50);

  Serial1.print("B");

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(15);


  //mysine.connect(myout);
  //mysine.connect(dac);
  //mysine.connect(dac, 1, 0);
  mysine.frequency(440);
  
  //wav.connect(dac);
  //wav.connect(dac, 1, 0);
  
  //codec.inputLinein();
  codec.volume(40);
  
  //adc.connect(dac);
  //ana.connect(dac);
  //ana.connect(dac, 1, 0);
  
/*
  SPI.setMOSI(7);
  SPI.setSCK(14);
  if (SD.begin(10)) {
    Serial.println("SD init ok");
    //wav.play("01_16M.WAV");
  }
*/
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
