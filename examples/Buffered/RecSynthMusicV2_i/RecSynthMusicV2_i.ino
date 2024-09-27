// Implement a 16 note polyphonic midi player  :-)
// Play the music, while recording to SD
// Then play it back from the SD
//
// Music data is read from memory.  The "Miditones" program is used to
// convert from a MIDI file to this compact format.
//
// This example code is in the public domain.
 
#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>

#include "RecSynthMusic.h"

volatile unsigned char *sp = score;

#define AMPLITUDE (0.95) // recording, so keep signal "hot"

// Create 16 waveforms, one for each MIDI channel
AudioSynthWaveform sine0, sine1, sine2, sine3;
AudioSynthWaveform sine4, sine5, sine6, sine7;
AudioSynthWaveform sine8, sine9, sine10, sine11;
AudioSynthWaveform sine12, sine13, sine14, sine15;
AudioSynthWaveform *waves[16] = {
  &sine0, &sine1, &sine2, &sine3,
  &sine4, &sine5, &sine6, &sine7,
  &sine8, &sine9, &sine10, &sine11,
  &sine12, &sine13, &sine14, &sine15
};

// allocate a wave type to each channel.
// The types used and their order is purely arbitrary.
short wave_type[16] = {
  WAVEFORM_SINE,
  WAVEFORM_SQUARE,
  WAVEFORM_SAWTOOTH,
  WAVEFORM_TRIANGLE,
  WAVEFORM_SINE,
  WAVEFORM_SQUARE,
  WAVEFORM_SAWTOOTH,
  WAVEFORM_TRIANGLE,
  WAVEFORM_SINE,
  WAVEFORM_SQUARE,
  WAVEFORM_SAWTOOTH,
  WAVEFORM_TRIANGLE,
  WAVEFORM_SINE,
  WAVEFORM_SQUARE,
  WAVEFORM_SAWTOOTH,
  WAVEFORM_TRIANGLE
};

// Each waveform will be shaped by an envelope
AudioEffectEnvelope env0, env1, env2, env3;
AudioEffectEnvelope env4, env5, env6, env7;
AudioEffectEnvelope env8, env9, env10, env11;
AudioEffectEnvelope env12, env13, env14, env15;
AudioEffectEnvelope *envs[16] = {
  &env0, &env1, &env2, &env3,
  &env4, &env5, &env6, &env7,
  &env8, &env9, &env10, &env11,
  &env12, &env13, &env14, &env15
};

// Route each waveform through its own envelope effect
AudioConnection patchCord01(sine0, env0);
AudioConnection patchCord02(sine1, env1);
AudioConnection patchCord03(sine2, env2);
AudioConnection patchCord04(sine3, env3);
AudioConnection patchCord05(sine4, env4);
AudioConnection patchCord06(sine5, env5);
AudioConnection patchCord07(sine6, env6);
AudioConnection patchCord08(sine7, env7);
AudioConnection patchCord09(sine8, env8);
AudioConnection patchCord10(sine9, env9);
AudioConnection patchCord11(sine10, env10);
AudioConnection patchCord12(sine11, env11);
AudioConnection patchCord13(sine12, env12);
AudioConnection patchCord14(sine13, env13);
AudioConnection patchCord15(sine14, env14);
AudioConnection patchCord16(sine15, env15);

// Four mixers are needed to handle 16 channels of music
AudioMixer4     mixer1;
AudioMixer4     mixer2;
AudioMixer4     mixer3;
AudioMixer4     mixer4;

// Mix the 16 channels down to 4 audio streams
AudioConnection patchCord17(env0, 0, mixer1, 0);
AudioConnection patchCord18(env1, 0, mixer1, 1);
AudioConnection patchCord19(env2, 0, mixer1, 2);
AudioConnection patchCord20(env3, 0, mixer1, 3);
AudioConnection patchCord21(env4, 0, mixer2, 0);
AudioConnection patchCord22(env5, 0, mixer2, 1);
AudioConnection patchCord23(env6, 0, mixer2, 2);
AudioConnection patchCord24(env7, 0, mixer2, 3);
AudioConnection patchCord25(env8, 0, mixer3, 0);
AudioConnection patchCord26(env9, 0, mixer3, 1);
AudioConnection patchCord27(env10, 0, mixer3, 2);
AudioConnection patchCord28(env11, 0, mixer3, 3);
AudioConnection patchCord29(env12, 0, mixer4, 0);
AudioConnection patchCord30(env13, 0, mixer4, 1);
AudioConnection patchCord31(env14, 0, mixer4, 2);
AudioConnection patchCord32(env15, 0, mixer4, 3);

// Now create 2 mixers for the main output
AudioMixer4     mixerLeft;
AudioMixer4     mixerRight;
AudioOutputI2S  audioOut;

// Mix all channels to both the outputs
AudioConnection patchCord33(mixer1, 0, mixerLeft, 0);
AudioConnection patchCord34(mixer2, 0, mixerLeft, 1);
AudioConnection patchCord35(mixer3, 0, mixerLeft, 2);
AudioConnection patchCord36(mixer4, 0, mixerLeft, 3);
AudioConnection patchCord37(mixer1, 0, mixerRight, 0);
AudioConnection patchCord38(mixer2, 0, mixerRight, 1);
AudioConnection patchCord39(mixer3, 0, mixerRight, 2);
AudioConnection patchCord40(mixer4, 0, mixerRight, 3);
AudioConnection patchCord41(mixerLeft, 0, audioOut, 0);
AudioConnection patchCord42(mixerRight, 0, audioOut, 1);

AudioControlSGTL5000 codec;

//======================================================================================================
// Extras for recording and playback
// We'll re-patch these:
AudioConnection* ppcs[] = {
  &patchCord17,&patchCord18,&patchCord19,
  &patchCord20,&patchCord21,&patchCord22,&patchCord23,&patchCord24,
  &patchCord25,&patchCord26,&patchCord27,&patchCord28,&patchCord29,
  &patchCord30,&patchCord31,&patchCord32
};

// these stay in place for recording:
AudioConnection* rpcs[16];

AudioRecordWAVquad rec4;
AudioRecordWAVhex  rec6;
AudioRecordWAVoct  rec8;

AudioPlayWAVquad play4;
AudioPlayWAVhex  play6;
AudioPlayWAVoct  play8;

AudioMixer4* mixers[] = {&mixer1,&mixer2,&mixer3,&mixer4};

//------------------------------------------------------------------------------------------------------
// connect all envelope outputs to a record object input, using 
// newly-created AudioConnection objects
void mkRecPcs(void)
{
  int n=0;

  for (int i = 0;i<4;i++,n++) rpcs[n] = new AudioConnection(*envs[n],0,rec4,i);
  for (int i = 0;i<5;i++,n++) rpcs[n] = new AudioConnection(*envs[n],0,rec6,i); // don't use channel 5
  for (int i = 0;i<7;i++,n++) rpcs[n] = new AudioConnection(*envs[n],0,rec8,i); // don't use channel 7
}


//------------------------------------------------------------------------------------------------------
// mixer inputs can come from the envelopes, or
// from the playback objects
void rePatch(bool forPlayback)
{
  int n = 0;
  
  for (int i=0;i<16;i++) ppcs[i]->disconnect();
  
  if (forPlayback)
  {
    for (int i = 0;i<4;i++,n++) ppcs[n]->connect(play4,i,*mixers[n/4],n&3);
    for (int i = 0;i<5;i++,n++) ppcs[n]->connect(play6,i,*mixers[n/4],n&3);
    for (int i = 0;i<7;i++,n++) ppcs[n]->connect(play8,i,*mixers[n/4],n&3);  
  }
  else
  {
    for (int j = 0;j<4;j++)
      for (int i = 0;i<4;i++,n++) 
        ppcs[n]->connect(*envs[n],0,*mixers[j],i);    
  }
}

void setLevels(void)
{
  // avoid overloading signal chain, while allowing
  // waveforms to be full amplitude for recording
  for (int j = 0;j<4;j++)
    for (int i = 0;i<4;i++) 
      mixers[j]->gain(i,0.2f);    
}


//------------------------------------------------------------------------------------------------------
// create playback and record buffers, depending on
// whether we have EXTMEM or not
void createBuffers(void)
{
  AudioBuffer::bufType bufMem = AudioBuffer::inHeap;
  size_t sz = 4096;
  
#if defined(ARDUINO_TEENSY41)  // only Teensy 4.1 supports EXTMEM
  extern uint8_t external_psram_size;
  if (external_psram_size > 0) // we have PSRAM
  {
     bufMem = AudioBuffer::inExt;
     sz = 8192;
     Serial.println("Buffers in PSRAM");
  }
  else
#endif // defined(ARDUINO_TEENSY41)  
     Serial.println("Buffers in heap");

  // we could de-allocate the record buffers
  // and create the play ones if we were
  // reslly short of RAM...
  rec4.createBuffer(4*2*sz,bufMem);
  rec6.createBuffer(6*2*sz,bufMem);
  rec8.createBuffer(8*2*sz,bufMem);

  play4.createBuffer(4*sz,bufMem);
  play6.createBuffer(6*sz,bufMem);
  play8.createBuffer(8*sz,bufMem);
}

// Initial value of the volume control
int volume = 50;

//------------------------------------------------------------------------------------------------------
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


//======================================================================================================
void printInstrumentPlay(AudioPlayWAVbuffered& o, const char* nam)
{
  Serial.printf("%s: low-water: %u/%u; worst read time: %uus; updates: %u\n",
                nam,
                o.bufferAvail.getLowest(), o.bufSize, 
                o.readMicros.getHighest(), 
                o.bufferAvail.getUpdates());
  o.bufferAvail.reset(); 
  o.readMicros.reset(); 
}

//======================================================================================================
void printInstrumentRec(AudioRecordWAVbuffered& o, const char* nam)
{
  Serial.printf("%s: high-water: %u/%u; worst write time: %uus; updates: %u\n",
                nam,
                o.bufferAvail.getHighest(), o.bufSize,
                o.readMicros.getHighest(), 
                o.bufferAvail.getUpdates());
  o.bufferAvail.reset(); 
  o.readMicros.reset(); 
}

//======================================================================================================
void setup()
{
  Serial.begin(115200);
  //while (!Serial) ; // wait for Arduino Serial Monitor
  delay(200);

  // change this if your SD card is somewhere else:
  // may not be very useful for multi-channel
  // use, though
  while (!SD.begin(BUILTIN_SDCARD))
  {
    Serial.println("SD wait...");
    delay(250);
  } 
  
// http://gcc.gnu.org/onlinedocs/cpp/Standard-Predefined-Macros.html
  Serial.print("Begin ");
  Serial.println(__FILE__);
  
  // Proc = 12 (13),  Mem = 2 (8)
  // Audio connections require memory to work.
  // The memory usage code indicates that ~30 is the maximum
  // so give it plenty just to be sure.
  AudioMemory(50);
  
  findSGTL5000(codec);  // find codec, if I²C address has been changed
  mkRecPcs();           // make record patchCord connections
  createBuffers();      // allocate buffer memory
  setLevels();          // set mixer levels

  // reduce the gain on some channels, so half of the channels
  // are "positioned" to the left, half to the right, but all
  // are heard at least partially on both ears
  mixerLeft.gain(1, 0.36);
  mixerLeft.gain(3, 0.36);
  mixerRight.gain(0, 0.36);
  mixerRight.gain(2, 0.36);

  // set envelope parameters, for pleasing sound :-)
  for (int i=0; i<16; i++) {
    envs[i]->attack(9.2);
    envs[i]->hold(2.1);
    envs[i]->decay(31.4);
    envs[i]->sustain(0.6);
    envs[i]->release(84.5);
    // uncomment these to hear without envelope effects
    //envs[i]->attack(0.0);
    //envs[i]->hold(0.0);
    //envs[i]->decay(0.0);
    //envs[i]->release(0.0);
  }

  Serial.println("setup done");
  
  // Initialize processor and memory measurements
  AudioProcessorUsageMaxReset();
  AudioMemoryUsageMaxReset();

}


//======================================================================================================
const char* names[] = {"wm_tell4.wav","wm_tell6.wav","wm_tell8.wav"};
unsigned long last_time = millis();
enum state_e {boot,synth,playback,playing,peace} state;
//-------------------------------------------------
/*
 * Use an IntervalTimer to ensure synth playing is not affected by
 * SD card buffering. The old scheme used delay() to time the notes
 * and rests, but this can end up being longer than requested if 
 * an SD write is pending in EventResponder.
 * 
 * For this scheme we run the synth engine, starting and stopping notes,
 * until we get to a point where we want the note to play for a while.
 * The synth engine sets its IntervalTimer to interrupt at this time, and
 * when that occurs it continues up to the next "delay point", or the end 
 * of the song.
 * 
 * We actually use a slightly convoluted method, because IntervalTimer may 
 * be set to a high priority, but we want the synth engine to execute at
 * a higher priority than foreground / yield() code, but at a lower priority
 * than the (already low!) audio engine.
 * 
 * Hence we set up an unused interrupt at super-low priority, and all the
 * IntervalTimer does is to .end() itself (so it doesn't re-trigger) and 
 * trigger the synth interrupt. The latter then runs until a delay is needed,
 * sets up the IntervalTimer once again with the new delay, and exits.
 */
#define IRQ_SYNTH IRQ_Reserved2
void setupSynthInterrupt()
{
  attachInterruptVector(IRQ_SYNTH, runSynthIRQ);
  NVIC_SET_PRIORITY(IRQ_SYNTH, 224); // lower priority than audio
  NVIC_ENABLE_IRQ(IRQ_SYNTH);
}

IntervalTimer synthTimer;
volatile sr_enum synthReturn = sr_idle;
void synthIntvl()
{
  synthTimer.end(); // synth IRQ will .begin() it again
  NVIC_SET_PENDING(IRQ_SYNTH);
}

void runSynthIRQ()
{
  do 
  {
    synthReturn = runSynth();
  } while (synthReturn < sr_done);
  asm("DSB");
}

#define STATS_INTERVAL 500
//-------------------------------------------------
void loop()
{ 
  // output statistics about how the recording process is going 
  static uint32_t nextStats = 0;
  if (rec4.isRecording()) // we'll assume all recorder obejects are recording
  {
    if (millis() >= nextStats)
    {
      nextStats = millis() + STATS_INTERVAL;
      
      if (rec4.isRecording())
        printInstrumentRec(rec4,"rec4");
      
      if (rec6.isRecording())
        printInstrumentRec(rec6,"rec6");
      
      if (rec8.isRecording())
        printInstrumentRec(rec8,"rec8");    

      Serial.println();          
    }
  }
  
  if (play4.isPlaying()) // we'll assume all recorder obejects are recording
  {
    if (millis() >= nextStats)
    {
      nextStats = millis() + STATS_INTERVAL;
      
      if (play4.isPlaying())
        printInstrumentPlay(play4,"play4");
      
      if (play6.isPlaying())
        printInstrumentPlay(play6,"play6");
      
      if (play8.isPlaying())
        printInstrumentPlay(play8,"play8");  

      Serial.println();          
    }
  }
  
  if (boot == state && !rec4.isRecording())
  {
    // prepare to record
    rec4.cueSD(names[0]);
    rec6.cueSD(names[1]);
    rec8.cueSD(names[2]);

    // start in sync
    rec4.record();
    rec6.record();
    rec8.record();

    // if something went wrong,
    // stall while printing dots
    while (rec4.positionMillis() == 0)
    {
      Serial.print('.');
      delay(250);
    }
    
    Serial.println("Recording...");
    setupSynthInterrupt();
    synthTimer.begin(synthIntvl,5);
    state = synth;
  }
  
  // check audio engine statistics
  if (peace != state) 
  {
    if(millis() - last_time >= 5000) 
    {
      Serial.print("Proc = ");
      Serial.print(AudioProcessorUsage());
      Serial.print(" (");    
      Serial.print(AudioProcessorUsageMax());
      Serial.print("),  Mem = ");
      Serial.print(AudioMemoryUsage());
      Serial.print(" (");    
      Serial.print(AudioMemoryUsageMax());
      Serial.println(")");
      last_time = millis();
    }
  }

  // Volume control
  //  uncomment if you have a volume pot soldered to your audio shield
  /*
  int n = analogRead(15);
  if (n != volume) {
    volume = n;
    codec.volume((float)n / 1023);
  }
  */

  if (synth == state)
  {
    // nothing much to do here but wait for 
    // synth to stop
    if (sr_cmd == synthReturn)
      return;    
  }
  
  if (synth == state && sr_done == synthReturn)
  {
    // ensure the IntervalTimer is stopped
    synthTimer.end(); 
    
    // pause recordings: just sets an internal
    // flag so (probably) we pause in sync
    rec4.pause();
    rec6.pause();
    rec8.pause();

    uint32_t startStop = millis();
    // stop recordings: these take time because
    // remaining audio is flushed. If we just
    // stopped then the files would potentially
    // be different lengths
    rec4.stop();
    rec6.stop();
    rec8.stop();
    Serial.printf("Buffer flushes took %lums\n",millis() - startStop);
    
    state = playback;
  }

  // start playback from SD card
  if (playback == state && !play4.isPlaying())
  {
    rePatch(true);

    play4.cueSD(names[0]);
    play6.cueSD(names[1]);
    play8.cueSD(names[2]);

    play4.play();
    play6.play();
    play8.play();

    Serial.println("Playback...");
    state = playing;
  }

  // stop checking statistics once playback completes
  if (playing == state && !play4.isPlaying())
  {
    state = peace;
  }
  delay(3);
}

//-------------------------------------------------
sr_enum runSynth()
{
    unsigned char c,opcode,chan;
    unsigned long d_time;
    static bool stopped = false;

    // read the next command from the table
    c = *sp++;
    opcode = c & 0xF0;
    chan = c & 0x0F;
  
    if (c < 0x80) // not a command, a delay time
    {
      sr_enum rv = sr_cmd;
      // Delay
      d_time = (c << 8) | *sp++;
      //delay(d_time);
      if (synthTimer.begin(synthIntvl,d_time*1000UL))
        rv = sr_timeout;
      return rv;
    }
    
    if (c == CMD_STOP) 
    {
      sp--; // force re-read of stop command from now on
      
      if (!stopped)
      {
        for (chan=0; chan<10; chan++) 
          envs[chan]->noteOff();
  
        stopped = true;
  
        // allow envelopes to decay
        synthTimer.begin(synthIntvl,200*1000UL);
        
        return sr_timeout;
      }
      else
      {    
        for (chan=0; chan<10; chan++)
          waves[chan]->amplitude(0);
  
        return sr_done; // no more to do
      }
    }
  
    // It is a command
    
    // Stop the note on 'chan'
    if (opcode == CMD_STOPNOTE) 
    {
      envs[chan]->noteOff();
      return sr_cmd;
    }
    
    // Play the note on 'chan'
    if (opcode == CMD_PLAYNOTE) 
    {
      unsigned char note = *sp++;
      unsigned char velocity = *sp++;
      AudioNoInterrupts();
      waves[chan]->begin(AMPLITUDE * velocity2amplitude[velocity-1],
                         tune_frequencies2_PGM[note],
                         wave_type[chan]);
      envs[chan]->noteOn();
      AudioInterrupts();
      return sr_cmd;
    }
  
    // replay the tune
    if (opcode == CMD_RESTART) 
    {
      sp = score;
      return sr_idle;
    }

    return sr_idle;
}
