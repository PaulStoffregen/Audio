
// Table of midi note frequencies * 2
//   They are times 2 for greater accuracy, yet still fits in a word.
//   Generated from Excel by =ROUND(2*440/32*(2^((x-9)/12)),0) for 0<x<128
// The lowest notes might not work, depending on the Arduino clock frequency

const unsigned int tune_frequencies2_PGM[128] =
{
    16,17,18,19,21,22,23,24,26,28,29,31,33,35,37,39,41,
    44,46,49,52,55,58,62,65,69,73,78,82,87,92,98,104,110,
    117,123,131,139,147,156,165,175,185,196,208,220,233,
    247,262,277,294,311,330,349,370,392,415,440,466,494,
    523,554,587,622,659,698,740,784,831,880,932,988,1047,
    1109,1175,1245,1319,1397,1480,1568,1661,1760,1865,1976,
    2093,2217,2349,2489,2637,2794,2960,3136,3322,3520,3729,
    3951,4186,4435,4699,4978,5274,5588,5920,6272,6645,7040,
    7459,7902,8372,8870,9397,9956,10548,11175,11840,12544,
    13290,14080,14917,15804,16744,17740,18795,19912,21096,
    22351,23680,25088
};


#define CMD_PLAYNOTE	0x90	/* play a note: low nibble is generator #, note is next byte */
#define CMD_STOPNOTE	0x80	/* stop a note: low nibble is generator # */
#define CMD_RESTART	0xe0	/* restart the score from the beginning */
#define CMD_STOP	0xf0	/* stop playing */

// User must supply this score array
extern unsigned char score[];
