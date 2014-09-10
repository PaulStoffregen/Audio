/*
CHORUS and FLANGE effects
  Both effects use a delay line to hold previous samples. This allows
  the current sample to be combined in some way with a sample that
  occurred in the past. An obvious effect this would allow would be
  an echo where the current sample is combined with a sample from,
  say, 250 milliseconds ago. The chorus and flange effects do this
  as well but they combine samples from only about 50ms (or less) ago.
  
CHORUS EFFECT
  This combines one or more samples up to about 50ms ago. In this
  library, the additional samples are evenly spread through the
  supplied delay line.
  E.G. If the number of voices is specified as 2 then the effect
  combines the current sample and the oldest sample (the last one in
  the delay line). If the number of voices is 3 then the effect
  combines the most recent sample, the oldest sample and the sample
  in the middle of the delay line.
  For two voices the effect can be represented as:
  result = (sample(0) + sample(dt))/2
  where sample(0) represents the current sample and sample(dt) is
  the sample in the delay line from dt milliseconds ago.

FLANGE EFFECT
  This combines only one sample from the delay line but the position
  of that sample varies sinusoidally.
  In this case the effect can be represented as:
  result = sample(0) + sample(dt + depth*sin(2*PI*Fe))
  The value of the sine function is always a number from -1 to +1
  and so the result of depth*(sinFe) is always a number from
  -depth to +depth. Thus, the delayed sample will be selected from
  the range (dt-depth) to (dt+depth). This selection will vary
  at whatever rate is specified as the frequency of the effect Fe.

Try these settings:
#define FLANGE_DELAY_LENGTH (2*AUDIO_BLOCK_SAMPLES)
and
int s_idx = 2*FLANGE_DELAY_LENGTH/4;
int s_depth = FLANGE_DELAY_LENGTH/4;
double s_freq = 3;
The flange effect can also produce a chorus effect if a longer
delay line is used with a slower rate, for example try:
#define FLANGE_DELAY_LENGTH (12*AUDIO_BLOCK_SAMPLES)
and
int s_idx = 3*FLANGE_DELAY_LENGTH/4;
int s_depth = FLANGE_DELAY_LENGTH/8;
double s_freq = .0625;

  When trying out these effects with recorded music as input, it is
  best to use those where there is a solo voice which is clearly
  "in front" of the accompaniment. Tracks which already contain
  flange or chorus effects don't work well.
  
*/
