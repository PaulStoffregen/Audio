// Plays a RAW (16-bit signed) PCM audio file at slower or faster rate
// this example requires an uSD-card inserted to teensy 3.6 with a file called DEMO.raw
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioPlaySdRawResampled  playSdRaw1;     //xy=324,457
AudioOutputI2S           i2s2;           //xy=840.8571472167969,445.5714416503906
AudioConnection          patchCord1(playSdRaw1, 0, i2s2, 0);
AudioConnection          patchCord2(playSdRaw1, 0, i2s2, 1);
// GUItool: end automatically generated code

AudioControlSGTL5000 audioShield;

void setup() {

    Serial.begin(57600);

    if (!(SD.begin(BUILTIN_SDCARD))) {
        // stop here if no SD card, but print a message
        while (1) {
            Serial.println("Unable to access the SD card");
            delay(500);
        }
    }

    AudioMemory(24);

    audioShield.enable();
    audioShield.volume(0.5);

    playSdRaw1.play("DEMO.RAW");
    playSdRaw1.setReadRate(1.5);
    Serial.println("playing...");
}

void loop() {

    if (!playSdRaw1.isPlaying()) {
        //Serial.println("playing...");

        playSdRaw1.play("demo2.raw");
    }
}
