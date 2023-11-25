//Perf counter

#include "perfcnt.h"

#define RANGE_0_THRU_16	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16
#define RANGE_1_THRU_16	1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16
#define RANGE_0_THRU_25	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25
#define RANGE_1_THRU_25	1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25
#define RANGE_0_THRU_40	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40
#define RANGE_1_THRU_40	1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40
#define RANGE_16_THRU_1	16,15,14,13,12,11,10, 9,8,7,6,5,4,3,2,1
#define RANGE_16_THRU_0	16,15,14,13,12,11,10, 9,8,7,6,5,4,3,2,1,0
#define RANGE_25_THRU_0	25,24,23,22,21,20, 19,18,17,16,15,14,13,12,11,10, 9,8,7,6,5,4,3,2,1,0
#define RANGE_25_THRU_1	25,24,23,22,21,20, 19,18,17,16,15,14,13,12,11,10, 9,8,7,6,5,4,3,2,1
#define RANGE_40_THRU_0	40,39,38,37,36,35,34,33,32,31,30, 29,28,27,26,25,24,23,22,21,20, 19,18,17,16,15,14,13,12,11,10, 9,8,7,6,5,4,3,2,1,0
#define RANGE_40_THRU_1	40,39,38,37,36,35,34,33,32,31,30, 29,28,27,26,25,24,23,22,21,20, 19,18,17,16,15,14,13,12,11,10, 9,8,7,6,5,4,3,2,1


#define pct_cr		r8
#define pct_ctr		r9
#define pct_track	r10

//////////////////////////////////////////////////////////////////////
//		PERF COUNTER MACROS
//////////////////////////////////////////////////////////////////////

//counter_id values
#define PM_CR0		(0)
#define PM_CR1		(4)

#define PM_CTR0_H	(0)
#define PM_CTR0_L	(4)
#define PM_CTR1_H	(8)
#define PM_CTR1_L	(12)

/*
	Stops and disables one perf counter, also clears event value.
	Does not clear counter.
	
	Input regs:
		pct_cr_ptr
	Trashed regs:
		r0
*/
.macro PFC_STOP_R0 pct_cr_ptr, counter_id
	/*
		mov.w	@(\counter_id, \pct_cr_ptr), r0
		swap.b	r0, r0
		or	#PMCR_PMENABLE>>8, r0
		xor	#PMCR_PMENABLE>>8, r0
		swap.b	r0, r0
	*/
	mov	#0, r0
	mov.W	r0, @(\counter_id, \pct_cr_ptr)
.endm

/*
	Stops and disables both perf counters, also clears event value.
	Does not clear counters.
	
	Input regs:
		pct_cr_ptr
	Trashed regs:
		r0
*/
.macro PFC_STOP_BOTH_R0 pct_cr_ptr
	mov	#0, r0
	mov.W	r0, @(PM_CR0, \pct_cr_ptr)
	mov.W	r0, @(PM_CR1, \pct_cr_ptr)
.endm



/*
	Clears counter, does not change event or stop/start counter
	
	Input regs:
		pct_cr_ptr
	Trashed regs:
		r0
*/
.macro PFC_CLEAR_R0 pct_cr_ptr, counter_id
	mov.W	@(\counter_id, \pct_cr_ptr), r0
	swap.b	r0, r0
	or	#PMCR_PMCLR>>8, r0
	swap.b	r0, r0
	mov.W	r0, @(\counter_id, \pct_cr_ptr)
.endm


/*
	Stops and clears both counters, event is set to 0
	Perf counter is disabled
	Input regs:
		pct_cr_ptr
	Trashed regs:
		r0
*/
.macro PFC_STOP_CLEAR_BOTH_R0 pct_cr_ptr
	mov	#PMCR_PMCLR>>8, r0
	shll8	r0
	mov.W	r0, @(PM_CR0, \pct_cr_ptr)
	mov.W	r0, @(PM_CR1, \pct_cr_ptr)
.endm

/*
	Input regs:
		r0 (event id), pct_cr_ptr
	Trashed regs:
		r0
*/
//Event id should be in r0
.macro PFC_RESUME_R0 pct_cr_ptr, counter_id
	swap.b	r0, r0
	or	#(PMCR_PMENABLE | PMCR_PMST)>>8, r0
	swap.b	r0, r0
	mov.W	r0, @(\counter_id, \pct_cr_ptr)
.endm

/*
	Clears and starts perf counter with event id (in r0)
	Input regs:
		r0 (event id), pct_cr_ptr
	Trashed regs:
		r0
*/
//Event id should be in r0
.macro PFC_START_R0 pct_cr_ptr, counter_id, use_ubc = 0
	swap.b	r0, r0
.if \use_ubc
	or	#(PMCR_PMENABLE | PMCR_PMCLR)>>8, r0
.else
	or	#(PMCR_PMENABLE | PMCR_PMST | PMCR_PMCLR)>>8, r0
.endif
	swap.b	r0, r0
	mov.W	r0, @(\counter_id, \pct_cr_ptr)
.endm

/*
	Input regs:
		r0 (counter 0 event id), r1 (counter event 1 id), pct_cr_ptr
	Trashed regs:
		r0, r1, r2
*/
.macro PFC_START_BOTH_R0_R1_R2 pct_cr_ptr
	mov	#PMCR_PMENABLE | PMCR_PMST | PMCR_PMCLR, r2
	shll8	r2
	or	r2, r0
	or	r2, r1
	mov.W	r0, @(PM_CR0, \pct_cr_ptr)
	mov	r1, r0
	mov.W	r0, @(PM_CR1, \pct_cr_ptr)
.endm


//////////////////////////////////////////////////////////////////////
//		PERF COUNTER TRACKER MACROS
//////////////////////////////////////////////////////////////////////

#define PCT_RESULTS_OFS		0
#define PCT_CUR_EVENT_OFS	4
#define PCT_EVENTS_OFS		8
#define PCT_FUNC_OFS		16
#define PCT_SETUP_OFS		20
#define PCT_USERDATA_INT_OFS	24
#define PCT_USERDATA_INT_CNT	4
#define PCT_USERDATA_STACK_OFS	40
#define PCT_USERDATA_STACK_CNT	8
#define PCT_USERDATA_FLOAT_OFS	72
#define PCT_USERDATA_FLOAT_CNT	8
#define PCT_USERDATA_IDX_OFS(x)	(PCT_USERDATA_INT_OFS + (x)*4)
#define PCT_USERDATA_FLOAT_IDX_OFS(x)	(PCT_USERDATA_FLOAT_OFS + (x)*4)

/*
	Macro branch labels
	
	In order to support the usage of multiple PCTs in a single source file,
	we can't use private direct labels like ".Lfoo:" because there can only be
	one used in a single file. Number labels like "1:" can be used multiple times.
	
	Defines are used to keep track of what each number label is used for.
	
			bt	1f
			//blah
		1:
	
	...becomes...
	
			bt	PCT_F(PCT_LBL_FOO)
			//blah
		PCT_LBL_FOO:
*/
#define PCT_F2(label)	label ## f
#define PCT_B2(label)	label ## b
#define PCT_F(label)	PCT_F2(label)
#define PCT_B(label)	PCT_B2(label)

#define PCT_LBL_LOOP_START	62834
#define PCT_LBL_LOOP_END	62835
#define PCT_LBL_NO_SETUP	628341
#define PCT_LBL_PCT_CR		6283410
#define PCT_LBL_PCT_CTR		6283411
#define PCT_LBL_UBC4		6283412
#define PCT_LBL_CONST_END	628343
#define PCT_LBL_RANGE_START	6283450
#define PCT_LBL_RANGE_END	6283451
#define PCT_LBL_HAVE_EVENT	6283470
#define PCT_LBL_NEXT_EVENT	6283471
#define PCT_LBL_RANGE_SIZE	628348
#define PCT_LBL_RANGE_SIZE_END	628349

.macro PCT_LOAD_RREG idx, dst
	mov.l	@(PCT_USERDATA_IDX_OFS(\idx), pct_track), \dst
.endm
.macro PCT_LOAD_FREG idx, dst
	mov	#PCT_USERDATA_FLOAT_OFS + \idx * 4, r0
	fmov	@(r0, pct_track), \dst
.endm

.macro PCT_GET_FREG_PTR dst
	mov	pct_track, \dst
	add	#PCT_USERDATA_FLOAT_OFS, \dst
.endm


.macro PCT_SAVE_REGS
		mov.l	r8, @-r15
		mov.l	r9, @-r15
		mov.l	r10, @-r15
		mov.l	r11, @-r15
		mov.l	r12, @-r15
		mov.l	r13, @-r15
		mov.l	r14, @-r15
		mov	r4, pct_track
		sts.l	pr, @-r15
.endm

.macro PCT_RESTORE_REGS_RETURN
		lds.l	@r15+, pr
		mov.l	@r15+, r14
		mov.l	@r15+, r13
		mov.l	@r15+, r12
		mov.l	@r15+, r11
		mov.l	@r15+, r10
		mov.l	@r15+, r9
		rts
		 mov.l	@r15+, r8
.endm


/*
	Initializes perf counter tracker.
	Set up UBC to trigger performance counter start/stop
	BAMRA, BAMRB = 0x4	//Trigger exact address, ignore ASID
	BBRA, BBRB = 0x1c	//Trigger on any instruction
	BRCR = 1		//Enable UBC
	
	Input:
		start, stop (must not be in r0/r1)
	Output:
		pct_cr, pct_ctr
	Trashed:
		r0, r1
*/
#define SH_UBC_BARA_OFS		(0x0000)	//L
#define SH_UBC_BAMRA_OFS	(0x0004)	//B
#define SH_UBC_BBRA_OFS		(0x0008)	//W
#define SH_UBC_BARB_OFS		(0x000C)	//L
#define SH_UBC_BAMRB_OFS	(0x0010)	//B
#define SH_UBC_BBRB_OFS		(0x0014)	//W
#define SH_UBC_BDRB_OFS		(0x0018)
#define SH_UBC_BDMRB_OFS	(0x001C)
#define SH_UBC_BRCR_OFS		(0x0020)	//W

.macro PCT_INIT start, stop, pct_results=pct_track
	//Get address for perf counter registers
		mov.l	PCT_F(PCT_LBL_PCT_CR), pct_cr
		mov.l	PCT_F(PCT_LBL_PCT_CTR), pct_ctr
		mov.l	PCT_F(PCT_LBL_UBC4), r1	//Get UBC regs pointer+4
		bra	PCT_F(PCT_LBL_CONST_END)
		 mov	#0x4, r0
	.align 2
	PCT_LBL_PCT_CR:
		.long	PM_CR_BASE_ADDR
	PCT_LBL_PCT_CTR:
		.long	PM_CTR_BASE_ADDR
	PCT_LBL_UBC4:
		.long	0xff200004
	PCT_LBL_CONST_END:
	
		//Set address masks (BAMRA, BAMRB)
		mov.b	r0, @(SH_UBC_BAMRA_OFS-4, r1)	//BAMRA
		mov.b	r0, @(SH_UBC_BAMRB_OFS-4, r1)	//BAMRB
		
		//Set cycles (BBRA, BBRB)
		mov	#0x1c, r0
		mov.w	r0, @(SH_UBC_BBRA_OFS-4, r1)
		mov.w	r0, @(SH_UBC_BBRB_OFS-4, r1)
		
		//Set BRCR
		mov	#1, r0
		mov.w	r0, @(SH_UBC_BRCR_OFS-4, r1)
		add	#-4, r1
		
		mov.l	\start, @(SH_UBC_BARA_OFS, r1)
		mov.l	\stop, @(SH_UBC_BARB_OFS, r1)
		
		//Start with first event
		mov	#0, r0
		mov.l	r0, @(PCT_CUR_EVENT_OFS, \pct_results)
.endm

/*
	Places range defined by following PCT_MEASURE_START and PCT_MEASURE_END in r2 and r3
	
	MOVA can only return an address on a 4 byte boundry, and is limited to a 1KB range.
	To support large code that isn't an even number of instructions, we calculate
	the end manually.
*/
.macro PCT_GET_RANGE_R2R3
	mov.W	PCT_F(PCT_LBL_RANGE_SIZE), r3
	mova	PCT_F(PCT_LBL_RANGE_START), r0
	mov	r0, r2
	bra	PCT_F(PCT_LBL_RANGE_SIZE_END)
	 add	r2, r3
	PCT_LBL_RANGE_SIZE:
		.word	PCT_F(PCT_LBL_RANGE_END) - PCT_F(PCT_LBL_RANGE_START)
	PCT_LBL_RANGE_SIZE_END:
.endm

.macro PCT_MEASURE_START
	.align	2
		PCT_LBL_RANGE_START:
.endm

.macro PCT_MEASURE_END
	PCT_LBL_RANGE_END:
		//Last instruction of the benchmarked area can be miscounted if it's dual executed with another instruction
		//This clrmac instruction cannot dual execute with anything, and helps ensure the last instruction is counted correctly
		clrmac
.endm

/*
	Input regs:
		pct_results, pct_cr, pct_ctr
	Trashed regs:
		r0-r7
*/
.macro PCT_START use_ubc=1, pct_results=pct_track
.if \use_ubc == 0
	.error Not sure this still works
.endif
	PCT_LBL_LOOP_START:
		//Check if we are running current event
		mov.l	@(PCT_CUR_EVENT_OFS, \pct_results), r0
		//Get bit to test from event mask
		and	#0x7, r0
		mov	#1, r1
		shld	r0, r1
		
		//Get byte we are testing from event mask
		mov.l	@(PCT_CUR_EVENT_OFS, \pct_results), r0
		shlr2	r0
		shlr	r0
		add	#PCT_EVENTS_OFS, r0
		mov.b	@(r0, \pct_results), r0
		
		//If bit set, run next event
		tst	r1, r0
		bf	PCT_F(PCT_LBL_HAVE_EVENT)
		
		//Otherwise, do next event
		bra	PCT_F(PCT_LBL_NEXT_EVENT)
		 nop
		
	PCT_LBL_HAVE_EVENT:
		//Do user setup if pct->setup is not null
		mov.l	@(PCT_SETUP_OFS, \pct_results), r0
		tst	r0, r0
		bt	PCT_F(PCT_LBL_NO_SETUP)
		jsr	@r0
		 mov	\pct_results, r4
	PCT_LBL_NO_SETUP:
		
		//Clear counter, start counting current event
		//PFC_CLEAR	pct_cr, PM_CR0
		mov.l	@(PCT_CUR_EVENT_OFS, \pct_results), r0
		PFC_START_R0	pct_cr, PM_CR0, \use_ubc
.endm

/*
	Input regs:
		pct_results, pct_cr, pct_ctr
	Trashed regs:
		r0-r3
*/
.macro PCT_END use_ubc=1, pct_results=pct_track
	.if \use_ubc == 0
		//Stop counter and write result
		PFC_STOP_R0	pct_cr, PM_CR0
	.endif
		
		//pct->results->val[pct->cur_event] += result;
		mov.l	@(PCT_CUR_EVENT_OFS, \pct_results), r0
		mov.l	@(PCT_RESULTS_OFS, \pct_results), r3
		mov.l	@(PM_CTR0_L, pct_ctr), r1
		shll2	r0
		mov.l	@(r0, r3), r2
		add	r1, r2
		mov.l	r2, @(r0, r3)
		
	PCT_LBL_NEXT_EVENT:
		//Advance to next event
		//pct->cur_event++;
		mov.l	@(PCT_CUR_EVENT_OFS, \pct_results), r0
		add	#1, r0
		mov.l	r0, @(PCT_CUR_EVENT_OFS, \pct_results)
		
		//Finished last event?
		cmp/eq	#PE_LAST_EVENT+1, r0
		bt	PCT_F(PCT_LBL_LOOP_END)
		//Might have a large jump if there's a lot between PCT_START and PCT_END, which
		//would cause a warning about BF being converted to BT/BRA, so do
		//that manually to prevent the warning
		bra	PCT_B(PCT_LBL_LOOP_START)
		 nop
	PCT_LBL_LOOP_END:
.endm
