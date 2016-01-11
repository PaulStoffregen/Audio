// The SGTL5000 has a secondard "dacVolume" setting, in addition to normal volume.
// 
// Normally dacVolume defaults to 1.0, to pass your sound directly to the normal
// volume control.  The actual volume you hear depends on both settings.
//
// The dacVolume control has an option to gracefully ramp (change) its setting over
// time, to prevent a sudden pop or click sound.  You could achieve this by adding
// code to change the volume setting in very small steps over time, but the
// SGTL5000 can do it for you automatically.
//
// This example was originally contributed by Hedde Bosman

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthWaveformSine   sine1;          //xy=203,233
AudioOutputI2S           i2s1;           //xy=441,233
AudioConnection          patchCord1(sine1, 0, i2s1, 0);
AudioConnection          patchCord2(sine1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=452,162
// GUItool: end automatically generated code


elapsedMillis msec;
float vol;
float inc;
int rampType;
const char *rampName[] = {
  "No Ramp (instant)",  // loud pop due to instant change
  "Normal Ramp",        // graceful transition between volume levels
  "Linear Ramp"         // slight click/chirp
};

void setup(void) {
  Serial.begin(9600);

  AudioMemory(4);
  vol = 0.0;
  inc = 0.2;

  rampType = 0;

  sine1.amplitude(1.0);
  sine1.frequency(440);

  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);     // set the main volume...
  sgtl5000_1.dacVolume(0);    // set the "dac" volume (extra control)
  sgtl5000_1.dacVolumeRampDisable();

  Serial.println("setup done");
}

void loop(void) {
  if (msec > 1000) { // change the volume every second

    // increment or decrement the volume variable
    vol += inc;
    if (vol >= 1.0) {
      vol = 1.0;
      inc = -inc;
    }
    if (vol < 0.01) {
      vol = 0.0;
      inc = -inc;
    }
    Serial.print("Volume: ");
    Serial.print(vol);
    Serial.print("   ");
    Serial.println(rampName[rampType]);

    // configure which type of volume transition to use
    if (rampType == 0) {
      sgtl5000_1.dacVolumeRampDisable();
    } else if (rampType == 1) {
      sgtl5000_1.dacVolumeRamp();
    } else {
      sgtl5000_1.dacVolumeRampLinear();
    }

    // set the dacVolume.  The actual change make take place over time, if ramping
    // this is a logarithmic volume,
    // that is, the range 0.0 to 1.0 gets converted to -90dB to 0dB in 0.5dB steps
    sgtl5000_1.dacVolume(vol);

    // if we turned the volume off, advance to the next ramp type
    if (vol < 0.01) {
      rampType = rampType + 1;
      if (rampType > 2) rampType = 0;
    }
    msec = 0;
  }
}



