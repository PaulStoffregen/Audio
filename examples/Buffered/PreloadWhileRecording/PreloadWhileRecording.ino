/*
 * Demonstration of simple looper application.
 * Based on this post: https://forum.pjrc.com/threads/70963-Yet-Another-File-Player-(and-recorder)?p=329731&viewfull=1#post329731
 * 
 * For a looper, we want to be able to (pre-load or) play a file from the start while it's
 * still recording. To do this, an interim WAV header is needed so the playback has enough
 * information to get going. However, we also need to adjust that information once recording
 * has stopped, as otherwise the playback will stop when the interim length is reached, not
 * the final length. Two new functions are added to achieve this, see cases 3 and 20 in 
 * loop() below.
 * 
 * Two sets of waveform, envelope and playback objects are used, so we can start a new note
 * or playback while the other one is still playing. This gives us fairly seamless joins.
 */
#include <Audio.h>

// GUItool: begin automatically generated code
AudioSynthWaveform       wav1; //xy=202,176
AudioSynthWaveform       wav2; //xy=207,214
AudioPlayWAVstereo       playWAVstereo1; //xy=265,260
AudioPlayWAVstereo       playWAVstereo2; //xy=272,295
AudioEffectEnvelope      env1; //xy=362,176
AudioEffectEnvelope      env2; //xy=367,215
AudioMixer4              recMixer; //xy=559,166
AudioMixer4              mixer;         //xy=565,243
AudioRecordWAVmono       recordWAVmono1; //xy=751,165
AudioOutputI2S           i2sOut;           //xy=788,260

AudioConnection          patchCord1(wav1, env1);
AudioConnection          patchCord2(wav2, env2);
AudioConnection          patchCord3(playWAVstereo1, 0, mixer, 2);
AudioConnection          patchCord4(playWAVstereo2, 0, mixer, 3);
AudioConnection          patchCord5(env1, 0, mixer, 0);
AudioConnection          patchCord6(env1, 0, recMixer, 0);
AudioConnection          patchCord7(env2, 0, mixer, 1);
AudioConnection          patchCord8(env2, 0, recMixer, 1);
AudioConnection          patchCord9(recMixer, recordWAVmono1);
AudioConnection          patchCord10(mixer, 0, i2sOut, 0);
AudioConnection          patchCord11(mixer, 0, i2sOut, 1);

AudioControlSGTL5000     audioShield;    //xy=775,304
// GUItool: end automatically generated code


AudioPreload preLoad1;

uint32_t next;
void setup() 
{
  AudioMemory(10);

  while (!Serial)
    ;

  if (CrashReport)
    Serial.print(CrashReport);    

  while (!SD.begin(BUILTIN_SDCARD))
  {
    Serial.println("SD card problem!");
    delay(500);
  }

  audioShield.setAddress(HIGH);
  audioShield.enable();
  audioShield.volume(0.1f);

  // 16k @ 44.1kHz is ~186ms
  playWAVstereo1.createBuffer(16384,AudioBuffer::inHeap);
  playWAVstereo2.createBuffer(16384,AudioBuffer::inHeap);
  recordWAVmono1.createBuffer(16384,AudioBuffer::inHeap);
  preLoad1.createBuffer(16384,AudioBuffer::inHeap);

  
  next = millis() / 1000;
  next = (next+1)*1000 - 1;

  // Standard envelope is 10.5+2.5+35.0+<sustain time>+300.0 ms
  // Total of 348ms plus however long we sustain for
  // Say 100ms, then trigger a new note every 250ms means
  // release is just done by the time we want to re-use a voice
}

/*
C  523.25
B 493.88
Bb  466.16
A 440
Ab  415.3
G 392
F#  369.99
F 349.23
E 329.63
Eb  311.13
D 293.66
C#  277.18
C 261.63
*/
float Cscale[] = {261.63f, 293.66f, 329.63f, 349.23f, 392.0f, 440.0f, 493.88f, 523.25f};
int state;
const char* scaleFile = "scale.wav";

void loop() 
{
  if (millis() >= next)
  {
    Serial.printf("%lu: %d\n",millis(),state);
    switch (state)
    {
      default:
      case 0:
        next = millis() + 1;
        state = 1;
        break;

      //================================================================================
      // synthesize a C scale: each note starts
      // 250ms after the previous but the last note
      // release ends 448ms after its attack started
      case 1:
        Serial.println("Synthesize a scale of C major, and record it");
        recordWAVmono1.record(scaleFile);
      case 5:
      case 7:
        wav1.begin(0.5f,Cscale[state - 1],WAVEFORM_SINE);
        env1.noteOn();
        next = millis() + 148;
        state += 10;
        break; 

      // Near-duplicate, but need to start wave on time
      case 3: // E: have enough for preload
        wav1.begin(0.5f,Cscale[state - 1],WAVEFORM_SINE);
        env1.noteOn();
        next = millis() + 148;
        state += 10;

        // Call new function to allow pre-load to work:
        Serial.printf("Header at %d bytes\n",recordWAVmono1.writeCurrentHeader()); 
        
        Serial.printf("Preload returned %d\n",preLoad1.preLoad(scaleFile));       
        break; 
        
      case 2:
      case 4:
      case 6:
      case 8:
        wav2.begin(0.5f,Cscale[state - 1],WAVEFORM_SINE);
        env2.noteOn();
        next = millis() + 148;
        state += 10;
        break; 

      case 11:        
      case 13:        
      case 15:        
      case 17:        
        env1.noteOff();
        next = millis() + 102;
        state -=9;
        break;
        
      case 12:        
      case 14:        
      case 16:        
      case 18:        
        env2.noteOff(); // sound will end in 300ms
        next = millis() + 102;
        state -=9;
        break;

      //================================================================================
      // Begin the loop
      case 9:
        Serial.println("Start playback of recorded scale");
        playWAVstereo2.play(preLoad1); // start playback loop - we're still recording
        next = millis() + 250; // allow time for last release
        state = 20;
        break;

      case 20:
        recordWAVmono1.stop();    // last release is finished - stop recording
        
        // Call new function to ensure file plays to the (new) end point:
        Serial.printf("Header adjusted by %lu bytes\n",playWAVstereo2.adjustHeaderInfo());
        
        next = millis() + 7*250;  // 7 notes to play back before playing again
        state++;
    //state = 99;
        break;
        
      //================================================================================
      // Continue the loop
      case 21:
        playWAVstereo1.play(preLoad1); // re-start playback loop - previous playback release still playing
        next = millis() + 8*250;  // 8 notes to play back before playing again
        state++;
        break;
        
      case 22:
        playWAVstereo2.play(preLoad1); // re-start playback loop - previous playback release still playing
        next = millis() + 8*250;  // 8 notes to play back before playing again
        state--;
        break;

      case 99:
        next = millis() + 4*250;
        break;              
    }
  }
}
