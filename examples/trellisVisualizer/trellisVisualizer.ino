// This example code is in the public domain.

#include <Audio.h>
#include <Adafruit_NeoPixel.h>
#define NEO_PIN 10

// Create the Audio components.  These should be created in the
// order data flows, inputs/sources -> processing -> outputs
//
AudioInputAnalogStereo  audioInput(PIN_MIC, 0);
AudioAnalyzeFFT1024    myFFT;
AudioOutputAnalogStereo  audioOutput;

// Connect either the live input or synthesized sine wave
AudioConnection patchCord1(audioInput, 0, myFFT, 0);

Adafruit_NeoPixel strip = Adafruit_NeoPixel(32, NEO_PIN, NEO_GRB + NEO_KHZ800);

extern const float _mel_8_256[8][256];

void setup() {
  Serial.begin(115200);

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  strip.setBrightness(255);
  
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(12);

  // Configure the window algorithm to use
  myFFT.windowFunction(AudioWindowHanning1024);
}

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  return 0;
}

float bins[8] = {0, 0, 0, 0, 0, 0, 0, 0};
float binsLast[8] = {0, 0, 0, 0, 0, 0, 0, 0};
float binsLastLast[8] = {0, 0, 0, 0, 0, 0, 0, 0};

void loop() {
  float n;
  int i;
  
  if (myFFT.available()) {
    for(i=0; i<8; i++) bins[i] = 0;

    // bin the first 256 values into 8 bins
    for(i=0; i<256; i++){
      n = myFFT.read(i);
      // dot product with a matrix of mel bands
      for(int j=0; j<8; j++){
        bins[j] += _mel_8_256[j][i] * n;
      }
    }
    
    // each time new FFT data is available
    // print it all to the Arduino Serial Monitor
    Serial.print("FFT: ");
    for (i=0; i<8; i++) {
      //moving average across the past 3 windows
      bins[i] = (binsLastLast[i] + binsLast[i] + bins[i])/3;
      n = bins[i];
      binsLastLast[i] = binsLast[i];
      binsLast[i] = bins[i];

      for(int j=3; j>=0; j--){
        int pixnum = i + 8*j;
        if(n > (0.12/4)*(4-j))
          strip.setPixelColor(pixnum, Wheel((255>>2)*(j+1)));
        else
          strip.setPixelColor(pixnum, 0);
      }
      
      if (n >= 0.01) {
        Serial.print(n);
        Serial.print(" ");
      } else {
        Serial.print("  -  "); // don't print "0.00"
      }
    }
    Serial.println();
    strip.show();
  }
  delay(10);
}


