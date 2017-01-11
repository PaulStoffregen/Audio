#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

AudioInputUSB            usb1;           //xy=200,69  (must set Tools > USB Type to Audio)
AudioOutputI2S           i2s1;           //xy=365,94
AudioConnection          patchCord1(usb1, 0, i2s1, 0);
AudioConnection          patchCord2(usb1, 1, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=302,184

void setup() {                
  AudioMemory(12);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.6);
}

void loop() {
  // TODO: make PC's volume setting control the SGTL5000 volume...
}

// A known problem occurs on Macintosh computers, where the Mac's driver
// does not seem to be able to adapt and transmit horribly distorted
// audio to Teensy after a matter of minutes.  An imperfect workaround
// can be enabled by editing usb_audio.cpp.  Find and uncomment
// "#define MACOSX_ADAPTIVE_LIMIT".  More detailed info is available here:
// https://forum.pjrc.com/threads/34855-Distorted-audio-when-using-USB-input-on-Teensy-3-1?p=110392&viewfull=1#post110392

