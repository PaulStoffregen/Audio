/* Teensyduino Audio Memcpy
 * Copyright (c) 2016 Frank Bösing
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * 2. If the Software is incorporated into a build system that allows 
 * selection among a list of target devices, then similar target
 * devices manufactured by PJRC.COM must be included in the list of
 * target devices and selectable in the same manner.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

.cpu cortex-m4
.syntax unified
.thumb
.text
.align	2

/* void memcpy_tointerleave(short *dst, short *srcL, short *srcR); */
 .global	memcpy_tointerleaveLR
.thumb_func
	memcpy_tointerleaveLR:

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

	pkhbt r3,r5,r6,LSL #16	//1
	pkhtb r4,r6,r5,ASR #16	//1

	pkhbt r5,r7,r8,LSL #16	//1
	pkhtb r6,r8,r7,ASR #16	//1

	pkhbt r7,r9,r10,LSL #16	//1
	pkhtb r8,r10,r9,ASR #16	//1

	pkhbt r9,r11,r12,LSL #16	//1
	pkhtb r10,r12,r11,ASR #16	//1

	//Write 8 Words
	stmia r0!, {r3,r4,r5,r6,r7,r8,r9,r10}	//1+8

	.endr //5+5+8+9 = 27 Cycles to interleave 32 bytes.

	cmp r14, r0
	bne .loopLR

	pop	{r3-r12,r14}
	BX lr


/* void memcpy_tointerleaveL(short *dst, short *srcL); */
 .global	memcpy_tointerleaveL
.thumb_func
	memcpy_tointerleaveL:

	@ r0: dst
	@ r1: srcL

	push	{r2-r12}
	mov r2, #0
	add r12,r0,#256
	.align 2
.loopL:

	.irp offset, 1,2

	//Load 4 words
	ldmia r1!, {r5,r7,r9,r11}  //1+4

	pkhbt r3,r5,r2	//1
	pkhtb r4,r2,r5,ASR #16	//1

	pkhbt r5,r7,r2	//1
	pkhtb r6,r2,r7,ASR #16	//1

	pkhbt r7,r9,r2	//1
	pkhtb r8,r2,r9,ASR #16	//1

	pkhbt r9,r11,r2	//1
	pkhtb r10,r2,r11,ASR #16	//1

	//Write 8 Words
	stmia r0!, {r3,r4,r5,r6,r7,r8,r9,r10}	//1+8

	.endr

	cmp r12, r0
	bne .loopL

	pop	{r2-r12}
	BX lr

/* void memcpy_tointerleaveL(short *dst, short *srcR); */
 .global	memcpy_tointerleaveR
.thumb_func
	memcpy_tointerleaveR:

	@ r0: dst
	@ r1: srcR

	push	{r2-r12}
	mov r2, #0
	add r12,r0,#256
	.align 2
.loopR:

	.irp offset, 1,2

	//Load 4 words
	ldmia r1!, {r5,r7,r9,r11}

	pkhbt r3,r2,r5,LSL #16
	pkhtb r4,r5,r2

	pkhbt r5,r2,r7,LSL #16
	pkhtb r6,r7,r2

	pkhbt r7,r2,r9,LSL #16
	pkhtb r8,r9,r2

	pkhbt r9,r2,r11,LSL #16
	pkhtb r10,r11,r2

	//Write 8 Words
	stmia r0!, {r3,r4,r5,r6,r7,r8,r9,r10}

	.endr

	cmp r12, r0
	bne .loopR

	pop	{r2-r12}
	BX lr


.END


