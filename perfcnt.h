#ifndef SH4_PERFCNT_H
#define SH4_PERFCNT_H

/***************************************************************************
	Hardware performance counter
***************************************************************************/

#define PM_CR_BASE_ADDR		(0xff000084)
#define PM_CTR_BASE_ADDR	(0xff100004)
#define PM_CR_BASE		((volatile int16_t*)PM_CR_BASE_ADDR)
#define PM_CTR_BASE		((volatile int32_t*)PM_CTR_BASE_ADDR)
#define PMCR(n)			(PM_CR_BASE[n*2])
#define PMCTRH(n)       	(PM_CTR_BASE[n*2])
#define PMCTRL(n)       	(PM_CTR_BASE[n*2+1])

#define PMCR_PMM_MASK   0x0000003f

#define PMCR_CLKF       0x00000100
#define PMCR_PMCLR      0x00002000

/* 	If set, perfcount starts immediately.
	otherwise, user breakpoints are disabled and UBCA event triggers start,
	and UBCB event triggers stop */
#define PMCR_PMST       0x00004000

/*
	When perf counter is enabled, UBC can not be used to trigger breakpoints
*/
#define PMCR_PMENABLE   0x00008000


//Performance counter events
#define PE_OC_READ_ACCESS		0x01
#define PE_OC_WRITE_ACCESS		0x02
#define PE_UTLB_MISS			0x03
#define PE_OC_READ_MISS			0x04
#define PE_OC_WRITE_MISS		0x05
#define PE_IC_READ_HIT			0x06
#define PE_ITLB_MISS			0x07
#define PE_IC_READ_MISS			0x08
#define PE_OC_ALL_ACCESS		0x09	//Includes PREF as one access (TODO check other cache instructions)
#define PE_IC_ALL_ACCESS		0x0a
#define PE_OCRAM_ACCESS			0x0b
#define PE_CHIPIO_ACCESS		0x0d
#define PE_OC_RW_ACCESS			0x0e
#define PE_OC_ALL_MISS			0x0f
#define PE_BRANCH_COUNT			0x10	//Does not count BT/BF over single instruction
#define PE_BRANCH_TAKEN			0x11	//Does not count BT/BF over single instruction
#define PE_SUBROUTINE_CALLS		0x12	//JSR, BSR, BSRF instruction count
#define PE_SINGLE_INSTRUCTIONS_EXECUTED	0x13
#define PE_DUAL_INSTRUCTIONS_EXECUTED	0x14
#define PE_FPU_INSTRUCTIONS_EXECUTED	0x15
#define PE_IRQ_COUNT			0x16
#define PE_NMI_COUNT			0x17
#define PE_TRAPA_COUNT			0x18
#define PE_UBCA_COUNT			0x19
#define PE_UBCB_COUNT			0x1a
#define PE_IC_LINE_READ_CYCLES 		0x21
#define PE_OC_LINE_READ_CYCLES		0x22
#define PE_CYCLE_COUNT			0x23
#define PE_STALL_IC_MISS_CYCLES		0x24	//Includes uncached access stalls
#define PE_STALL_OC_MISS_CYCLES		0x25	//Includes uncached access stalls
#define PE_STALL_BRANCH_CYCLES		0x27
#define PE_STALL_CPU_CYCLES		0x28
#define PE_STALL_FPU_CYCLES		0x29
#define PE_LAST_EVENT			PE_STALL_FPU_CYCLES
#define PE_COUNT			(PE_LAST_EVENT+1)


#if !__ASSEMBLER__

#include <stdint.h>

typedef unsigned int sh4PerfEvent;
typedef uint64_t pfCounter;	//Number of events

static inline pfCounter pfcRead(int idx) {
	pfCounter result = (pfCounter)(PMCTRH(idx) & 0xffff) << 32;
	result |= PMCTRL(idx);
	return result;
}

static inline unsigned int pfcReadLow(int idx) {
	return PMCTRL(idx);
}

static inline void pfcClear(int idx) {
	PMCR(idx) = PMCR(idx) | PMCR_PMCLR;
}

static inline void pfcStop(int idx) {
	PMCR(idx) &= ~(PMCR_PMM_MASK | PMCR_PMENABLE);
}

static inline void pfcStart(int idx, sh4PerfEvent event) {
	pfcStop(idx);
	pfcClear(idx);
	PMCR(idx) = event | PMCR_PMENABLE | PMCR_PMST;
}

const char * pfcLabel(sh4PerfEvent event);

#if SH4_PERFCNT_IMPLENTATION
const char * pfcLabel(sh4PerfEvent event) {
	static const char * event_lbl[] = {
		"Unknown event (0x00)",
		"OC Read Access Count",
		"OC Write Access Count",
		"UTLB Miss Count",
		"OC Read Miss Count",
		"OC Write Miss Count",
		"IC Read Hit Count",
		"ITLB Miss Count",
		"IC Read Miss Count",
		"OC All Access Count",	//?
		"IC All Access Count",	//?
		"OCRAM Access Count",
		"Unknown event (0x0c)",
		"Chip I/O Access Count",
		"OC Read/Write Access Count",
		"OC Read/Write Miss Count",
		"Branch Count",
		"Branch Taken Count",
		"Subroutine Call Count",
		"Single Execute Count",
		"Dual Execute Count",
		"FPU Execute Count",
		"IRQ Count",
		"NMI Count",
		"TRAPA Count",
		"UBCA Count",
		"UBCB Count",
		"Unknown event (0x1b)",
		"Unknown event (0x1c)",
		"Unknown event (0x1d)",
		"Unknown event (0x1e)",
		"Unknown event (0x1f)",
		"Unknown event (0x20)",
		"IC Line Fill Cycles",
		"OC Line Fill Cycles",
		"Cycle Count Cycles",
		"IC Miss Stall Cycles",
		"OC Miss Stall Cycles",
		"Unknown event (0x26)",
		"Branch Stall Cycles",
		"Reg Latency Stall Cycles",
		"FReg Latency Stall Cycles",
	};
	
	return event <= PE_STALL_FPU_CYCLES ? event_lbl[event] : "Unknown event";
}
#endif

#endif /* !__ASSEMBLER__ */

#endif
