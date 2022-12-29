/*
 * A simple queue test which generates a sine wave in the 
 * application and sends it to headphone jack. The results
 * should always be a glitch-free wave, but various options
 * can be changed to modify the usage of audio blocks, and an
 * ability to execute the loop() function more slowly, with
 * simple waveform generation, or more efficiently / faster,
 * but requring some programmer effort to re-try if sending
 * waveform data fails due to a lack of buffer or queue space.
 *
 * This example code is in the public domain.
 * 
 * Jonathan Oakley, November 2021
 */
 
 #include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioPlayQueue           queue1;         //xy=917,329
AudioOutputI2S           i2s2;           //xy=1121,330
AudioConnection          patchCord1(queue1, 0, i2s2, 0);
AudioConnection          patchCord2(queue1, 0, i2s2, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=1131,379
// GUItool: end automatically generated code

#define COUNT_OF(a) (sizeof a / sizeof a[0])

uint32_t next;
void setup() {
  pinMode(13,OUTPUT);
  
  Serial.begin(115200);
  while (!Serial)
    ;

  if (CrashReport) 
  {
    Serial.println(CrashReport);
    CrashReport.clear();
  }

  AudioMemory(10);
  
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);

  next = millis();
  
  // Comment the following out (or set to ORIGINAL) for old stall behaviour;
  // set to NON_STALLING for return with status if audio blocks not available,
  // or no room in queue for another audio block.
  queue1.setBehaviour(AudioPlayQueue::NON_STALLING);

  queue1.setMaxBuffers(4);
}


/*
 * Generate one sample of a waveform.
 * Currently 220Hz sine wave, but could make it more complex.
 */
uint32_t genLen;
int16_t nextSample()
{
  static float phas = 0.f;
  int16_t result = (int16_t) (sin(phas)*8000.);
  genLen++;
  
  phas += 220./AUDIO_SAMPLE_RATE_EXACT*TWO_PI;
  if (phas > TWO_PI)
    phas -= TWO_PI;  

  return result;
}


int loops;
int nulls,nulls2;
int testMode = 2; // 1: getBuffer / playBuffer; 2: play(), mix of samples and buffers
int playMode; // 1: generate individual samples and send; 2: generate buffer of samples and send
int16_t samples[512],*sptr; // space for samples when using play()
uint32_t len; // number of buffered samples (remaining)

void loop() {

  switch (testMode)
  {
    case 1: // use getBuffer / playBuffer 
    {  
      int16_t* buf = queue1.getBuffer();
      
      if (NULL == buf)
        nulls++;
      else
      {
        for (int i=0;i<AUDIO_BLOCK_SAMPLES;i++)
          buf[i] = nextSample();
        queue1.playBuffer();
      }
    }
      break;

    case 2: // use play()
    {
      if (0 == len)
      {
        playMode = random(2)+1; // 1 or 2     
      }
        
      switch (playMode)
      {
        case 1: // send single samples
          if (0 == len)
          {
            len = random(10,2*AUDIO_BLOCK_SAMPLES+1); // be deliberately awkward
            samples[0] = nextSample();
          }
          
          while (len > 0)
          {
            if (0 == queue1.play(samples[0])) // sent a sample...
            {
              len--;        // count down
              if (len > 0)  // if more to send...
                samples[0] = nextSample(); // ...create the next one   
            }
            else
            {
              nulls++; // count up the re-tries
              break;
            }
          }
          break;
        
         case 2: // send a block of samples
          if (0 == len)
          {
            len = random(10,COUNT_OF(samples)); // be deliberately awkward
            for (unsigned int i=0;i<len;i++) // pre-fill the buffer
              samples[i] = nextSample();
            sptr = samples;
          }
          
          {
            uint32_t left = queue1.play(sptr,len); // send samples, maybe
            if (left > 0) // not everything went
            {
              sptr += len - left; // advance the pointer
              nulls2++; // count up the re-tries
            }            
            len = left; 
          }
          break;       
      }
    }
      break;
  }

  loops++;
  if (millis() > next)
  {
    next += 100; // aim to output every 100ms
    
    // In NON_STALLING mode this loops really fast, and the millis() value goes up by
    // 100 on every output line. In ORIGINAL mode the loop is slow, and the internal
    // stall results in slightly unpredictable timestamps.
    Serial.printf("%d: millis = %d, loops = %d, nulls = %u, nulls2 = %u, samples = %u\n",
                  playMode,millis(),loops,nulls,nulls2,genLen);
  }
}
