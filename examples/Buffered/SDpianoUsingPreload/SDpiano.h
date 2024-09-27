//=============================================================
#define STARTLEN  (60*512) // bytes to pre-buffer
#define NOTES   88
#define LAYERS   3

#define COUNT_OF(x) ((sizeof x) / (sizeof x[0]))

struct thresholds
{
  enum {PP=48,MF=96,FF=127};
};

struct sample_t
{
  AudioPreload pload;   // preload class
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
  pianote_t notes[NOTES];
};
// ----5---10---15---20---25-27
// /piano/loud/Bb7-97-127.wav

/*
 * Piano voice class based on this topology:
 * 
    // GUItool: begin automatically generated code
    AudioPlayWAVstereo       playWAVstereo1; //xy=303,203
    AudioAmplifier           ampL;           //xy=501,159
    AudioAmplifier           ampR; //xy=503,246
    AudioEffectEnvelope      envL;      //xy=647,159
    AudioEffectEnvelope      envR; //xy=653,246
    AudioOutputI2S           i2sOut;         //xy=824,203
    
    AudioConnection          patchCord1(playWAVstereo1, 0, ampL, 0);
    AudioConnection          patchCord2(playWAVstereo1, 1, ampR, 0);
    AudioConnection          patchCord3(ampL, envL);
    AudioConnection          patchCord4(ampR, envR);
    AudioConnection          patchCord5(envL, 0, i2sOut, 0);
    AudioConnection          patchCord6(envR, 0, i2sOut, 1);
    
    AudioControlSGTL5000     sgtl5000;       //xy=812,263
    // GUItool: end automatically generated code
*/

extern const float velocity2amplitude[];
class PianoVoice
{
    int _nv,_lv;
    allNotes_t* _pNotes;
    AudioEffectEnvelope* _env[2];

  public:
    PianoVoice() : 
    _nv(0), _lv(0), _pNotes(NULL),
    _env{&envL,&envR},
    patchCord1(playWAVstereo1, 0, ampL, 0),
    patchCord2(playWAVstereo1, 1, ampR, 0), 
    patchCord3(ampL, envL),
    patchCord4(ampR, envR),
    opL(NULL), opR(NULL),
    playState(silent)
    {
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
    
    AudioPlayWAVstereo       playWAVstereo1; //xy=220,284
    AudioAmplifier           ampL;         //xy=408,242
    AudioAmplifier           ampR;         //xy=412,329
    AudioEffectEnvelope      envL;      //xy=546,242
    AudioEffectEnvelope      envR; //xy=551,327
          
    AudioConnection          patchCord1;
    AudioConnection          patchCord2; 
    AudioConnection          patchCord3;
    AudioConnection          patchCord4; 

    AudioConnection* opL,*opR; // create these when application requests connections
  
    volatile enum {silent,playing,ending} playState;

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
      ampL.gain(ampl);
      ampR.gain(ampl);
      
      playWAVstereo1.play(allNotes.notes[nv].layers[lv].pload); 
      envL.noteOn();
      envR.noteOn();
      playState = playing; 
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
    // checks to see if playing has stopped recently and 
    // if so sets the status and stops file playback
    // return true if voice is currently playing a sound
    bool isPlaying(void)
    {
      if (!envL.isActive())
      {
        if (silent != playState && playWAVstereo1.isPlaying())
          playWAVstereo1.stop();
        playState = silent;        
      }
      return silent != playState;
    }
};
