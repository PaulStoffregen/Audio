/*
 * Demonstration of dynamic patchcords,
 * introduced with Teensyduino 1.57
 */

#include <Audio.h>


// GUItool: begin automatically generated code
AudioSynthWaveform       wav1;      //xy=245,515
AudioSynthWaveform       wav2; //xy=245,564
AudioMixer4              mixerL;         //xy=503,527
AudioRecordQueue         queue;         //xy=504,468
AudioMixer4              mixerR; //xy=507,600
AudioOutputI2S           i2sOut;           //xy=700,561
AudioConnection          patchCord1(wav1, queue);
AudioConnection          patchCord2(mixerL, 0, i2sOut, 0);
AudioConnection          patchCord3(mixerR, 0, i2sOut, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=692,616
// GUItool: end automatically generated code



void setup() {
  while (!Serial)
    ;
  Serial.println("Started");
  
  AudioMemory(10);

  // sgtl5000_1.setAddress(HIGH); // use if Audio Adaptor I²C address has been changed
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);

  wav1.begin(0.5,220.0,WAVEFORM_SINE);
  wav2.begin(0.5,330.0,WAVEFORM_SINE);

  queue.begin();
}

AudioConnection* pc[4] = {&patchCord2,&patchCord3}; // array to keep track of connection objects
uint32_t audioCycles; // count of audio engine cycles
bool audioCycled;

#define UPDATE_CYCLES 172 // change patching after this number of audio engine cycles
void loop() {
  static uint32_t nextCycleCount;
  while (queue.available() > 0)
  {
    audioCycles++;
    audioCycled = true;
    queue.readBuffer();
    queue.freeBuffer();
  }

  if (audioCycled)
  {
    static uint32_t nextConnect;
    static int state;
    
    audioCycled = false;

    if (audioCycles > nextConnect)
    {
      switch (state)
      {
        default: // don't let state machine run wild!
          Serial.println();
          state = 0;
          break;


        // delete any existing dynamic connections
        // DO NOT do this to static connections, e.g. those
        // imported from the design tool!
        case 0: 
          for (int i=2;i<4;i++)
          {
            if (nullptr != pc[i])
            {
              delete pc[i];
              pc[i] = nullptr;
            }
          }
          Serial.println("dynamic connections deleted");

          nextConnect = audioCycles + UPDATE_CYCLES;
          state++;
          break;


        // creating new connections
        case 1:
          pc[2] = new AudioConnection(wav1,mixerL);
          Serial.println("wav1 connection created");

          nextConnect = audioCycles + UPDATE_CYCLES;
          state++;
          break;

        case 2:      
          pc[3] = new AudioConnection(wav2,mixerR);
          Serial.println("wav2 connection created");        
          
          nextConnect = audioCycles + UPDATE_CYCLES;
          state++;
          break;


        // disconnecting existing connections
        case 3:
          pc[2]->disconnect();
          Serial.println("wav1 disconnected");

          nextConnect = audioCycles + UPDATE_CYCLES;
          state++;
          break; 
                   
        case 4:
          pc[3]->disconnect();
          Serial.println("wav2 disconnected");

          nextConnect = audioCycles + UPDATE_CYCLES;
          state++;
          break; 


        // re-making connections:
        // the original object and port is remembered
        case 5:
          pc[2]->connect();
          Serial.println("wav1 re-connected");

          nextConnect = audioCycles + UPDATE_CYCLES;
          state++;
          break; 
                   
        case 6:
          pc[3]->connect();
          Serial.println("wav2 re-connected");

          nextConnect = audioCycles + UPDATE_CYCLES;
          state++;
          break; 

          
        // swapping connections to different ports
        case 7:
          pc[2]->disconnect();
          pc[2]->connect(wav1,0,mixerR,1);
          Serial.println("wav1 re-connected to new destination");

          nextConnect = audioCycles + UPDATE_CYCLES;
          state++;
          break; 
                   
        case 8:
          pc[3]->disconnect();
          pc[3]->connect(wav2,0,mixerL,1);
          Serial.println("wav2 re-connected to new destination");

          nextConnect = audioCycles + UPDATE_CYCLES;
          state++;
          break; 


        // swapping connections of static objects
        case 9: 
          {
            static bool LtoL;

            // disconnect mixers from output
            pc[0]->disconnect();
            patchCord3.disconnect();

            if (LtoL)
            {
              // can refer to connections via the array...
              pc[0]->connect(mixerL,i2sOut);
              pc[1]->connect(mixerR,0,i2sOut,1);
              Serial.println("mixerL to i2sL");
            }
            else
            {
              // ...or direct reference to static objects
              patchCord2.connect(mixerR,i2sOut);
              patchCord3.connect(mixerL,0,i2sOut,1);
              Serial.println("mixerR to i2sL - swapped!");
            }
            
            LtoL = !LtoL; // swap back next time
          }

          // no time delay needed, will persist for whole 
          // state machine cycle
          state++;
          break;
      }                   
    }    
  }

#define AUDIO_COUNT_ENABLED false
  // Can enable this to see cycle count ticking up
  if (AUDIO_COUNT_ENABLED && audioCycles >= nextCycleCount)
  {
    nextCycleCount += 100; // about 290ms
    Serial.printf("%lu audio cycles\n",audioCycles);
  }
}
