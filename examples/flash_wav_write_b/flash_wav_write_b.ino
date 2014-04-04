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
  This copies a specified STEREO WAV file from the uSD card
  to the flash chip.
  When it is done it will print the number of pages that it
  has written to the flash chip. You will NEED this number when
  you use flash_wav_play.

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

//#include <Audio.h>
#include <Wire.h>
//#include <WM8731.h>
#include <SD.h>
#include <SPI.h>
#include <flash_spi.h>
#include "wav_file.h"

// Change this to the name of a STEREO wav file on the uSD card.
const char *in_filename = "bach_441.wav";

#define uSD_ENABLE_PIN 10
File sd_in;

struct soundhdr i_hdr;

unsigned char inbuf[512];

void setup() {
  unsigned char id_tab[32];
  unsigned long t_start;
  // length of data segment in the WAV file
  unsigned int d_length;
  unsigned int page_number;
  
  Serial.begin(9600);
  while(!Serial);
  delay(2000);

  // Assign MOSI and SCK for the audio board
  SPI.setMOSI(7);
  SPI.setSCK(14);

  flash_init();


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


  /*
    Open the sd device
   */
  if(!SD.begin(uSD_ENABLE_PIN/*, SPI_HALF_SPEED*/)) {
    Serial.println("Can't open the uSD card");
    return;
  }

  Serial.println("SD open");

  // Open the input file
  if(!(sd_in = SD.open(in_filename, FILE_READ))) {
    Serial.print("Failed to open input file ");
    Serial.println(in_filename);
    return;
  }
  Serial.print("Bulk erasing the chip ... ");
t_start = millis();
  flash_bulk_erase();  
t_start = millis() - t_start;
Serial.print(t_start);
Serial.println("ms");

  // Read the header
  sd_in.read(&i_hdr,44);
  // print the header info
  dump_seg(&i_hdr);
  
  d_length = i_hdr.dlength;
  
  Serial.println("Writing WAV file to flash");
//t_start = micros();
  page_number = 0;
  while(sd_in.read(inbuf,512) == 512) {    
    flash_page_program(inbuf,page_number++);
    flash_page_program(&inbuf[256],page_number++);
    d_length -= 512;
    if(d_length < 512)break;
  }
  sd_in.close();
//t_start = micros() - t_start;
//Serial.print("time (us) = ");
//Serial.println(t_start);
  Serial.print("Done - ");
  Serial.print(page_number);
  Serial.println(" pages written");
}

void loop() {
}





