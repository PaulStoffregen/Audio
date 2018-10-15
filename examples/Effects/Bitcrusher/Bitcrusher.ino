/* Press button 1 (top left) on the trellis to cycle through different bit depths.
Press button 2 to cycle through differnt sample rates.
This example plays a WAV file off the QSPI filesystem. */

#include <Audio.h>
#include <Adafruit_Keypad.h>
#include <Adafruit_SPIFlash_FatFs.h>
#include "Adafruit_QSPI_GD25Q.h"
#define FLASH_TYPE     SPIFLASHTYPE_W25Q16BV  // Flash chip type.
Adafruit_QSPI_GD25Q flash;

#define NEO_PIN 10
#define NUM_KEYS 32

// Bitcrusher example by Jonathan Payne (Pensive) jon@jonnypayne.com
// No rights reserved. Do what you like with it.
// modified for trellis m4 by Dean Miller for Adafruit

// Create the Audio components.
AudioPlayQspiWav           playWav1;
AudioOutputAnalogStereo     headphones;

//Audio Effect BitCrusher defs here
AudioEffectBitcrusher   left_BitCrusher;
AudioEffectBitcrusher   right_BitCrusher;

// Create Audio connections between the components
AudioConnection         patchCord1(playWav1, 0, left_BitCrusher, 0);
AudioConnection         patchCord2(playWav1, 1, right_BitCrusher, 0);
AudioConnection         patchCord3(left_BitCrusher, 0, headphones, 0);
AudioConnection         patchCord4(right_BitCrusher, 0, headphones, 1);

const byte ROWS = 4; // four rows
const byte COLS = 8; // eight columns
//define the symbols on the buttons of the keypads
byte trellisKeys[ROWS][COLS] = {
  {1,  2,  3,  4,  5,  6,  7,  8},
  {9,  10, 11, 12, 13, 14, 15, 16},
  {17, 18, 19, 20, 21, 22, 23, 24},
  {25, 26, 27, 28, 29, 30, 31, 32}
};
byte rowPins[ROWS] = {14, 15, 16, 17}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {2, 3, 4, 5, 6, 7, 8, 9}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Adafruit_Keypad customKeypad = Adafruit_Keypad( makeKeymap(trellisKeys), rowPins, colPins, ROWS, COLS);

Adafruit_M0_Express_CircuitPython pythonfs(flash);

//BitCrusher
int current_CrushBits = 16; //this defaults to passthrough.
int current_SampleRate = 44100; // this defaults to passthrough.

void setup() {
  Serial.begin(9600); // open the serial

  customKeypad.begin();

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(40); //this is WAY more tha nwe need
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

}

void loop() {

  customKeypad.tick();
  
  while(customKeypad.available())
  {
    keypadEvent e = customKeypad.read();
    if(e.bit.EVENT == KEY_JUST_PRESSED){
        if(e.bit.KEY == 1){
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
        else if(e.bit.KEY == 2){
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
      }
  }

  // Start test sound if it is not playing. This will loop infinitely.
  if (! (playWav1.isPlaying())){
    playWav1.play("A4-49-96.wav");
  }

  delay(10);
}
