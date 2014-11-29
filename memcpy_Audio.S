/*
	(c) Frank Boesing, 2014	
	GNU General Public License version 3
*/
 
.cpu cortex-m4
.syntax unified
.thumb
.text
.align	2

/* void memcpy_Audio(short *dst1, short *src, int len); */
 .global	memcpy_Audio
.thumb_func	
	memcpy_Audio:

	@ r0: dst1
	@ r1: src
	@ r2: len

	push	{r0, r4-r10}
	.align 2	
.loop:

	ldmia r1!,{r3-r10} 
	stmia r0!,{r3-r10} 
	
	subs r6, #4*8
	bhs .loop
	
	pop	{r0, r4-r10}
	
.END


