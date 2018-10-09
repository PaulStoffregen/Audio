// Simple WAV file player from QSPI circuitpython filesystem example
//
//
// This example code is in the public domain.

#include <Audio.h>
#include <Adafruit_SPIFlash_FatFs.h>
#include "Adafruit_QSPI_GD25Q.h"
#define FLASH_TYPE     SPIFLASHTYPE_W25Q16BV  // Flash chip type.
Adafruit_QSPI_GD25Q flash;

// create an Adafruit_M0_Express_CircuitPython object which gives
// an SD card-like interface to interacting with files stored in CircuitPython's
// flash filesystem.
Adafruit_M0_Express_CircuitPython pythonfs(flash);

AudioPlayQspiWav           playWav1;
AudioOutputAnalogStereo  audioOutput;
AudioConnection          patchCord1(playWav1, 0, audioOutput, 0);
AudioConnection          patchCord2(playWav1, 1, audioOutput, 1);

void setup() {
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(8);

  // Initialize serial port
  Serial.begin(115200);
  Serial.println("Adafruit M4 CircuitPython flash wavefile player");

  // Initialize flash library and check its chip ID.
  if (!flash.begin()) {
    while(1){
      Serial.println("Error, failed to initialize flash chip!");
      delay(1000);
    }
  }
  flash.setFlashType(FLASH_TYPE);

  // First call begin to mount the filesystem.  Check that it returns true
  // to make sure the filesystem was mounted.
  if (!pythonfs.begin()) {
    while(1){
      Serial.println("Failed to mount filesystem!");
      Serial.println("Was CircuitPython loaded on the board first to create the filesystem?");
    }
  }
  Serial.println("Mounted filesystem!");

  playWav1.useFilesystem(&pythonfs);
}

void playFile(const char *filename)
{
  Serial.print("Playing file: ");
  Serial.println(filename);

  // Start playing the file.  This sketch continues to
  // run while the file plays.
  playWav1.play(filename);

  // A brief delay for the library read WAV info
  delay(5);

  // Simply wait for the file to finish playing.
  while (playWav1.isPlaying()) {
    
  }
}


void loop() {
  playFile("A6-49-96.WAV");  // filenames are always uppercase 8.3 format
  delay(1500);
}

