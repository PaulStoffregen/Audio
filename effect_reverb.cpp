/*
 * Copyright (c) 2016 Joao Rossi Filho
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// https://github.com/joaoRossiFilho/teensy_reverb

#include "effect_reverb.h"
#include "utility/dspinst.h"
#include "math_helper.h"

void 
AudioEffectReverb::_do_comb_apf(struct comb_apf *apf, int32_t *in_buf, int32_t *out_buf)
{
  int32_t acc_x, acc_y, g;
  int32_t w, z;
  uint32_t n, buf_len;

  g = apf->g;
  buf_len = apf->buf_len;

  for (n = 0; n < AUDIO_BLOCK_SAMPLES; n++) {
    acc_y = apf->buffer[apf->rd_idx%buf_len];
    acc_x = in_buf[n];

    w = multiply_32x32_rshift32_rounded(g, acc_y);
    acc_x += (w << 1);
    z = multiply_32x32_rshift32_rounded(g, acc_x);
    acc_y -= (z << 1);

    apf->buffer[apf->wr_idx%buf_len] = acc_x;
    out_buf[n] = acc_y;

    apf->rd_idx++;
    apf->wr_idx++;
  }
}

void
AudioEffectReverb::_do_comb_lpf(struct comb_lpf *lpf, int32_t *in_buf, int32_t *out_buf)
{
  int32_t x, y, w, z, g1, g2, z1;
  uint32_t n, buf_len;

  g1 = lpf->g1;
  g2 = lpf->g2;
  z1 = lpf->z1;
  buf_len = lpf->buf_len;

  for (n = 0; n < AUDIO_BLOCK_SAMPLES; n++) {
    y = lpf->buffer[lpf->rd_idx%buf_len];
    x = in_buf[n];

    w = multiply_accumulate_32x32_rshift32_rounded(y, g2, z1);
    z = multiply_accumulate_32x32_rshift32_rounded(x, g1, w);

    z1 = w;
    lpf->buffer[lpf->wr_idx%buf_len] = z;
    out_buf[n] = y;

    lpf->rd_idx++;
    lpf->wr_idx++;
  }

  lpf->z1 = z1;
}

void
AudioEffectReverb::init_comb_filters(void)
{
  int i;

  g_flt_apf[0] = 0.7;
  g_flt_apf[1] = -0.54;
  g_flt_apf[2] = 0.6;

  g2_flt_lpf = 0.985;

  arm_float_to_q31(g_flt_apf, g_q31_apf, 3);
  arm_float_to_q31(&g2_flt_lpf, &g2_q31_lpf, 1);

  apf[0].buffer = apf1_buf;
  apf[0].buf_len = APF1_BUF_LEN;
  apf[0].delay = APF1_DLY_LEN;
  apf[1].buffer = apf2_buf;
  apf[1].buf_len = APF2_BUF_LEN;
  apf[1].delay = APF2_DLY_LEN;
  apf[2].buffer = apf3_buf;
  apf[2].buf_len = APF3_BUF_LEN;
  apf[2].delay = APF3_DLY_LEN;

  for (i = 0; i < 3; i++) {
    apf[i].g = g_q31_apf[i];
    apf[i].wr_idx = 0;
    apf[i].rd_idx = apf[i].wr_idx - apf[i].delay -1;
  }

  lpf[0].buffer = lpf1_buf;
  lpf[0].buf_len = LPF1_BUF_LEN;
  lpf[0].delay = LPF1_DLY_LEN;
  lpf[1].buffer = lpf2_buf;
  lpf[1].buf_len = LPF2_BUF_LEN;
  lpf[1].delay = LPF2_DLY_LEN;
  lpf[2].buffer = lpf3_buf;
  lpf[2].buf_len = LPF3_BUF_LEN;
  lpf[2].delay = LPF3_DLY_LEN;
  lpf[3].buffer = lpf4_buf;
  lpf[3].buf_len = LPF4_BUF_LEN;
  lpf[3].delay = LPF4_DLY_LEN;

  for (i = 0; i < 4; i++) {
    lpf[i].g1 = g1_q31_lpf[i];
    lpf[i].g2 = g2_q31_lpf;
    lpf[i].wr_idx = 0;
    lpf[i].rd_idx = lpf[i].wr_idx - lpf[i].delay -1;
  }
}

void
AudioEffectReverb::clear_buffers(void)
{
  memset(apf1_buf, 0, APF1_BUF_LEN);
  memset(apf2_buf, 0, APF1_BUF_LEN);
  memset(apf3_buf, 0, APF1_BUF_LEN);

  memset(lpf1_buf, 0, LPF1_BUF_LEN);
  memset(lpf2_buf, 0, LPF2_BUF_LEN);
  memset(lpf3_buf, 0, LPF3_BUF_LEN);
  memset(lpf4_buf, 0, LPF4_BUF_LEN);
}

void
AudioEffectReverb::reverbTime(float rt)
{
  if (rt <= 0.0)
    return;

  reverb_time_sec = rt;

  g1_flt_lpf[0] = powf(10.0, -(3.0*LPF1_DLY_SEC)/(reverb_time_sec));
  g1_flt_lpf[1] = powf(10.0, -(3.0*LPF2_DLY_SEC)/(reverb_time_sec));
  g1_flt_lpf[2] = powf(10.0, -(3.0*LPF3_DLY_SEC)/(reverb_time_sec));
  g1_flt_lpf[3] = powf(10.0, -(3.0*LPF4_DLY_SEC)/(reverb_time_sec));

  arm_float_to_q31(g1_flt_lpf, g1_q31_lpf, 4);

  for (int i = 0; i < 4; i++)
    lpf[i].g1 = g1_q31_lpf[i];
}

void
AudioEffectReverb::update(void)
{
  audio_block_t *block;

  if (!(block = receiveWritable()))
    return;

  if (!block->data)
    return;

  arm_q15_to_q31(block->data, q31_buf, AUDIO_BLOCK_SAMPLES);

  _do_comb_apf(&apf[0], q31_buf, q31_buf);
  _do_comb_apf(&apf[1], q31_buf, q31_buf);

  _do_comb_lpf(&lpf[0], q31_buf, sum_buf);
  arm_shift_q31(sum_buf, -3, sum_buf, AUDIO_BLOCK_SAMPLES);

  _do_comb_lpf(&lpf[1], q31_buf, aux_buf);
  arm_shift_q31(aux_buf, -3, aux_buf, AUDIO_BLOCK_SAMPLES);
  arm_add_q31(sum_buf, aux_buf, sum_buf, AUDIO_BLOCK_SAMPLES);

  _do_comb_lpf(&lpf[2], q31_buf, aux_buf);
  arm_shift_q31(aux_buf, -3, aux_buf, AUDIO_BLOCK_SAMPLES);
  arm_add_q31(sum_buf, aux_buf, sum_buf, AUDIO_BLOCK_SAMPLES);

  _do_comb_lpf(&lpf[3], q31_buf, aux_buf);
  arm_shift_q31(aux_buf, -3, aux_buf, AUDIO_BLOCK_SAMPLES);
  arm_add_q31(sum_buf, aux_buf, sum_buf, AUDIO_BLOCK_SAMPLES);

  _do_comb_apf(&apf[2], sum_buf, q31_buf);

  arm_q31_to_q15(q31_buf, block->data, AUDIO_BLOCK_SAMPLES);

  transmit(block, 0);
  release(block);
}
/* EOF */
