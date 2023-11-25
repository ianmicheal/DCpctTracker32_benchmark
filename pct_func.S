#include "pct_asm.h"

//returns 0 on success, 1 if pct is null, 2 if func is null, 3 if results is null
//int pctBenchmarkFunc(pctTracker32 *pct);

//#define DISABLE_IRQS	1

.globl _pctBenchmarkFunc
_pctBenchmarkFunc:
		PCT_SAVE_REGS
		
		//Check for null tracker
		tst	pct_track, pct_track
		mov	#1, r0
		bt	.Lexit
		
		//Check for null function
		mov.l	@(PCT_FUNC_OFS, pct_track), r0
		tst	r0, r0
		mov	#2, r0
		bt	.Lexit
		
		//Check for null results
		mov.l	@(PCT_RESULTS_OFS, pct_track), r0
		tst	r0, r0
		mov	#3, r0
		bt	.Lexit
		
		mova	.Lret_addr, r0
		mov.l	@(PCT_FUNC_OFS, pct_track), r11
		mov	r0, r12
		PCT_INIT	r11, r12
		
		PCT_START
		
		//Call function
		add	#-PCT_USERDATA_STACK_CNT*4, r15
		mov	pct_track, r0
		add	#PCT_USERDATA_STACK_OFS, r0
		mov.l	@(0, r0), r4
		mov.l	@(4, r0), r5
		mov.l	@(8, r0), r6
		mov.l	@(12, r0), r7
		mov.l	r4, @(0, r15)
		mov.l	r5, @(4, r15)
		mov.l	r6, @(8, r15)
		mov.l	r7, @(12, r15)
		mov.l	@(16, r0), r4
		mov.l	@(20, r0), r5
		mov.l	@(24, r0), r6
		mov.l	@(28, r0), r7
		mov.l	r4, @(16, r15)
		mov.l	r5, @(20, r15)
		mov.l	r6, @(24, r15)
		mov.l	r7, @(28, r15)
		
		mov	pct_track, r0
		add	#PCT_USERDATA_FLOAT_OFS, r0
		fmov	@r0+, fr4
		fmov	@r0+, fr5
		fmov	@r0+, fr6
		fmov	@r0+, fr7
		fmov	@r0+, fr8
		fmov	@r0+, fr9
		fmov	@r0+, fr10
		fmov	@r0+, fr11
		
		mov.l	@(PCT_FUNC_OFS, pct_track), r1
		mov.l	@(PCT_USERDATA_IDX_OFS(0), pct_track), r4
		mov.l	@(PCT_USERDATA_IDX_OFS(1), pct_track), r5
		mov.l	@(PCT_USERDATA_IDX_OFS(2), pct_track), r6
		mov.l	@(PCT_USERDATA_IDX_OFS(3), pct_track), r7
	.align 2
		jsr	@r1
		 nop
	.Lret_addr:
		add	#PCT_USERDATA_STACK_CNT*4, r15
		
		PCT_END
		
		mov	#0, r0
	.Lexit:
		
		PCT_RESTORE_REGS_RETURN
		
	
