/*
 * Example of 8-channel USB aoudio output to host
 * Settings on Tools menu:
 * - USB Type     : must include "Audio"
 * - USB channels : set to 8
 */
#include <Audio.h>

#define AUDIO_kHz ((int) AUDIO_SAMPLE_RATE / 1000)

extern "C"
{
    struct usb_string_descriptor_struct
    {
        uint8_t bLength;
        uint8_t bDescriptorType;
        uint16_t wString[6+1+1+2+1];
    };
    
  usb_string_descriptor_struct usb_string_serial_number={
    2+(6+1+1+2+1)*2,3,
    {'A','u','d','i','o','-','0'+AUDIO_USB_CHANNEL_COUNT,'/','0'+(AUDIO_kHz / 10),'0' + (AUDIO_kHz % 10),'B'}
  };
}


// GUItool: begin automatically generated code
AudioSynthWaveform       wav1;           //xy=156,129
AudioSynthWaveform       wav2;           //xy=161,165
AudioSynthWaveform       wav3;           //xy=163,203
AudioSynthWaveform       wav4;           //xy=167,240
AudioSynthWaveform       wav5;           //xy=171,278
AudioSynthWaveform       wav6;           //xy=176,314
AudioSynthWaveform       wav7;           //xy=180,351
AudioSynthWaveform       wav8;           //xy=184,389
AudioOutputUSBOct        usb_oct_out;    //xy=514,237

AudioConnection          patchCord1{wav1, 0, usb_oct_out, 0};
AudioConnection          patchCord2{wav2, 0, usb_oct_out, 1};
AudioConnection          patchCord3{wav3, 0, usb_oct_out, 2};
AudioConnection          patchCord4{wav4, 0, usb_oct_out, 3};
AudioConnection          patchCord5{wav5, 0, usb_oct_out, 4};
AudioConnection          patchCord6{wav6, 0, usb_oct_out, 5};
AudioConnection          patchCord7{wav7, 0, usb_oct_out, 6};
AudioConnection          patchCord8{wav8, 0, usb_oct_out, 7};

// GUItool: end automatically generated code


AudioSynthWaveform* wavs[] = {
  &wav1,
  &wav2,
  &wav3,
  &wav4,
  &wav5,
  &wav6,
  &wav7,
  &wav8
};

void setup() 
{
  AudioMemory(80);

  for (int i=0;i<8;i++)
    wavs[i]->begin(0.5f,(i+1)*110.0f,WAVEFORM_SINE);
}

void loop() 
{
}
