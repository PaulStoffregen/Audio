// Simple WAV file player from SD or QSPI filesystem example
// This example plays 2 wav files at once
//
// This example code is in the public domain.

#include <Audio.h>
#include <SdFat.h>
#include <Adafruit_SPIFlash.h>
#include <play_fs_wav.h>

AudioPlayFSWav           playWav1;
AudioPlayFSWav           playWav2;
AudioMixer4              mixer1;
AudioMixer4              mixer2;
AudioOutputAnalogStereo  audioOutput;
AudioConnection          patchCord1(playWav1, 0, mixer1, 0);
AudioConnection          patchCord2(playWav1, 1, mixer2, 0);
AudioConnection          patchCord3(playWav2, 0, mixer1, 1);
AudioConnection          patchCord4(playWav2, 1, mixer2, 1);
AudioConnection          patchCord5(mixer1, 0, audioOutput, 0);
AudioConnection          patchCord6(mixer2, 0, audioOutput, 1);


// Use these with the PyGamer
#if defined(ADAFRUIT_PYGAMER_M4_EXPRESS)
  #define SDCARD_CS_PIN    4
  #define SPEAKER_ENABLE   51
  Adafruit_FlashTransport_QSPI flashTransport(PIN_QSPI_SCK, PIN_QSPI_CS, PIN_QSPI_IO0, PIN_QSPI_IO1, PIN_QSPI_IO2, PIN_QSPI_IO3);
#elif defined(ADAFRUIT_PYPORTAL)
  #define SDCARD_CS_PIN    32
  #define SPEAKER_ENABLE   50
  Adafruit_FlashTransport_QSPI flashTransport(PIN_QSPI_SCK, PIN_QSPI_CS, PIN_QSPI_IO0, PIN_QSPI_IO1, PIN_QSPI_IO2, PIN_QSPI_IO3);
#endif

Adafruit_SPIFlash flash(&flashTransport);
FatFileSystem QSPIFS;
SdFat SD;
bool SDOK=false, QSPIOK=false;

void setup() {
  // Initialize serial port
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("Multi wave player demo");
  delay(100);

#ifdef SPEAKER_ENABLE
  pinMode(SPEAKER_ENABLE, OUTPUT);
  digitalWrite(SPEAKER_ENABLE, HIGH);
#endif

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(16);

  SDOK = false;
  if (!(SD.begin(SDCARD_CS_PIN))) {
    Serial.println("Unable to access the SD card");
  } else {
    Serial.println("SD card OK!");
    SDOK = true;
  }
  
  QSPIOK = false;
  if (!flash.begin()) {
    Serial.println("Error, failed to initialize flash chip!");
  } else if (!QSPIFS.begin(&flash)) {
    Serial.println("Failed to mount QSPI filesystem!");
    Serial.println("Was CircuitPython loaded on the board first to create the filesystem?");
  } else {
    Serial.println("QSPI OK!");
    QSPIOK = true;
  }
}

void playFiles(const char *filename1, const char *filename2)
{
  Serial.print("Playing files: ");
  Serial.print(filename1);
  Serial.print(" and ");
  Serial.println(filename2);

  File f1, f2;
  
  if (SDOK) {
    f1 = SD.open(filename1);
    f2 = SD.open(filename2);
  } else if (QSPIOK) {
    f1 = QSPIFS.open(filename1);
    f2 = QSPIFS.open(filename2);
  }

  // run while the file plays.
  if (!playWav1.play(f1) || !playWav2.play(f2))	 {
    Serial.println("Failed to play");
    return;
  }

  // A brief delay for the library read WAV info
  delay(5);

  // Simply wait for the file to finish playing.
  while (playWav1.isPlaying() || playWav2.isPlaying()) {
    Serial.print(".");
    delay(100);
  }
}


void loop() {
  playFiles("SDTEST1.wav", "SDTEST2.wav");
  delay(1500);
}