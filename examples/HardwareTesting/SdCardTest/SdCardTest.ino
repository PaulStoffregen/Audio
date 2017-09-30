// SD Card Test
//
// Check if the SD card on the Audio Shield is working,
// and perform some simple speed measurements to gauge
// its ability to play 1, 2, 3 and 4 WAV files at a time.
//
// Requires the audio shield:
//   http://www.pjrc.com/store/teensy3_audio.html
//
// Data files to put on your SD card can be downloaded here:
//   http://www.pjrc.com/teensy/td_libs_AudioDataFiles.html
//
// This example code is in the public domain.

#include <SD.h>
#include <SPI.h>

// Use these with the Teensy Audio Shield
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14

// Use these with the Teensy 3.5 & 3.6 SD card
//#define SDCARD_CS_PIN    BUILTIN_SDCARD
//#define SDCARD_MOSI_PIN  11  // not actually used
//#define SDCARD_SCK_PIN   13  // not actually used

// Use these for the SD+Wiz820 or other adaptors
//#define SDCARD_CS_PIN    4
//#define SDCARD_MOSI_PIN  11
//#define SDCARD_SCK_PIN   13

void setup() {
  Sd2Card card;
  SdVolume volume;
  File f1, f2, f3, f4;
  char buffer[512];
  boolean status;
  unsigned long usec, usecMax;
  elapsedMicros usecTotal, usecSingle;
  int i, type;
  float size;

  // wait for the Arduino Serial Monitor to open
  while (!Serial) ;
  delay(50);

  // Configure SPI
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);

  Serial.begin(9600);
  Serial.println("SD Card Test");
  Serial.println("------------");

  // First, detect the card
  status = card.init(SPI_FULL_SPEED, SDCARD_CS_PIN);
  if (status) {
    Serial.println("SD card is connected :-)");
  } else {
    Serial.println("SD card is not connected or unusable :-(");
    return;
  }

  type = card.type();
  if (type == SD_CARD_TYPE_SD1 || type == SD_CARD_TYPE_SD2) {
    Serial.println("Card type is SD");
  } else if (type == SD_CARD_TYPE_SDHC) {
    Serial.println("Card type is SDHC");
  } else {
    Serial.println("Card is an unknown type (maybe SDXC?)");
  }

  // Then look at the file system and print its capacity
  status = volume.init(card);
  if (!status) {
    Serial.println("Unable to access the filesystem on this card. :-(");
    return;
  }

  size = volume.blocksPerCluster() * volume.clusterCount();
  size = size * (512.0 / 1e6); // convert blocks to millions of bytes
  Serial.print("File system space is ");
  Serial.print(size);
  Serial.println(" Mbytes.");

  // Now open the SD card normally
  status = SD.begin(SDCARD_CS_PIN);
  if (status) {
    Serial.println("SD library is able to access the filesystem");
  } else {
    Serial.println("SD library can not access the filesystem!");
    Serial.println("Please report this problem, with the make & model of your SD card.");
    Serial.println("  http://forum.pjrc.com/forums/4-Suggestions-amp-Bug-Reports");
  }


  // Open the 4 sample files.  Hopefully they're on the card
  f1 = SD.open("SDTEST1.WAV");
  f2 = SD.open("SDTEST2.WAV");
  f3 = SD.open("SDTEST3.WAV");
  f4 = SD.open("SDTEST4.WAV");

  // Speed test reading a single file
  if (f1) {
    Serial.println();
    Serial.println("Reading SDTEST1.WAV:");
    if (f1.size() >= 514048) {
      usecMax = 0;
      usecTotal = 0;
      for (i=0; i < 1000; i++) {
        usecSingle = 0;
        f1.read(buffer, 512);
        usec = usecSingle;
        if (usec > usecMax) usecMax = usec;
      }
      reportSpeed(1, 1000, usecTotal, usecMax);
    } else {
      Serial.println("SDTEST1.WAV is too small for speed testing");
    }
  } else {
    Serial.println("Unable to find SDTEST1.WAV on this card");
    return;
  }

  // Speed test reading two files
  if (f2) {
    Serial.println();
    Serial.println("Reading SDTEST1.WAV & SDTEST2.WAV:");
    if (f2.size() >= 514048) {
      f1.seek(0);
      usecMax = 0;
      usecTotal = 0;
      for (i=0; i < 1000; i++) {
        usecSingle = 0;
        f1.read(buffer, 512);
        f2.read(buffer, 512);
        usec = usecSingle;
        if (usec > usecMax) usecMax = usec;
      }
      reportSpeed(2, 1000, usecTotal, usecMax);

      Serial.println();
      Serial.println("Reading SDTEST1.WAV & SDTEST2.WAV staggered:");
      f1.seek(0);
      f2.seek(0);
      f1.read(buffer, 512);
      usecMax = 0;
      usecTotal = 0;
      for (i=0; i < 1000; i++) {
        usecSingle = 0;
        f1.read(buffer, 512);
        f2.read(buffer, 512);
        usec = usecSingle;
        if (usec > usecMax) usecMax = usec;
      }
      reportSpeed(2, 1000, usecTotal, usecMax);
    } else {
      Serial.println("SDTEST2.WAV is too small for speed testing");
    }
  } else {
    Serial.println("Unable to find SDTEST2.WAV on this card");
    return;
  }


  // Speed test reading three files
  if (f3) {
    Serial.println();
    Serial.println("Reading SDTEST1.WAV, SDTEST2.WAV, SDTEST3.WAV:");
    if (f3.size() >= 514048) {
      f1.seek(0);
      f2.seek(0);
      usecMax = 0;
      usecTotal = 0;
      for (i=0; i < 1000; i++) {
        usecSingle = 0;
        f1.read(buffer, 512);
        f2.read(buffer, 512);
        f3.read(buffer, 512);
        usec = usecSingle;
        if (usec > usecMax) usecMax = usec;
      }
      reportSpeed(3, 1000, usecTotal, usecMax);

      Serial.println();
      Serial.println("Reading SDTEST1.WAV, SDTEST2.WAV, SDTEST3.WAV staggered:");
      f1.seek(0);
      f2.seek(0);
      f3.seek(0);
      f1.read(buffer, 512);
      f1.read(buffer, 512);
      f2.read(buffer, 512);
      usecMax = 0;
      usecTotal = 0;
      for (i=0; i < 1000; i++) {
        usecSingle = 0;
        f1.read(buffer, 512);
        f2.read(buffer, 512);
        f3.read(buffer, 512);
        usec = usecSingle;
        if (usec > usecMax) usecMax = usec;
      }
      reportSpeed(3, 1000, usecTotal, usecMax);
    } else {
      Serial.println("SDTEST3.WAV is too small for speed testing");
    }
  } else {
    Serial.println("Unable to find SDTEST3.WAV on this card");
    return;
  }


  // Speed test reading four files
  if (f4) {
    Serial.println();
    Serial.println("Reading SDTEST1.WAV, SDTEST2.WAV, SDTEST3.WAV, SDTEST4.WAV:");
    if (f4.size() >= 514048) {
      f1.seek(0);
      f2.seek(0);
      f3.seek(0);
      usecMax = 0;
      usecTotal = 0;
      for (i=0; i < 1000; i++) {
        usecSingle = 0;
        f1.read(buffer, 512);
        f2.read(buffer, 512);
        f3.read(buffer, 512);
        f4.read(buffer, 512);
        usec = usecSingle;
        if (usec > usecMax) usecMax = usec;
      }
      reportSpeed(4, 1000, usecTotal, usecMax);

      Serial.println();
      Serial.println("Reading SDTEST1.WAV, SDTEST2.WAV, SDTEST3.WAV, SDTEST4.WAV staggered:");
      f1.seek(0);
      f2.seek(0);
      f3.seek(0);
      f4.seek(0);
      f1.read(buffer, 512);
      f1.read(buffer, 512);
      f1.read(buffer, 512);
      f2.read(buffer, 512);
      f2.read(buffer, 512);
      f3.read(buffer, 512);
      usecMax = 0;
      usecTotal = 0;
      for (i=0; i < 1000; i++) {
        usecSingle = 0;
        f1.read(buffer, 512);
        f2.read(buffer, 512);
        f3.read(buffer, 512);
        f4.read(buffer, 512);
        usec = usecSingle;
        if (usec > usecMax) usecMax = usec;
      }
      reportSpeed(4, 1000, usecTotal, usecMax);
    } else {
      Serial.println("SDTEST4.WAV is too small for speed testing");
    }
  } else {
    Serial.println("Unable to find SDTEST4.WAV on this card");
    return;
  }

}


unsigned long maximum(unsigned long a, unsigned long b,
  unsigned long c, unsigned long d)
{
  if (b > a) a = b;
  if (c > a) a = c;
  if (d > a) a = d;
  return a;
}


void reportSpeed(unsigned int numFiles, unsigned long blockCount, unsigned long usecTotal, unsigned long usecMax)
{
  float bytesPerSecond = (float)(blockCount * 512 * numFiles) / usecTotal;
  Serial.print("  Overall speed = ");
  Serial.print(bytesPerSecond);
  Serial.println(" Mbyte/sec");
  Serial.print("  Worst block time = ");
  Serial.print((float)usecMax / 1000.0);
  Serial.println(" ms");
  Serial.print("    ");
  Serial.print( (float)usecMax / 29.01333);
  Serial.println("% of audio frame time");
}


void loop(void) {
  // do nothing after the test
}
