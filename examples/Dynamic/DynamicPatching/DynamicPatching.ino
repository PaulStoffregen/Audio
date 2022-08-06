/*
 * Demonstration of dynamic patchcords,
 * introduced with Teensyduino 1.57
 * 
 * Also demonstrates probing for SGTL5000 at both I²C addresses,
 * and use of the AudioRecordQueue object to synchronise to the
 * audio engine updates.
 */

#include <Audio.h>

// GUItool: begin automatically generated code
AudioSynthWaveform       wav1;      //xy=245,515
AudioSynthWaveform       wav2;      //xy=245,564
AudioMixer4              mixerL;    //xy=503,527
AudioRecordQueue         queue;     //xy=504,468
AudioMixer4              mixerR;    //xy=507,600
AudioOutputI2S           i2sOut;    //xy=700,561
AudioConnection          patchCord1(wav1, queue);
AudioConnection          patchCord2(mixerL, 0, i2sOut, 0);
AudioConnection          patchCord3(mixerR, 0, i2sOut, 1);
AudioControlSGTL5000     sgtl5000_1; //xy=692,616
// GUItool: end automatically generated code


/***************************************************************************/
void setup() 
{
  while (!Serial)
    ;
  Serial.println("Started");
  
  AudioMemory(10);

  // search for SGTL5000 at both I²C addresses
  for (int i=0;i<2;i++)
  {
    uint8_t levels[] {LOW,HIGH};
    
    sgtl5000_1.setAddress(levels[i]);
    sgtl5000_1.enable();
    if (sgtl5000_1.volume(0.5))
    {
      Serial.printf("SGTL5000 found at %s address\n",i?"HIGH":"LOW");
      break;
    }
  }

  // set up audio objects
  wav1.begin(0.5,220.0,WAVEFORM_SINE);
  wav2.begin(0.5,330.0,WAVEFORM_SINE);

  mixerL.gain(1,0.1);
  mixerR.gain(1,0.1);

  queue.begin();
}


/***************************************************************************/
AudioConnection* pc[4] = {&patchCord2,&patchCord3}; // array to keep track of connection objects
uint32_t audioUpdates; // count of audio engine updates
bool audioUpdated;

#define UPDATE_INTERVAL 172 // change patching after this number of audio engine updates

/***************************************************************************/
void loop() 
{
  static uint32_t nextUpdateCount;

  // synchronise to audio engine while
  // running loop() as fast as possible
  while (queue.available() > 0)
  {
    audioUpdates++;
    audioUpdated = true; // audio engine just updated
    queue.readBuffer(); // not intereseted in contents,
    queue.freeBuffer(); // just that 
  }

  if (audioUpdated)
  {
    static uint32_t nextConnect;
    static int state;
    
    audioUpdated = false;

    if (audioUpdates > nextConnect)
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

          nextConnect = audioUpdates + UPDATE_INTERVAL;
          state++;
          break;


        // creating new connections
        case 1:
          pc[2] = new AudioConnection(wav1,mixerL);
          Serial.println("wav1 connection created");

          nextConnect = audioUpdates + UPDATE_INTERVAL;
          state++;
          break;

        case 2:      
          pc[3] = new AudioConnection(wav2,mixerR);
          Serial.println("wav2 connection created");        
          
          nextConnect = audioUpdates + UPDATE_INTERVAL;
          state++;
          break;


        // disconnecting existing connections
        case 3:
          pc[2]->disconnect();
          Serial.println("wav1 disconnected");

          nextConnect = audioUpdates + UPDATE_INTERVAL;
          state++;
          break; 
                   
        case 4:
          pc[3]->disconnect();
          Serial.println("wav2 disconnected");

          nextConnect = audioUpdates + UPDATE_INTERVAL;
          state++;
          break; 


        // re-making connections:
        // the original object and port is remembered
        case 5:
          pc[2]->connect();
          Serial.println("wav1 re-connected");

          nextConnect = audioUpdates + UPDATE_INTERVAL;
          state++;
          break; 
                   
        case 6:
          pc[3]->connect();
          Serial.println("wav2 re-connected");

          nextConnect = audioUpdates + UPDATE_INTERVAL;
          state++;
          break; 

          
        // swapping connections to different mixer inputs
        // these are set to a lower level in setup(), so
        // it's easy to hear the effect
        case 7:
          pc[2]->disconnect();
          pc[2]->connect(wav1,0,mixerR,1);
          Serial.println("wav1 re-connected to new destination");

          nextConnect = audioUpdates + UPDATE_INTERVAL;
          state++;
          break; 
                   
        case 8:
          pc[3]->disconnect();
          pc[3]->connect(wav2,0,mixerL,1);
          Serial.println("wav2 re-connected to new destination");

          nextConnect = audioUpdates + UPDATE_INTERVAL;
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
  // Can enable this to see update count ticking up
  if (AUDIO_COUNT_ENABLED && audioUpdates >= nextUpdateCount)
  {
    nextUpdateCount += 100; // about 290ms
    Serial.printf("%lu audio updates\n",audioUpdates);
  }
}
/***************************************************************************/
