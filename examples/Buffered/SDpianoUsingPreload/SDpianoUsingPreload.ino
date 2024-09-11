#include "Arduino.h"
#include "SD.h"
#include "Audio.h"

//=============================================================
// For debugging, use (custom) Dual Serial + MIDI + Audio
// Install TeensyDebug library
// Set GDB to "Dual Serial"
// and uncomment the following line:
//#include "TeensyDebug.h"

#if defined(GDB_IS_ENABLED)
#pragma GCC optimize ("O0")
#endif

#include "SDpiano.h"

// Needs fix for AudioEffectEnvelope:
// https://github.com/PaulStoffregen/Audio/pull/444

//=============================================================
// mixer array for an 8-voice piano
// GUItool: begin automatically generated code
AudioMixer4              mixerL1; //xy=512,191
AudioMixer4              mixerL2; //xy=514,252
AudioMixer4              mixerR1; //xy=516,312
AudioMixer4              mixerR2;  //xy=516,373
AudioMixer4              mixerL;         //xy=699,223
AudioMixer4              mixerR;         //xy=701,344
AudioOutputI2S           i2sOut;           //xy=948,268

AudioConnection          patchCord1(mixerL1, 0, mixerL, 0);
AudioConnection          patchCord2(mixerL2, 0, mixerL, 1);
AudioConnection          patchCord3(mixerR1, 0, mixerR, 0);
AudioConnection          patchCord4(mixerR2, 0, mixerR, 1);
AudioConnection          patchCord5(mixerL, 0, i2sOut, 0);
AudioConnection          patchCord6(mixerR, 0, i2sOut, 1);

AudioControlSGTL5000     sgtl5000;     //xy=936,328
// GUItool: end automatically generated code
//=============================================================
#define VOICES 8
PianoVoice pv[VOICES];
AudioMixer4* mixers[] = {&mixerL1,&mixerL2,&mixerR1,&mixerR2};
//=============================================================

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

//=============================================================
/*
 * Use an IntervalTimer to ensure system latency is not affected by
 * SD card buffering. 
 * 
 * For this scheme we run the MIDI engine, starting and stopping notes,
 * until we have no further MIDI events to process.
 * 
 * We actually use a slightly convoluted method, because IntervalTimer may 
 * be set to a high priority, but we want the MIDI processor to execute at
 * a higher priority than foreground / yield() code, but at a lower priority
 * than the (already low!) audio engine.
 * 
 * Hence we set up an unused interrupt at super-low priority, and all the
 * IntervalTimer does is to .end() itself (so it doesn't re-trigger) and 
 * trigger the MIDI service interrupt. The latter then runs until no events
 * are left, sets up the IntervalTimer once again with the new delay, and exits.
 */
#define IRQ_SYNTH IRQ_Reserved3
#define RESTART_ADDR 0xE000ED0C
#define WRITE_RESTART(val) ((*(volatile uint32_t *)RESTART_ADDR) = (val))
void setupSynthInterrupt()
{
  attachInterruptVector(IRQ_SYNTH, runSynthIRQ);
  NVIC_SET_PRIORITY(IRQ_SYNTH, 224); // lower priority than audio
  NVIC_ENABLE_IRQ(IRQ_SYNTH);
}

IntervalTimer synthTimer;
void synthIntvl()
{
  digitalWrite(LED_BUILTIN,1);
  synthTimer.end(); // synth IRQ will .begin() it again
  NVIC_SET_PENDING(IRQ_SYNTH);
}

void runSynthIRQ()
{
  usbMIDI.read(); // do all MIDI actions
  synthTimer.begin(synthIntvl,1000);  // run again in 1ms
  digitalWrite(LED_BUILTIN,0);
  asm("DSB");
}

//=============================================================

size_t sppOff;
uint8_t layer,threshold;
float gain;
allNotes_t allNotes = {.lowest = 21 /* A0 */};

//=============================================================
void setup() 
{
  pinMode(LED_BUILTIN,OUTPUT);

  // Open serial communications and wait for port to open:
  Serial.begin(115200);
   while (!Serial) 
    ; // wait for serial port to connect.

  // initialise SD card
  while (!SD.begin(BUILTIN_SDCARD)) 
  {
    Serial.println("initialization failed!");
    delay(250);
  }

  // initialise audio engine
  AudioMemory(50);
  findSGTL5000(sgtl5000);

  // create connections to output mixers
  // and buffers for sampler voices
  int mn=0,mi=0;
  for (uint8_t i=0;i<COUNT_OF(pv);i++)
  {
    // pv[i].setRelease(500.0f);
    pv[i].playWAVstereo1.createBuffer(32768,AudioBuffer::inExt);
    pv[i].connect(mixers[mn+0],mi,mixers[mn+2],mi);
    mi++;
    if (mi > 3)
    {
      mn++;
      mi = 0;
    }
  }

#if defined(GDB_IS_ENABLED)
  halt_cpu();
#endif


  // scan directory and fill in data structure
  // we expect to find 3 folders under the root, called
  // loud, med and soft; within the folders are the relevant
  // layer files, with names like Bb4<something>.wav
  // which are decoded to the MIDI note number
  File root = SD.open("/piano");
  
  printDirectory(root, 0);
  allNotes.maxIdx = NOTES-1;
  Serial.println();

  // for debug, print out the list of files we've found
  for (int i=0;i<NOTES;i++)
  {
    Serial.printf("%3d: %-30s, %-30s, %-30s\n",
                  i+allNotes.lowest,
                  allNotes.notes[i].layers[0].pload.filepath,
                  allNotes.notes[i].layers[1].pload.filepath,
                  allNotes.notes[i].layers[2].pload.filepath);
  }
  Serial.println("Pre-load complete - play!\n");

  // set up the MIDI system
  allocInit();
  usbMIDI.setHandleNoteOff(myNoteOff);
  usbMIDI.setHandleNoteOn(myNoteOn);

  // set up the low-priority interrupt handler
  // to try to improve MIDI->sound latency
  /*
  setupSynthInterrupt();
  synthTimer.begin(synthIntvl,1000);  // run MIDI update in 1ms
  //*/
}

//=============================================================
// Note->voice allocation
static uint32_t actFlags[4];
bool isActive(uint8_t note)
{
  int idx  = note >> 5;
  uint32_t bitF = 1UL << (note & 0x1F);

  return 0 != (actFlags[idx] & bitF);
}


void setActive(uint8_t note)
{
  int idx  = note >> 5;
  uint32_t bitF = 1UL << (note & 0x1F);

  actFlags[idx] |= bitF;
}


void clearActive(uint8_t note)
{
  int idx  = note >> 5;
  uint32_t bitF = 1UL << (note & 0x1F);

  actFlags[idx] &= ~bitF;
}


#define UNUSED 0x80
uint8_t allocated[VOICES];
void allocInit(void)
{
  for (uint8_t i=0;i<COUNT_OF(allocated);i++)
    allocated[i] = UNUSED;
}


// find voice allocated to note, or -1 if we fail
int8_t allocFind(uint8_t note)
{
  int8_t rv = (int8_t) COUNT_OF(allocated);

  while (--rv >= 0 && allocated[rv] != note)
    ;

  return rv;
}


int8_t allocVoice(uint8_t note)
{
  int8_t rv;
  
  if (isActive(note))
    rv = allocFind(note);
  else
    rv = allocFind(UNUSED);

  if (rv >= 0)
  {
    allocated[rv] = note;
    setActive(note);
  }
    
  return rv;
}


int8_t deAllocVoice(uint8_t note)
{
  int8_t rv = allocFind(note);

  if (rv >= 0)
  {
    allocated[rv] = UNUSED;
    clearActive(note);
  }

  return rv;
}

//=============================================================
void myNoteOn(uint8_t channel, uint8_t note, uint8_t velocity)
{
  int nv = note - allNotes.lowest; // convert MIDI number to array index
  if (nv <= allNotes.maxIdx)
  {
    int8_t av = allocVoice(note); // try to allocate a voice to play this note
    if (av >= 0) // we have a free voice, start playing
      pv[av].startNote(allNotes,nv,velocity);
  }
}


void myNoteOff(uint8_t channel, uint8_t note, uint8_t velocity)
{
  int8_t av = allocFind(note); // try to find voice that's playing this note  

  if (av >= 0)        // there is one
    pv[av].endNote(); // begin its release process
      
}


//=============================================================
void loop() 
{
  if (Serial.available())
  {
    switch(Serial.read())
    {
      case '!':
        delay(2000);
        WRITE_RESTART(0x5FA0004);
        break;

      default:
        break;
    }
  }

  //*
  while (usbMIDI.read()) // do all MIDI actions
    ;
  //*/
  
  for (uint8_t i=0;i<COUNT_OF(pv);i++)
  {
    if (allocated[i] != UNUSED && !pv[i].isPlaying()) // allocated, but now silent:
      deAllocVoice(allocated[i]); // free it up for re-use
  }  

#if 0 // defined(GDB_IS_ENABLED)
  static uint32_t next = 0;
  
  if (millis() > next)
  {
    for (int i =0;i<8;i++)
    {
      Serial.printf("%3d, %d, %d: ",allocated[i],pv[i].playState,pv[i].fileState);
    }
    Serial.printf("; ocnt=%d, myOcnt=%d\n",ocnt,myOcnt);
    next = millis() + 250;
  }
#endif // defined(GDB_IS_ENABLED)
}


//=============================================================
void prepNote(uint8_t nv,const char* pathBuffer)
{
  if (nv >= allNotes.lowest 
   && nv  - allNotes.lowest < NOTES)  // note is in range
  {
      nv -= allNotes.lowest; // convert MIDI note to array index
      // fill in data for this layer of this note
      allNotes.notes[nv].layers[layer] = {.threshold = threshold, .maxGain = gain};
      allNotes.notes[nv].layers[layer].pload.preLoad(pathBuffer, AudioBuffer::inExt, STARTLEN);
  }
}


//=============================================================
char pathBuffer[50]="/"; // wild guess as to length
size_t pathOff = 1;
//=============================================================
/*
 * Our filenames are like Eb4-97-127.wav
 * The velocity ranges aren't needed, because they're in folders
 * named "loud", "med" and "soft", but we can translate the first
 * few characters to a MIDI note number.
 */
void checkFile(File& entry)
{
  const char* fn = entry.name();
  size_t nlen=strlen(fn);

  strcpy(pathBuffer+pathOff,fn);
  if (entry.isDirectory()) // change the thresholds etc
  {
    switch (*fn)
    {
      case 'l': // loud
        threshold = thresholds::FF;
        layer = 2;
        gain = 1.0f;
        break;
        
      case 'm': // med
        threshold = thresholds::MF;
        layer = 1;
        gain = 0.6f;
        break;
        
      case 's': // soft
        threshold = thresholds::PP;
        layer = 0;
        gain = 0.3f;
        break;        
    }
  }
  
  if (nlen >= 4)
  {
    if (0==strcmp(".wav",fn+nlen-4))
    {
      // C0 is MIDI note 12: go from there
      static const char* notes="C D EF G A B";
      const char* fnd = strchr(notes,fn[0]);
      if (nullptr != fnd)
      {
        int nv = fnd - notes + 12; // C0
        fnd = fn+1;
        if ('b' == *fnd) // flat?
        {
          nv--;   // down a semi
          fnd++;  // skip the character
        }
        nv += (*fnd - '0')*12; // add in the octave

        prepNote(nv,pathBuffer);        
      }
    }
  }
}

//=============================================================
void printDirectory(File dir, int numSpaces) 
{
  strcpy(pathBuffer+pathOff,dir.name());
  pathOff = strlen(pathBuffer)+1;
  pathBuffer[pathOff-1] = '/'; // zaps the terminator!
  
   while(true) 
   {
     File entry = dir.openNextFile();
     if (!entry) 
       break;
     checkFile(entry);
     if (entry.isDirectory()) 
       printDirectory(entry, numSpaces+2);
     entry.close();
   }
   pathOff -= strlen(dir.name()) + 1;
}
