/*
	(c) Frank Boesing, 2014	
	GNU General Public License version 3
*/

#ifndef memcpy_audio_h_
#define memcpy_audio_h_

#ifdef __cplusplus
extern "C" {
#endif
void memcpy_tointerlaceLR(short *dst, short *srcL, short *srcR);
void memcpy_tointerlaceL(short *dst, short *srcL);
void memcpy_tointerlaceR(short *dst, short *srcR);
#ifdef __cplusplus
}
#endif


#endif