#include "pct_asm.h"

.macro DELAY_CYCLE
	//or	r15, r15
	nop
	nop
.endm

//stack is likely to be in cache
.macro DUMMY_READ
	mov.l	@r15, r0
	nop
.endm

.macro DUMMY_WRITE
	mov.l	r0, @r15
	//nop
.endm

#define ODD_CYCLES	0

#if 1
	//Regular
	#define DO_PREF	1
	#define REAL_LOAD	1
	#define INCSIZE	32
#elif 1
	//No pref
	#define NO_PREF	1
	#define REAL_LOAD	1
	#define INCSIZE	32
#elif 1
	//No read/pref
	#define NO_PREF	1
	#define NO_LOAD	1
	#define INCSIZE	32
#elif 1
	//Pref r15 read
	#define DO_PREF	1
	#define STACK_LOAD	1
	#define INCSIZE	32
#elif 1
	//Pref noread
	#define DO_PREF	1
	#define NO_LOAD	1
	#define INCSIZE	32
#elif 1
	//No inc
	#define DO_PREF	1
	#define REAL_LOAD	1
	#define INCSIZE	0
#elif 1
	//No inc/read
	#define DO_PREF	1
	#define NO_LOAD	1
	#define INCSIZE	0
#endif

#define xstr(a) #a
#define str(a) xstr(a)


//Generate a string to describe settings to help record what type of benchmark is being run
.align 5
.globl _BenchPrefString
_BenchPrefString:

#if DO_PREF
	.macro PREFM	reg
		pref	@\reg
		nop
	.endm
	.ascii "PREF YES | "
#elif NO_PREF
	.macro PREFM	reg
		nop
		nop
	.endm
	.ascii "PREF NO  | "
#else
	#error	Bad pref
#endif

#if REAL_LOAD
	.macro	LOADM
		mov.l	@r4, r0
		nop
	.endm
	.ascii "LOAD FROM CACHE | "
#elif STACK_LOAD
	.macro	LOADM
		mov.l	@r15, r0
		nop
	.endm
	.ascii "LOAD FROM STACK | "
#elif NO_LOAD
	.macro	LOADM
		nop
		nop
	.endm
	.ascii "LOAD FROM NONE  | "
#else
	#error	Bad load
#endif

#if ODD_CYCLES
	.ascii "ODD  | "
#else
	.ascii "EVEN | "
#endif
	.ascii "INC "
	.asciz str(INCSIZE)


//.macro BranchPref_Dx PostWait=40
.irp PostWait, NoPref, RANGE_40_THRU_0
.globl _BenchPrefX_\PostWait
.align 5
_BenchPrefX_\PostWait:
		mov	r15, r0
		add	#-4, r15
	1:
		
		DELAY_CYCLE
.if \PostWait <> NoPref
	.rept \PostWait
		DELAY_CYCLE
		//DUMMY_WRITE
	.endr
		
		PREFM	r4
	.rept 40-\PostWait
		DELAY_CYCLE
		//DUMMY_WRITE
	.endr
.else
	.rept 40	//40 cycle delay
		DELAY_CYCLE
	.endr
		DELAY_CYCLE	//Fake PREF delay
.endif
		LOADM
		
		dt	r5
		nop
		
		add	#INCSIZE, r4
		nop
		
	#if ODD_CYCLES
		nop
	#endif
	
		nop
		bf	1b
		
		rts
		 add	#4, r15

.endr


//extern void (*PrefBenchList[42])(void *area, int lines);
.globl _PrefBenchList
.align 5
_PrefBenchList:
.irp PostWait, NoPref, RANGE_40_THRU_0
		.long _BenchPrefX_\PostWait
.endr
