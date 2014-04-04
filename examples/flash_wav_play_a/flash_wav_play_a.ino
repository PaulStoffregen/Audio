/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Pete (El Supremo), el_supremo@shaw.ca
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
 
// Read/Write W25Q128FV SPI flash (128Mbit = 16MBytes)
// Pete (El Supremo) 140403


/*
  Read a WAV file from flash and play it.
  At the moment you need to know beforehand how many pages
  are in the WAV file. This number was printed by flash_wav_write
  See myWAV.play in setup() below.
  
  The flash chip does not have to be soldered on the audio board.
  You can just breadboard it as long as you wire it up properly
Flash  Teensy
  1      6    Chip Select
  2     12    MISO
  3     3V3   Write Protect wired to 3V3
  4     Gnd   Gnd
  5     7     MOSI
  6     14    CLK
  7     3V3   Hold/Reset wired to 3V3
  8     3v3   Vcc
*/


#include <Audio.h>
#include <Wire.h>
//#include <WM8731.h>
#include <SD.h>
#include <SPI.h>
//#include <flash_spi.h>

int volume = 50;
AudioPlayFlashWAV       myWAV;
AudioOutputI2S      audioOutput;        // audio shield: headphones & line-out

AudioConnection c1(myWAV, 0, audioOutput, 0);
AudioConnection c2(myWAV, 1, audioOutput, 1);

AudioControlSGTL5000 audioShield;

unsigned char in_page[256];

void setup() {
  unsigned char id_tab[32];
  
  Serial.begin(9600);
  while(!Serial);
  delay(2000);

  // Assign MOSI and SCK for the audio board
  SPI.setMOSI(7);
  SPI.setSCK(14);

  // Initialize the flash chip on the Audio board
  flash_init();

  // fast read works with 4 buffers
  // but while playing, this code MUST enter the loop()
  AudioMemory(4);

  audioShield.enable();
  audioShield.volume(volume);
  // I want output on the line out too
  audioShield.unmuteLineout();
  //  audioShield.muteHeadphone();
  

// ID the flash chip
  Serial.print("Status = ");
  Serial.println(flash_read_status(),HEX);
  // flash read doesn't work unless preceded by flash_read_status ??
  // Page 24 - a W25Q128FV should return EF, 40, 18, 0,
  flash_read_id(id_tab);
  // Just print the first four bytes
  // For now ignore the 16 remaining bytes
  for(int i = 0;i < 4;i++) {
    Serial.print(id_tab[i],HEX);
    Serial.print(", ");
  }
  Serial.println("");
  Serial.println("");

  // bach_441.wav is 39418 pages long
  // Change this to the number reported by flash_wav_write when you
  // wrote the WAV file to the chip.
  // This specifies the starting page number (usually zero)
  // and the number of pages
  myWAV.play(0,39418);

}

void loop() {
  if(!myWAV.isPlaying()) {
    Serial.print("Done");
    while(1);
  }
  // Volume control
  int n = analogRead(15);
  if (n != volume) {
    volume = n;
    audioShield.volume((float)n / 10.23);
  }
}





