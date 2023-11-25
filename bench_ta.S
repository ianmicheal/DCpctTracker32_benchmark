#include "pct_asm.h"

#define TRISTRIPRANGE	RANGE_1_THRU_16

#define vert_size	32
#define stripcnt	r5
#define sq		r4
#define vert_cmd	r2
#define vert_eos_cmd	r3

//Writes one vertex using SQ. Alternates vertex X pos depending on \num to ensure TA writes to multiple tiles
.macro write_vert	num, cmd = vert_cmd
	mov.l	\cmd, @sq
	.if \num & 1
		fmov	fr4, @(r0, sq)	//Set x0
		//fmov	fr5, @(xxxxx, sq)	//Set y0
	.else
		fmov	fr6, @(r0, sq)	//Set x1
		//fmov	fr7, @(xxxxx, sq)	//Set y1
	.endif
	pref	@sq
	add	#vert_size, sq
	
.endm

//Macro to write TA tri strip commands for a strip \num triangles long
.macro	write_tris	num
	mov.l	vert_cmd, @sq
	.if \num & 1
		fmov	fr4, @(r0, sq)
	.else
		fmov	fr6, @(r0, sq)
	.endif
	pref	@sq
	add	#vert_size, sq
	
	.if \num > 1
		write_tris (\num - 1)
	.else
		//Write final two vertices of last triangle
		mov.l	vert_cmd, @sq
		fmov	fr6, @(r0, sq)
		pref	@sq
		add	#vert_size, sq
		
		//Last vertex has end of strip set
		mov.l	vert_eos_cmd, @sq
		fmov	fr4, @(r0, sq)
		pref	@sq
		add	#vert_size, sq
	.endif
.endm

//Generate multiple versions of code for different strip lengths
.irp Striplentris, TRISTRIPRANGE
.globl _BenchPrefX_\Striplentris
.align 5
_BenchTA_\Striplentris:
		PCT_SAVE_REGS
		
		PCT_GET_RANGE_R2R3
		PCT_INIT	r2, r3
		
		PCT_START
		
		mov.l	@(PCT_USERDATA_IDX_OFS(0), pct_track), r4
		mov.l	@(PCT_USERDATA_IDX_OFS(1), pct_track), stripcnt
		PCT_GET_FREG_PTR	r0
		fmov	@r0+, fr4
		fmov	@r0+, fr5
		fmov	@r0+, fr6
		fmov	@r0+, fr7
		
		//*
		//Load 4 constant for addressing
		mov	#4, r0
		
		//Get vertex end-of-strip command (0xffffffff)
		mov	#-1, vert_eos_cmd
		
		//Get regular vertex command (0xe0000000)
		mov	#-1, vert_cmd
		shlr2	vert_cmd
		shlr	vert_cmd
		xor	r3, vert_cmd
		
		//Set Y,Z of SQs to 1.0f
		mov	r4, r1
		fldi1	fr0
		add	#8, r1
		
		fmov	fr0, @r1
		fmov	fr0, @(r0, r1)
		add	#32, r1
		fmov	fr0, @r1
		fmov	fr0, @(r0, r1)
		
		//*/
		 dt	stripcnt
	PCT_MEASURE_START
	1:

		write_tris	\Striplentris
		
		bf/s	1b
		 dt	stripcnt
		
	PCT_MEASURE_END
		
		PCT_END
		
		PCT_RESTORE_REGS_RETURN
.endr

//Generate array of pointers to TA benchmark functions
.globl _TABenchList
.align 5
_TABenchList:
.irp Striplentris, TRISTRIPRANGE
		.long _BenchTA_\Striplentris
.endr
