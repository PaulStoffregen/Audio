/* Waterfall Audio Spectrum Analyzer, adapted from Nathaniel Quillin's
   award winning (most over the top) Hackaday SuperCon 2015 Badge Hack.

   https://hackaday.io/project/8575-audio-spectrum-analyzer-a-supercon-badge
   https://github.com/nqbit/superconbadge

   ILI9341 Color TFT Display is used to display spectral data.
   Two pots on analog A2 and A3 are required to adjust sensitivity.

   Copyright (c) 2015 Nathaniel Quillin

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files
   (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge,
   publish, distribute, sublicense, and/or sell copies of the Software,
   and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <Audio.h>
#include <ILI9341_t3.h>
#include <SD.h>
#include <SerialFlash.h>
#include <SPI.h>
#include <Wire.h>

// ILI9341 Color TFT Display connections
#define TFT_DC      20
#define TFT_CS      21
#define TFT_RST    255  // 255 = unused, connect to 3.3V
#define TFT_MOSI     7
#define TFT_SCLK    14
#define TFT_MISO    12

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);

AudioInputI2S            i2s1;
AudioOutputI2S           i2s2;
AudioAnalyzeFFT1024      fft1024_1;
AudioConnection          patchCord5(i2s1, 0, i2s2, 0);
AudioConnection          patchCord6(i2s1, 0, i2s2, 1);
AudioConnection          patchCord7(i2s1, fft1024_1);
AudioControlSGTL5000     sgtl5000_1;

static int count = 0;
static uint16_t line_buffer[320];
static float scale = 10.0;
static int knob = 0;
static int vol = 0;

void setup(void) {
  // Initialize the peripherals.
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(2);

  AudioMemory(20);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);
  sgtl5000_1.inputSelect(AUDIO_INPUT_MIC);
  sgtl5000_1.micGain(36);

  // Uncomment one these to try other window functions
  // fft1024_1.windowFunction(NULL);
  // fft1024_1.windowFunction(AudioWindowBartlett1024);
  // fft1024_1.windowFunction(AudioWindowFlattop1024);

  tft.println("Waterfall Spectrum");
  tft.println("adapted from");
  tft.println("Nathaniel Quillin");
  tft.println("SuperCon Badge Hack");
  for (int i = 0; i < 100; i++) {
    tft.setScroll(count++);
    count = count % 320;
    delay(12);
  }
  tft.setRotation(0);
}

void loop() {
  knob = analogRead(A2);
  vol = analogRead(A3);

  scale = 1024 - knob;
  sgtl5000_1.micGain((1024 - vol) / 4);

  if (fft1024_1.available()) {
    for (int i = 0; i < 240; i++) {
      line_buffer[240 - i - 1] = colorMap(fft1024_1.output[i]);
    }
    tft.writeRect(0, count, 240, 1, (uint16_t*) &line_buffer);
    tft.setScroll(count++);
    count = count % 320;
  }
}

uint16_t colorMap(uint16_t val) {
  float red;
  float green;
  float blue;
  float temp = val / 65536.0 * scale;

  if (temp < 0.5) {
    red = 0.0;
    green = temp * 2;
    blue = 2 * (0.5 - temp);
  } else {
    red = temp;
    green = (1.0 - temp);
    blue = 0.0;
  }
  return tft.color565(red * 256, green * 256, blue * 256);
}


