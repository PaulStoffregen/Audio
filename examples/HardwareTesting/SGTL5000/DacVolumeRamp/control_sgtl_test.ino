#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

// GUItool: begin automatically generated code
AudioSynthWaveformSine   sine1;          //xy=203,233
AudioOutputI2S           i2s1;           //xy=441,233
AudioConnection          patchCord1(sine1, 0, i2s1, 0);
AudioConnection          patchCord2(sine1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=452,162
// GUItool: end automatically generated code


elapsedMillis fps;
float dac_vol;
float dac_vol_inc;
boolean with_ramp;
boolean with_ramp_exponential;

void setup(void) {
  Serial.begin(9600);

  AudioMemory(4);
  dac_vol = 0.0;
  dac_vol_inc = 0.2;
  
  // the ramp seems to be around 768 samples linear, and around 256 samples for exponential.
  // without ramp, the volume change seems to happen around the zero crossing.
  
  with_ramp = true;
  with_ramp_exponential = false;
  
  sine1.amplitude(1.0);
  sine1.frequency(440);

  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5); // set the HP volume...
  sgtl5000_1.dacRampEnable(with_ramp_exponential);

  Serial.println("setup done");
}

void loop(void) {
  if (fps > 500) { // change the volume every 5 seconds
    Serial.print("Volume");
    Serial.print(dac_vol);
    Serial.print(",\tVolume increment");
    Serial.print(dac_vol_inc);
    Serial.print("\tramp: ");
    Serial.print(with_ramp);
    Serial.print(",");
    Serial.println(with_ramp_exponential);
    
    dac_vol += dac_vol_inc;
    if (dac_vol >= 1.0) {
      dac_vol = 1.0;
      dac_vol_inc = -dac_vol_inc;
    }
    if (dac_vol <= 0.0) {
      dac_vol = 0.0;
      dac_vol_inc = -dac_vol_inc;

      with_ramp = !with_ramp;
      if (with_ramp) {
        with_ramp_exponential = !with_ramp_exponential;
        sgtl5000_1.dacRampEnable(with_ramp_exponential);
      } else {
        sgtl5000_1.dacRampDisable();
      }
    }
    
    // this is a logarithmic volume, \
    // that is, the range 0.0 to 1.0 gets converted to -90dB to 0dB in 0.5dB steps
    sgtl5000_1.dacVolume(dac_vol); 
    
    fps = 0;
  }
}



