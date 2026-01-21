/*
 * Demonstration of S/PDIF clock sync, with fallback to
 * internal clock if the input signal is lost.
 *
 * Designed for:
 * - Teensy 4.x
 * - S/PDIF I/O on pins 15 and 14
 * - audio adaptor
 */
#include <Audio.h>

// GUItool: begin automatically generated code
AudioInputSPDIF3         spdifIn;        //xy=92,141
AudioSynthWaveform       wavL;           //xy=100,185
AudioSynthWaveform       wavR;           //xy=110,223
AudioMixer4              mixerL;         //xy=327,151
AudioMixer4              mixerR;         //xy=341,225
AudioOutputI2S           i2sOut;           //xy=543,156
AudioOutputSPDIF3        spdifOut;       //xy=553,215

AudioConnection          patchCord1(spdifIn, 0, mixerL, 0);
AudioConnection          patchCord2(spdifIn, 0, mixerR, 1);
AudioConnection          patchCord3(spdifIn, 1, mixerR, 0);
AudioConnection          patchCord4(spdifIn, 1, mixerL, 1);
AudioConnection          patchCord5(wavL, 0, mixerL, 2);
AudioConnection          patchCord6(wavL, 0, mixerR, 3);
AudioConnection          patchCord7(wavR, 0, mixerR, 2);
AudioConnection          patchCord8(wavR, 0, mixerL, 3);
AudioConnection          patchCord9(mixerL, 0, i2sOut, 0);
AudioConnection          patchCord10(mixerL, 0, spdifOut, 0);
AudioConnection          patchCord11(mixerR, 0, spdifOut, 1);
AudioConnection          patchCord12(mixerR, 0, i2sOut, 1);

AudioControlSGTL5000     sgtl5000;       //xy=669,156
// GUItool: end automatically generated code

const int SPDIF_in_L_ch = 0,
          SPDIF_in_R_ch = 1,
          wav_L_ch = 2,
          wav_R_ch = 3;

void setup() 
{
  AudioMemory(100);

  sgtl5000.enable();
  sgtl5000.volume(0.1f);

  wavL.begin(1.0f, 440.0f, WAVEFORM_SINE);
  wavR.begin(1.0f, 550.0f, WAVEFORM_SINE);

  // mix incoming and local signals
  // across stereo field:
  mixerL.gain(SPDIF_in_L_ch,0.4f);
  mixerL.gain(SPDIF_in_R_ch,0.1f);
  mixerL.gain(wav_L_ch,     0.3f);
  mixerL.gain(wav_R_ch,     0.15f);

  mixerR.gain(SPDIF_in_L_ch,0.1f);
  mixerR.gain(SPDIF_in_R_ch,0.4f);
  mixerR.gain(wav_L_ch,     0.15f);
  mixerR.gain(wav_R_ch,     0.3f);
}


/*
 * Ensure audio clocks and update responsibility
 * are correct, depending on incoming signal
 */
bool pollSPDIFsync(void)
{
  bool haveSPDIFinput = spdifIn.pllLocked(); // do we have signal?

  AudioNoInterrupts();

  i2sOut.syncToSPDIF(haveSPDIFinput); // sync only if we have signal

  // ensure update resonsibility comes from
  // ONE object ONLY, and from the one which
  // is providing the audio sample clock
  spdifIn.grabUpdateResponsibility(haveSPDIFinput);
  i2sOut.grabUpdateResponsibility(!haveSPDIFinput);
  
  AudioInterrupts();

  return haveSPDIFinput;
}

void loop() 
{
  static uint32_t outputRate = 0;
  static bool hadSPDIFinput = false;

  // need to poll fairly frequently
  bool haveSPDIFinput = pollSPDIFsync();

  // show user what's going on
  if (hadSPDIFinput != haveSPDIFinput) // S/PDIF input has changed
  {
    hadSPDIFinput = haveSPDIFinput;
    Serial.printf("S/PDIF input %s", haveSPDIFinput?"gained":"lost");
    if (haveSPDIFinput)
      outputRate = millis() + 100; // needs time to become valid
    else
      Serial.println();
  }

  // if recently got S/PDIF input, output the sample rate
  if (0 != outputRate && millis() >= outputRate)
  {
    outputRate = 0;
    Serial.printf(": sample rate %dHz\n", spdifIn.sampleRate());
  }
}
