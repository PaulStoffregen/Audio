#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>

// Bitcrusher example by Jonathan Payne (Pensive) jon@jonnypayne.com
// No rights reserved. Do what you like with it.

// Create the Audio components.
AudioPlaySdWav      playWav1;       //this will play a looping demo file
AudioOutputI2S      headphones;           // and it will come out of the headphone socket.

//Audio Effect BitCrusher defs here
AudioEffectBitcrusher   left_BitCrusher;
AudioEffectBitcrusher   right_BitCrusher;

// Create Audio connections between the components
AudioConnection         patchCord1(playWav1, 0, left_BitCrusher, 0);
AudioConnection         patchCord2(playWav1, 1, right_BitCrusher, 0);
AudioConnection         patchCord3(left_BitCrusher, 0, headphones, 0);
AudioConnection         patchCord4(right_BitCrusher, 0, headphones, 1);

// Create an object to control the audio shield.
//
AudioControlSGTL5000 audioShield;

// Bounce objects to read six pushbuttons (pins 0-5)
//
Bounce button0 = Bounce(0, 5); // cycles the bitcrusher through all bitdepths
Bounce button1 = Bounce(1, 5); //cycles the bitcrusher through some key samplerates
Bounce button2 = Bounce(2, 5); // turns on the compressor (or "Automatic Volume Leveling")

unsigned long SerialMillisecondCounter;

//BitCrusher
int current_CrushBits = 16; //this defaults to passthrough.
int current_SampleRate = 44100; // this defaults to passthrough.

//Compressor
boolean compressorOn = false; // default this to off.

void setup() {
  // Configure the pushbutton pins for pullups.
  // Each button should connect from the pin to GND.
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  Serial.begin(38400); // open the serial

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(40); //this is WAY more tha nwe need

  // turn on the output
  audioShield.enable();
  audioShield.volume(0.7);

  // by default the Teensy 3.1 DAC uses 3.3Vp-p output
  // if your 3.3V power has noise, switching to the
  // internal 1.2V reference can give you a clean signal
  //dac.analogReference(INTERNAL);

  // reduce the gain on mixer channels, so more than 1
  // sound can play simultaneously without clipping

  //SDCard Initialise
  SPI.setMOSI(7);
  SPI.setSCK(14);
  if (!(SD.begin(10))) {
    // stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
  // Bitcrusher
  left_BitCrusher.bits(current_CrushBits); //set the crusher to defaults. This will passthrough clean at 16,44100
  left_BitCrusher.sampleRate(current_SampleRate); //set the crusher to defaults. This will passthrough clean at 16,44100
  right_BitCrusher.bits(current_CrushBits); //set the crusher to defaults. This will passthrough clean at 16,44100
  right_BitCrusher.sampleRate(current_SampleRate); //set the crusher to defaults. This will passthrough clean at 16,44100
  //Bitcrusher


  /* Valid values for dap_avc parameters

	maxGain; Maximum gain that can be applied
	0 - 0 dB
	1 - 6.0 dB
	2 - 12 dB
	
	lbiResponse; Integrator Response
	0 - 0 mS
	1 - 25 mS
	2 - 50 mS
	3 - 100 mS
	
	hardLimit
	0 - Hard limit disabled. AVC Compressor/Expander enabled.
	1 - Hard limit enabled. The signal is limited to the programmed threshold (signal saturates at the threshold)
	
	threshold
	floating point in range 0 to -96 dB
	
	attack
	floating point figure is dB/s rate at which gain is increased
	
	decay
	floating point figure is dB/s rate at which gain is reduced
*/
// Initialise the AutoVolumeLeveller
  audioShield.autoVolumeControl(1, 1, 0, -6, 40, 20); // **BUG** with a max gain of 0, turning the AVC off leaves a hung AVC problem where the attack seems to hang in a loop. with it set 1 or 2, this does not occur.
  audioShield.autoVolumeDisable();
  audioShield.audioPostProcessorEnable();

  SerialMillisecondCounter = millis();
}

int val; //temporary variable for memory usage reporting.

void loop() {

  if (millis() - SerialMillisecondCounter >= 5000) {
    Serial.print("Proc = ");
    Serial.print(AudioProcessorUsage());
    Serial.print(" (");
    Serial.print(AudioProcessorUsageMax());
    Serial.print("),  Mem = ");
    Serial.print(AudioMemoryUsage());
    Serial.print(" (");
    Serial.print(AudioMemoryUsageMax());
    Serial.println(")");
    SerialMillisecondCounter = millis();
    AudioProcessorUsageMaxReset();
    AudioMemoryUsageMaxReset();
  }


  // Update all the button objects
  button0.update();
  button1.update();
  button2.update();

  // Start test sound if it is not playing. This will loop infinitely.
  if (! (playWav1.isPlaying())){
    playWav1.play("SDTEST1.WAV"); // http://www.pjrc.com/teensy/td_libs_AudioDataFiles.html
  }

  if (button0.fallingEdge()) {
    //Bitcrusher BitDepth
    if (current_CrushBits >= 2) { //eachtime you press it, deduct 1 bit from the settings.
        current_CrushBits--;
    } else {
      current_CrushBits = 16; // if you get down to 1 go back to the top.
    }

    left_BitCrusher.bits(current_CrushBits);
    left_BitCrusher.sampleRate(current_SampleRate);
    right_BitCrusher.bits(current_CrushBits);
    right_BitCrusher.sampleRate(current_SampleRate);
    Serial.print("Bitcrusher set to ");
    Serial.print(current_CrushBits);
    Serial.print(" Bit, Samplerate at ");
    Serial.print(current_SampleRate);
    Serial.println("Hz");
  }

  if (button1.fallingEdge()) {
    //Bitcrusher SampleRate // the lowest sensible setting is 345. There is a 128 sample buffer, and this will copy sample 1, to each of the other 127 samples.
    if (current_SampleRate >= 690) { // 345 * 2, so we can do one more divide
      current_SampleRate = current_SampleRate / 2; // half the sample rate each time
    } else {
      current_SampleRate=44100; // if you get down to the minimum then go back to the top and start over.
    }

    left_BitCrusher.bits(current_CrushBits);
    left_BitCrusher.sampleRate(current_SampleRate);
    right_BitCrusher.bits(current_CrushBits);
    right_BitCrusher.sampleRate(current_SampleRate);
    Serial.print("Bitcrusher set to ");
    Serial.print(current_CrushBits);
    Serial.print(" Bit, Samplerate at ");
    Serial.print(current_SampleRate);
    Serial.println("Hz");
  }

  if (button2.fallingEdge()) {
    if (compressorOn) {
      //turn off compressor
      audioShield.autoVolumeDisable();
      compressorOn = false;
    } else {
      //turn on compressor
      audioShield.autoVolumeEnable();
      compressorOn = true;
    }
    Serial.print("Compressor: ");
    Serial.println(compressorOn);
  }
}

