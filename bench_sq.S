#include "pct_asm.h"

#define dst	r0
#define count	r1
#define src	r2

.macro SQ
	pref	@dst
	xor	#32, dst
.endm
.macro PRE
	pref	@r2
	add	#32, src
.endm

.globl _BenchSQ
_BenchSQ:
		PCT_SAVE_REGS
		
		PCT_GET_RANGE_R2R3
		PCT_INIT	r2, r3
		
		PCT_START
		
		//***Do setup here***
		mov.l	@(PCT_USERDATA_IDX_OFS(0), pct_track), dst
		mov.l	@(PCT_USERDATA_IDX_OFS(1), pct_track), src
		mov.l	@(PCT_USERDATA_IDX_OFS(2), pct_track), count
		shlr2	count
		dt	count
		//***Do setup here***
		
	PCT_MEASURE_START
		//***This area is measured***
	1:
		PRE
		PRE
		SQ
		SQ
		PRE
		PRE
		SQ
		SQ
	
		bf/s	1b
		 dt	count
		
		//***This area is measured***
	PCT_MEASURE_END
		//***Do cleanup here***
		//No cleanup
		//***Do cleanup here***
		
		PCT_END
		
		PCT_RESTORE_REGS_RETURN
