/*
	(c) Frank Boesing, 2014
	GNU General Public License version 3
*/

.cpu cortex-m4
.syntax unified
.thumb
.text
.align	2

/* void memcpy_tointerlace(short *dst, short *srcL, short *srcR); */
 .global	memcpy_tointerlaceLR
.thumb_func
	memcpy_tointerlaceLR:

	@ r0: dst
	@ r1: srcL
	@ r2: srcR

	push	{r3-r12,r14}
	add r14,r0,#256
	.align 2
.loopLR:

	.irp offset, 1,2
	
	//Load 2*4 words
	ldmia r1!, {r5,r7,r9,r11}  //1+4
	ldmia r2!, {r6,r8,r10,r12} //1+4

	pkhbt r3,r5,r6 //1
	pkhbt r4,r6,r5	//1

	pkhbt r5,r7,r8  //1
	pkhbt r6,r8,r7  //1

	pkhbt r7,r9,r10  //1
	pkhbt r8,r10,r9	//1

	pkhbt r9,r11,r12 //1
	pkhbt r10,r12,r11 //1

	//Write 8 Words
	stmia r0!, {r3,r4,r5,r6,r7,r8,r9,r10} //1+8

	.endr //5+5+8+9 = 27 Cycles to interleave 32 bytes.


	cmp r14, r0
	bne .loopLR

	pop	{r3-r12,r14}
	BX lr



/* void memcpy_tointerlaceL(short *dst, short *srcL); */
.global	memcpy_tointerlaceL
.thumb_func
	memcpy_tointerlaceL:

	@ r0: dst
	@ r1: srcL

	push	{r0,r3-r11}
	mov r11, #256 - 1
	.align 2
.loopL:


	ldrh r3,[r1],#2
	ldrh r4,[r1],#2
	ldrh r5,[r1],#2
	ldrh r6,[r1],#2
	ldrh r7,[r1],#2
	ldrh r8,[r1],#2
	ldrh r9,[r1],#2
	ldrh r10,[r1],#2

	str r3,[r0],#4
	str r4,[r0],#4
	str r5,[r0],#4
	str r6,[r0],#4
	str r7,[r0],#4
	str r8,[r0],#4
	str r9,[r0],#4
	str r10,[r0],#4

	subs r11, #32
	bhs .loopL

	pop	{r0,r3-r11}
	BX lr



/* void memcpy_tointerlaceL(short *dst, short *srcR); */
 .global	memcpy_tointerlaceR
.thumb_func
	memcpy_tointerlaceR:

	@ r0: dst
	@ r1: srcR

	push	{r0,r3-r12}
	mov r12, #0
	mov r11, #256 - 1
	.align 2
.loopR:


	ldrh r3,[r1],#2
	ldrh r4,[r1],#2
	ldrh r5,[r1],#2
	ldrh r6,[r1],#2
	ldrh r7,[r1],#2
	ldrh r8,[r1],#2
	ldrh r9,[r1],#2
	ldrh r10,[r1],#2

	strh r12,[r0],#2
	strh r3,[r0],#2

	strh r12,[r0],#2
	strh r4,[r0],#2
	strh r12,[r0],#2
	strh r5,[r0],#2
	strh r12,[r0],#2
	strh r6,[r0],#2
	strh r12,[r0],#2
	strh r7,[r0],#2
	strh r12,[r0],#2
	strh r8,[r0],#2
	strh r12,[r0],#2
	strh r9,[r0],#2
	strh r12,[r0],#2
	strh r10,[r0],#2


	subs r11, #32
	bhs .loopR

	pop	{r0,r3-r12}
	BX lr

.END


