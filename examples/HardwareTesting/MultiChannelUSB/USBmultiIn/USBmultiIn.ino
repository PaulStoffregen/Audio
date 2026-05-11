/*
 * Example of 8-channel USB aoudio input from host
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
AudioInputUSBOct         usb_oct_in;       //xy=192,222
AudioMixer4              mixerL1;         //xy=444,121
AudioMixer4              mixerL2; //xy=452,185
AudioMixer4              mixerR1;  //xy=465,257
AudioMixer4              mixerR2; //xy=473,323
AudioMixer4              mixerL;         //xy=624,165
AudioMixer4              mixerR; //xy=636,303
AudioOutputI2S           i2s_out;           //xy=817,231

AudioConnection          patchCord1{usb_oct_in, 0, mixerL1, 0};
AudioConnection          patchCord2{usb_oct_in, 0, mixerR1, 0};
AudioConnection          patchCord3{usb_oct_in, 1, mixerL1, 1};
AudioConnection          patchCord4{usb_oct_in, 1, mixerR1, 1};
AudioConnection          patchCord5{usb_oct_in, 2, mixerL1, 2};
AudioConnection          patchCord6{usb_oct_in, 2, mixerR1, 2};
AudioConnection          patchCord7{usb_oct_in, 3, mixerL1, 3};
AudioConnection          patchCord8{usb_oct_in, 3, mixerR1, 3};
AudioConnection          patchCord9{usb_oct_in, 4, mixerL2, 0};
AudioConnection          patchCord10{usb_oct_in, 4, mixerR2, 0};
AudioConnection          patchCord11{usb_oct_in, 5, mixerL2, 1};
AudioConnection          patchCord12{usb_oct_in, 5, mixerR2, 1};
AudioConnection          patchCord13{usb_oct_in, 6, mixerL2, 2};
AudioConnection          patchCord14{usb_oct_in, 6, mixerR2, 2};
AudioConnection          patchCord15{usb_oct_in, 7, mixerL2, 3};
AudioConnection          patchCord16{usb_oct_in, 7, mixerR2, 3};
AudioConnection          patchCord17{mixerL1, 0, mixerL, 0};
AudioConnection          patchCord18{mixerL2, 0, mixerL, 1};
AudioConnection          patchCord19{mixerR1, 0, mixerR, 0};
AudioConnection          patchCord20{mixerR2, 0, mixerR, 1};
AudioConnection          patchCord21{mixerL, 0, i2s_out, 0};
AudioConnection          patchCord22{mixerR, 0, i2s_out, 1};

AudioControlSGTL5000     sgtl5000;     //xy=853,290
// GUItool: end automatically generated code


void setup() 
{
  AudioMemory(80);

  for (int i=0;i<4;i++)
  {
    mixerL1.gain(i,0.1f + i*0.1f);    
    mixerL2.gain(i,0.5f + i*0.1f);    
    mixerR1.gain(i,0.8f - i*0.1f);    
    mixerR2.gain(i,0.4f - i*0.1f);
        
    mixerL.gain(i,0.5f);    
    mixerR.gain(i,0.5f);    
  }

  sgtl5000.enable();
  sgtl5000.volume(0.1f);
}

void loop() 
{
}
