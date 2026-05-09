// Record sound as a stereo WAV file data to an SD card, and play it back.
//
// Requires the audio shield:
//   http://www.pjrc.com/store/teensy3_audio.html
//
// Three pushbuttons need to be connected:
//   Record Button: pin 0 to GND
//   Stop Button:   pin 1 to GND
//   Play Button:   pin 2 to GND
//
// This example code is in the public domain.

#include <Bounce.h>
#include <Audio.h>


// GUItool: begin automatically generated code
AudioPlayWAVstereo       playWAVstereo1; //xy=105,227
AudioInputI2S            i2sIn;           //xy=128,170
AudioAnalyzePeak         peakR; //xy=285,111
AudioOutputI2S           i2sOut;           //xy=285,227
AudioAnalyzePeak         peakL;          //xy=286,79
AudioRecordWAVstereo     recordWAVstereo1; //xy=319,170
AudioConnection          patchCord1(playWAVstereo1, 0, i2sOut, 0);
AudioConnection          patchCord2(playWAVstereo1, 1, i2sOut, 1);
AudioConnection          patchCord3(i2sIn, 0, peakL, 0);
AudioConnection          patchCord4(i2sIn, 0, recordWAVstereo1, 0);
AudioConnection          patchCord5(i2sIn, 1, recordWAVstereo1, 1);
AudioConnection          patchCord6(i2sIn, 1, peakR, 0);
AudioControlSGTL5000     control;     //xy=260,282
// GUItool: end automatically generated code


// Bounce objects to easily and reliably read the buttons
Bounce buttonRecord = Bounce(0, 8);
Bounce buttonStop =   Bounce(1, 8);  // 8 = 8 ms debounce time
Bounce buttonPlay =   Bounce(2, 8);


// which input on the audio shield will be used?
const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;


// Use these with the Teensy Audio Shield
/*
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14
//*/

// Use these with the Teensy 3.5, 3.6 and 4.1 SD card
//*
#define SDCARD_CS_PIN    BUILTIN_SDCARD
#define SDCARD_MOSI_PIN  11  // not actually used
#define SDCARD_SCK_PIN   13  // not actually used
//*/

// Use these for the SD+Wiz820 or other adaptors
/*
#define SDCARD_CS_PIN    4
#define SDCARD_MOSI_PIN  11
#define SDCARD_SCK_PIN   13
//*/

void findSGTL5000(AudioControlSGTL5000& sgtl5000_1)
{
  // search for SGTL5000 at both I²C addresses
  for (int i=0;i<2;i++)
  {
    uint8_t levels[] {LOW,HIGH};
    
    sgtl5000_1.setAddress(levels[i]);
    sgtl5000_1.enable();
    if (sgtl5000_1.volume(0.2))
    {
      Serial.printf("SGTL5000 found at %s address\n",i?"HIGH":"LOW");
      break;
    }
  }
}

// Remember which mode we're doing
int mode = 0;  // 0=stopped, 1=recording, 2=playing

const char* fileName = "Stereo.wav";
void setup() {
  // Configure the pushbutton pins
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);

  // Audio connections require memory:
  AudioMemory(10);

  // SD audio objects need buffers configuring:
  const size_t sz = 65536;
  const AudioBuffer::bufType bufMem = AudioBuffer::inHeap;
  playWAVstereo1.createBuffer(sz,bufMem);
  recordWAVstereo1.createBuffer(sz,bufMem);

  // Enable the audio shield, select input, and enable output
  findSGTL5000(control);
  control.inputSelect(myInput);
  control.lineInLevel(2);
  control.volume(0.5);
  
  // Initialize the SD card
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  while (!(SD.begin(SDCARD_CS_PIN))) 
  {
    // loop here if no SD card, printing a message
    Serial.println("Unable to access the SD card");
    delay(500);
  }
}


void loop() {
  // First, read the buttons
  buttonRecord.update();
  buttonStop.update();
  buttonPlay.update();

  // Respond to button presses
  if (buttonRecord.fallingEdge()) {
    Serial.println("Record Button Press");
    if (mode == 2) stopPlaying();
    if (mode == 0) startRecording();
  }
  if (buttonStop.fallingEdge()) {
    Serial.println("Stop Button Press");
    if (mode == 1) stopRecording();
    if (mode == 2) stopPlaying();
  }
  if (buttonPlay.fallingEdge()) {
    Serial.println("Play Button Press");
    if (mode == 1) stopRecording();
    if (mode == 0) startPlaying();
  }

  // If we're playing or recording, carry on...
  if (mode == 1) {
    continueRecording();
  }
  if (mode == 2) {
    continuePlaying();
  }

  showLevels();
  // when using a microphone, continuously adjust gain
  if (myInput == AUDIO_INPUT_MIC) adjustMicLevel();
}


void startRecording() 
{
  Serial.println("startRecording");
  recordWAVstereo1.recordSD(fileName);
  mode = 1;
}

void continueRecording() 
{
  // nothing to do here!
}

void stopRecording() 
{
  Serial.println("stopRecording");
  recordWAVstereo1.stop();  
  mode = 0;
}


void startPlaying() 
{
  Serial.println("startPlaying");
  playWAVstereo1.playSD(fileName);
  mode = 2;
}

void continuePlaying() 
{
  if (!playWAVstereo1.isPlaying()) 
  {
    Serial.println("endOfPlayback");
    mode = 0;
  }
}

void stopPlaying() 
{
  Serial.println("stopPlaying");
  playWAVstereo1.stop();
  mode = 0;
}

void adjustMicLevel() 
{
  // TODO: read the peak1 object and adjust control.micGain()
  // if anyone gets this working, please submit a github pull request :-)
}

void showLevels()
{
  static uint32_t nextOutput;

  if (millis() > nextOutput)
  {
    int lp = 0, rp = 0, scale = 20;
    char cl='?',cr='?';
    char buf[scale*2+10];
    
    nextOutput = millis() + 1000;

    if (peakL.available())
    {
      lp = peakL.read() * scale; 
      cl = '#'; 
    }
    if (peakR.available())
    {
      rp = peakR.read() * scale;  
      cr = '#'; 
    }

    for (int i=0;i<scale;i++)
    {
      buf[scale-i-1] = lp>=i?cl:' ';
      buf[scale+i+1] = rp>=i?cr:' ';
      buf[scale] = '|';
      buf[scale*2+2] = 0;
    }
    Serial.println(buf);
  }
}
