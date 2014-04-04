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
// Most recent version 140403 1600
/*
  This code implements the basic instructions of the
  W25Q128FV SPI flash chip which can be added to the
  Teensy 3.x Audio adapter board.
  The chip is divided into 65536 pages of 256 bytes each.
  When writing to the chip you must write a page at a time
  and that page must have previously been erased to all 1
  When erasing, pages  can be erased only in groups of
  16 (4kB sector), 128 (32kB block), 256 (64kB block) or
  the entire chip (chip erase).
  
*/
#include <SPI.h>
#include "flash_spi.h"

//=====================================
// convert a page number to a 24-bit address
int page_to_address(int pn)
{
  return(pn << 8);
}

//=====================================
// convert a 24-bit address to a page number
int address_to_page(int addr)
{
  return(addr >> 8);
}

//=====================================
void flash_read_id(unsigned char *idt)
{
  //set control register 
  digitalWrite(f_cs, LOW);
  SPI.transfer(CMD_READ_ID);
  for(int i = 0;i < 20;i++) {
    *idt++ = SPI.transfer(0x00);
  }
  digitalWrite(f_cs, HIGH);

}

//=====================================
unsigned char flash_read_status(void)
{
  unsigned char c;

  digitalWrite(f_cs, LOW);  
  SPI.transfer(CMD_READ_STATUS_REG);
  c = SPI.transfer(0x00);
  digitalWrite(f_cs, HIGH);
  return(c);
}

//=====================================
// Tbe Typ=13sec  Max=40sec
void flash_bulk_erase(void)
{
  // Send Write Enable command
  digitalWrite(f_cs, LOW);
  SPI.transfer(CMD_WRITE_ENABLE);
  digitalWrite(f_cs, HIGH);
  digitalWrite(f_cs, LOW);
  SPI.transfer(CMD_BULK_ERASE);
  digitalWrite(f_cs, HIGH);
  // Wait for the page to be written
  while(flash_read_status() & STAT_WIP);
//delay(1);
}

//=====================================
// Tse Typ=0.6sec Max=3sec
// measured 549.024ms
// Erase the sector which contains the specified
// page number.
// The smallest unit of memory which can be erased
// is the 4kB sector (which is 16 pages)
void flash_erase_pages_sector(int pn)
{
  int address;
  
  // Send Write Enable command
  digitalWrite(f_cs, LOW);
  SPI.transfer(CMD_WRITE_ENABLE);
  digitalWrite(f_cs, HIGH);
  digitalWrite(f_cs, LOW);
  SPI.transfer(CMD_SECTOR_ERASE);
  // Send the 3 byte address
  address = page_to_address(pn);
  SPI.transfer((address >> 16) & 0xff);
  SPI.transfer((address >> 8) & 0xff);
  SPI.transfer(address & 0xff);
  digitalWrite(f_cs, HIGH);
  // Wait for the page to be written
  while(flash_read_status() & STAT_WIP);
//delay(1);
}

//=====================================
// Tpp Typ=0.64ms Max=5ms
// measured 1667us
void flash_page_program(unsigned char *wp,int pn)
{
  int address;
  
  // Send Write Enable command
  digitalWrite(f_cs, LOW);
  SPI.transfer(CMD_WRITE_ENABLE);
  digitalWrite(f_cs, HIGH);
  
  digitalWrite(f_cs, LOW);
  SPI.transfer(CMD_PAGE_PROGRAM);
  // Send the 3 byte address
  address = page_to_address(pn);
  SPI.transfer((address >> 16) & 0xff);
  SPI.transfer((address >> 8) & 0xff);
  SPI.transfer(address & 0xff);
  // Now write 256 bytes to the page
  for(int i = 0;i < 256;i++) {
    SPI.transfer(*wp++);
  }
  digitalWrite(f_cs, HIGH);
//delay(1);
  // Wait for the page to be written
  while(flash_read_status() & STAT_WIP);

}

//=====================================
// measured = 664us
void flash_read_pages(unsigned char *rp,int pn,int n_pages)
{
  int address;
  
  digitalWrite(f_cs, LOW);
  SPI.transfer(CMD_READ_DATA);
  // Send the 3 byte address
  address = page_to_address(pn);
  SPI.transfer((address >> 16) & 0xff);
  SPI.transfer((address >> 8) & 0xff);
  SPI.transfer(address & 0xff);
  // Now read the page's data bytes
  for(int i = 0;i < n_pages * 256;i++) {
    *rp++ = SPI.transfer(0);
  }
  digitalWrite(f_cs, HIGH);
}

//=====================================
// measured = 442us for one page
// Read specified number of pages starting with pn
void flash_fast_read_pages(unsigned char *rp,int pn,int n_pages)
{
  int address;

// The chip doesn't run at the higher clock speed until
// after the command and address have been sent
  digitalWrite(f_cs, LOW);
  SPI.transfer(CMD_READ_HIGH_SPEED);
  // Send the 3 byte address
  address = page_to_address(pn);
  SPI.transfer((address >> 16) & 0xff);
  SPI.transfer((address >> 8) & 0xff);
  SPI.transfer(address & 0xff);
  // send dummy byte
  SPI.transfer(0);
  // Double the clock speed
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  // Now read the number of pages required
  for(int i = 0;i < n_pages * 256;i++) {
    *rp++ = SPI.transfer(0);
  }
  digitalWrite(f_cs, HIGH);
  // reset the clock speed
  SPI.setClockDivider(SPI_CLOCK_DIV4);
}

//=====================================
void flash_init(void)
{ 
  pinMode(f_cs,OUTPUT); // chip select
  // start the SPI library:
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  // MODE 3 works with the M25P16 flash chip
  SPI.setDataMode(SPI_MODE3);
  // Some of it fails with SPI_CLOCK_DIV2
  SPI.setClockDivider(SPI_CLOCK_DIV4);
//  delay(10);
}