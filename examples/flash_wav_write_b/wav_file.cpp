#include <Arduino.h>
#include "wav_file.h"

void x_printf(char *s)
{
  Serial.print(s);
}

void x_printf(char *s,int i)
{
  char tmp[64];
  sprintf(tmp,s,i);
  Serial.print(tmp);
}

void print4char(char *p)
{
  int i;
  for(i = 0;i < 4;i++) {
    x_printf("%c",*p++);
  }
  x_printf("\n");
}

void dump_seg(struct soundhdr *sh)
{
  x_printf("text     = ");
  print4char(&sh->riff[0]);
  x_printf("flength = %d\n",sh->flength);
}

void dump_header(struct soundhdr *sh)
{
  x_printf("riff       = ");
  print4char(&sh->riff[0]);
  x_printf("flength    = %d\n",sh->flength);
  x_printf("wave       = ");
  print4char(&sh->wave[0]);
  x_printf("fmt        = ");
  print4char(&sh->fmt[0]);
  x_printf("blocksize  = %d\n",sh->block_size);
  x_printf("formattag  = %d\n",sh->format_tag);
  x_printf("numchans   = %d\n",sh->num_chans);
  x_printf("srate      = %d\n",sh->srate);
  x_printf("bytes/sec  = %d\n",sh->bytes_per_sec);
  x_printf("bytes/samp = %d\n",sh->bytes_per_samp);
  x_printf("bits/samp  = %d\n",sh->bits_per_samp);
  x_printf("data       = ");
  print4char(&sh->data[0]);
  x_printf("dlength    = %d\n",sh->dlength);
}
