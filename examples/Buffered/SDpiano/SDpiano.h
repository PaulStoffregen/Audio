//=============================================================
#define STARTLEN  (60*512) // bytes to pre-buffer
#define STARTTIME ((float) STARTLEN / 4.0f / (float) AUDIO_SAMPLE_RATE * 1000.0f )
#define PATHLEN 27        // length of file path for one sample
#define NOTES   88
#define LAYERS   3

#define COUNT_OF(x) ((sizeof x) / (sizeof x[0]))

struct thresholds
{
  enum {PP=48,MF=96,FF=127};
};

struct sample_t
{
  char* fname;          // file with the whole sample in it
  unsigned int* left,*right;  // pre-buffered start of the sample
  uint8_t threshold;    // use this sample <= this velocity
  float maxGain;        // gain to apply when at threshold velocity
};

struct pianote_t
{
  sample_t layers[LAYERS];
};

struct allNotes_t
{
  uint8_t lowest;   // MIDI number of lowest note in sample set
  uint8_t maxIdx;   // maximum allowed index in notes[]
  float startFrom;  // length of pre-load buffers, in miliseconds
  pianote_t notes[NOTES];
};
// ----5---10---15---20---25-27
// /piano/loud/Bb7-97-127.wav

/*
 * Piano voice class based on this topology:
 * 
      // GUItool: begin automatically generated code
      AudioPlayMemory          playMemL;       //xy=186,227
      AudioPlayMemory          playMemR;  //xy=196,342
      AudioPlayWAVstereo       playWAVstereo1; //xy=220,284
      AudioMixer4              mixerL;         //xy=408,242
      AudioMixer4              mixerR;         //xy=412,329
      AudioEffectEnvelope      envL;      //xy=546,242
      AudioEffectEnvelope      envR; //xy=551,327
      AudioOutputI2S           i2sOut;           //xy=948,268
      
      AudioConnection          patchCord1(playMemL, 0, mixerL, 0);
      AudioConnection          patchCord2(playMemR, 0, mixerR, 0);
      AudioConnection          patchCord3(playWAVstereo1, 0, mixerL, 1);
      AudioConnection          patchCord4(playWAVstereo1, 1, mixerR, 1);
      AudioConnection          patchCord5(mixerL, envL);
      AudioConnection          patchCord6(mixerR, envR);
      AudioConnection          patchCord7(envL, 0, i2sOut, 0);
      AudioConnection          patchCord8(envR, 0, i2sOut, 1);
      
      AudioControlSGTL5000     sgtl5000;     //xy=936,328
      // GUItool: end automatically generated code

 */
extern const float velocity2amplitude[];
class PianoVoice : public AudioStream
{
    int _nv,_lv;
    allNotes_t* _pNotes;
    AudioEffectEnvelope* _env[2];

    // deal with real-time updates to internal state
    void update(void) 
    {
      switch (fileState)
      {
        default:
          break;
          
        case contDone: // filesystem is cued up and ready
          if ((sample == playState   // sample was playing...
            || ending == playState)  // ...or playback is ending soon
           && !playMemL.isPlaying()) // and pre-load buffer has just finished
          {
            if (playWAVstereo1.play()) // can we play the file?
              fileState = file;        // yes, good
            else
            {                          // no, not so good
              if (playState != ending) // unless already ending  
                endNote('E');          // start the envelope release
              fileState = ending;      // only do this once                
            }
          }
          break;
      }

      switch (playState)
      {
        default:
          break;

        case sample:
          if (file == fileState)
            playState = file;
          break;
          
        case ending: // envelope release in progress
        case silent: // active but silent: must be waiting for (now pointless) pre-load
          if (!envL.isActive() && !envR.isActive()) // it's done
          {
            playMemL.stop();  
            playMemR.stop();  
            playState = silent;
            if (fileState != contStarted) // filesystem could be cueing up: don't confuse it!
            {
              playWAVstereo1.stop(1);  // not cueing, safe to stop; from within interrupt
              fileState = silent;     // done with filesystem
              active = false; // no need to waste CPU on this
            }
          }
          break;
      }
    }

  public:
    PianoVoice() : AudioStream(0,NULL), 
    _nv(0), _lv(0), _pNotes(NULL),
    _env{&envL,&envR},
    patchCord1(playMemL, 0, mixerL, 0),
    patchCord2(playMemR, 0, mixerR, 0),
    patchCord3(playWAVstereo1, 0, mixerL, 1),
    patchCord4(playWAVstereo1, 1, mixerR, 1), 
    patchCord5(mixerL, envL),
    patchCord6(mixerR, envR),
    opL(NULL), opR(NULL),
    playState(silent), fileState(silent)
    {
      opL = opR = NULL;
      // set up envelopes - only used to release note
      for (uint8_t i=0;i<COUNT_OF(_env);i++)
      {
        _env[i]->delay(0.0f);
        _env[i]->attack(0.0f);
        _env[i]->hold(0.0f);
        _env[i]->decay(0.0f);
        _env[i]->sustain(1.0f);
        _env[i]->release(50.0f);
        _env[i]->releaseNoteOn(0.0f);
      }
    }
    
    AudioPlayMemory          playMemL;       //xy=186,227
    AudioPlayMemory          playMemR;  //xy=196,342
    AudioPlayWAVstereo       playWAVstereo1; //xy=220,284
    AudioMixer4              mixerL;         //xy=408,242
    AudioMixer4              mixerR;         //xy=412,329
    AudioEffectEnvelope      envL;      //xy=546,242
    AudioEffectEnvelope      envR; //xy=551,327
          
    AudioConnection          patchCord1;
    AudioConnection          patchCord2;
    AudioConnection          patchCord3;
    AudioConnection          patchCord4; 
    AudioConnection          patchCord5;
    AudioConnection          patchCord6; 

    AudioConnection* opL,*opR; // create these when application requests connections
  
    volatile enum {silent,sample,contStarted,contDone,file,ending} playState, fileState;

//==================================================================================
    void connect(AudioStream* ipL,int li,AudioStream* ipR,int ri)
    {
      if (nullptr == opL)
        opL = new AudioConnection(envL,0,*ipL,li);
      else
      {
        opL->disconnect();
        opL->connect(envL,0,*ipL,li);
      }
      
      if (nullptr == opR)
        opR = new AudioConnection(envR,0,*ipR,ri);
      else
      {
        opR->disconnect();
        opR->connect(envR,0,*ipR,ri);
      }
    }

//==================================================================================
    // start playing a note: play from the pre-loaded memory,
    // and start the envelopes
    void startNote(allNotes_t& allNotes,int nv, int vel)
    {
      int lv;
      float ampl;
      
      _pNotes = &allNotes;
      if (nv > allNotes.maxIdx) 
        nv = allNotes.maxIdx / 2; // defensive
      _nv = nv;

      // defensive: ensure velocity is sane!
      if (vel <= 0)  vel = 1;
      if (vel > 127) vel = 127;

      // find layer to use
      for (lv=0;lv<LAYERS;lv++)
        if (allNotes.notes[nv].layers[lv].threshold >= vel)
          break;
          
      _lv = lv;

      // set the amplitudes
      ampl = velocity2amplitude[vel-1];
      for (int i=0;i<2;i++)
      {
        mixerL.gain(i,ampl);
        mixerR.gain(i,ampl);
      }
      
      playMemL.play(allNotes.notes[nv].layers[lv].left); // isPlaying() becomes true
      playMemR.play(allNotes.notes[nv].layers[lv].right);
      playWAVstereo1.stop(); // could be re-triggering this voice while it's playing
      envL.noteOn();
      envR.noteOn();
      playState = fileState = sample; 
      active = true; // normally done by making a connection, but we don't have / need any
      // nearly done: call contNote() soon, after you've done all the startNote() calls
    }

//==================================================================================
    // Continue starting the note: this should be called as soon as practicable after
    // startNote(), certainly soon enough that there is time to buffer the SD playback
    // before the audio in the pre-load buffer has run out, otherwise you'll get a glitch!
    // Because of the pre-load this may take some time, so it makes two state changes in
    // case an update() or other interrupt occurs in the middle.
    bool contNote(void)
    {
      bool result = false; // didn't cue a note up
      if (sample == fileState && nullptr != _pNotes)
      {
        fileState = contStarted; // may get an interrupt between here...
        playWAVstereo1.cueSD(_pNotes->notes[_nv].layers[_lv].fname,_pNotes->startFrom);
        fileState = contDone;    // ...and here
        result = true;
      }
      return result;
    }

//==================================================================================
    // key released: note will take some time
    // to die away, then update() will stop it
    void endNote(char c='e')
    {
      playState = ending;
      envL.noteOff();
      envR.noteOff();
    }

//==================================================================================
    // allow user to adjust release time, really
    // the only thing we use the envelopes for
    void setRelease(float r)
    {
      envL.release(r);
      envR.release(r);
    }

//==================================================================================
    // return true if voice is currently playing a sound
    bool isPlaying(void)
    {
      return playState != silent;
    }
};
