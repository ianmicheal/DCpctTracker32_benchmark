
#define SH4_PERFCNT_IMPLENTATION	1
#include "pctracker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "sh4.h"

int pctBenchmarkFunc(pctTracker32 *pct);

void pctInit(pctTracker32 *pct, pctTrackerResults32 *results) {
	assert(pct);
	
	memset(pct, 0, sizeof(*pct));
	pct->results = results;
	pctEventSetAll(pct, true);
}

void pctInitFunc(pctTracker32 *pct, void *func, pctSetup setup) {
	assert(pct);
	
	pct->func = func;
	pct->setup = setup;
}

void pctClearResults(pctTrackerResults32 *pctr) {
	assert(pctr);
	
	memset(pctr, 0, sizeof(*pctr));
}
void pctClearResultsMany(pctTrackerResults32 *pctr, size_t count) {
	assert(pctr);
	for(size_t i = 0; i < count; i++)
			pctClearResults(pctr + i);
}

void pctEventEnableSingle(pctTracker32 *pct, sh4PerfEvent event) {
	assert(pct);
	
	pct->events = 0;
	pctEventSet(pct, true, event);
}
void pctEventSet(pctTracker32 *pct, bool enable, sh4PerfEvent event) {
	assert(pct);
	
	uint64_t bit = 1ull << event;
	
	pct->events &= ~bit;
	if (enable)
		pct->events |= bit;
}
void pctEventSetRange(pctTracker32 *pct, bool enable, sh4PerfEvent first, sh4PerfEvent last) {
	assert(pct);
	
	//Want to have first be smaller value
	if (last < first) {
		sh4PerfEvent temp = first;
		first = last;
		last = temp;
	}
	
	if (first > PE_LAST_EVENT)
		first = PE_LAST_EVENT;
	if (last > PE_LAST_EVENT)
		last = PE_LAST_EVENT;
	
	//Make mask for range
	uint64_t mask = ((uint64_t)-1) << first;
	mask ^= ((uint64_t)-1) << (last + 1);
	
	pct->events &= ~mask;
	if (enable)
		pct->events |= mask;
}
void pctEventSetAll(pctTracker32 *pct, bool enable) {
	assert(pct);
	
	pct->events = enable ? 0x3BE07FFEFFF : 0;
}

void pctPrint(const pctTrackerResults32 *results, float scale, float scale2) {
	assert(results);
	
	if (scale == 0)
		scale = 1;
	if (scale2 == 0)
		scale2 = 1;
	
	printf("                                         ");
	if (scale != 1.0f)
		printf("    1/%8.2f", scale);
	if (scale2 != 1.0f)
		printf("    1/%8.2f", scale2);
	printf("\n");
	scale = 1/scale;
	scale2 = 1/scale2;
	for(int i = 1; i < PE_COUNT; i++) {
		if (i == 0xc || i == 0x26)
			i++;
		else if (i == (PE_UBCB_COUNT+1))
			i = PE_CYCLE_COUNT;
		printf("%30s: %10lu", pfcLabel(i), results->val[i]);
		if (scale != 1.0f)
			printf("  (%10.2f)", results->val[i] * scale);
		if (scale2 != 1.0f)
			printf("  [%10.2f]", results->val[i] * scale2);
		printf("\n");
		//~ fflush(stdout);
	}
}

void pctPrintEvents(const pctTracker32 *tracker, pctLabelEvent eventlbl) {
	if (eventlbl == NULL)
		eventlbl = &pfcLabel;
	
	printf("Enabled events:\n");
	for(unsigned int i = 0; i < PE_COUNT; i++) {
		if (tracker->events & (1ull << i)) {
			printf("%s\n", eventlbl(i));
		}
	}
}
void pctPrintArrayCSVRow(const pctTrackerResults32 *results, size_t result_cnt, float scale, pctLabelRun runlbl, pctLabelEvent eventlbl) {
	assert(results);
	if (scale == 0)
		scale = 1;
	
	/*
		 CSV:
		 Row is bench, col is perfmon value
	*/
	scale = 1/scale;
	
	if (eventlbl) {
		//Add empty cell over the run names
		if (runlbl)
			printf(" , ");
		
		//Print run names for valid events
		for(int i = 1; i < PE_COUNT; i++) {
			if (i == 0xc || i == 0x26)
				i++;
			else if (i == (PE_UBCB_COUNT+1))
				i = PE_CYCLE_COUNT;
			printf("%s, ", eventlbl(i));
		}
		printf("\n");
	}
	
	for(size_t j = 0; j < result_cnt; j++) {
		if (runlbl)
			printf("%s, ", runlbl(j));
		for(int i = 1; i < PE_COUNT; i++) {
			if (i == 0xc || i == 0x26)
				i++;
			else if (i == (PE_UBCB_COUNT+1))
				i = PE_CYCLE_COUNT;
		
			//~ printf("%lu, ", results[j].val[i]);
			printf("%.2f, ", results[j].val[i] * scale);
		}
		printf("\n");
	}
}
void pctPrintArrayCSVCol(const pctTrackerResults32 *results, size_t result_cnt, float scale, pctLabelRun runlbl, pctLabelEvent eventlbl) {
	assert(results);
	
	if (scale == 0)
		scale = 1;
	
	/*
		 CSV:
		 Col is bench, row is perfmon value
	*/
	scale = 1/scale;
	
	if (runlbl) {
		//Add empty cell over the labels
		if (eventlbl)
			printf(" , ");
		for(size_t j = 0; j < result_cnt; j++) {
			printf("%s, ", runlbl(j));
		}
		printf("\n");
	}
	
	for(size_t i = 1; i < PE_COUNT; i++) {
		//Skip nonexistant events
		if (i == 0xc || i == 0x26)
			i++;
		else if (i == (PE_UBCB_COUNT+1))
			i = PE_CYCLE_COUNT;
		
		if (eventlbl)
			printf("%s, ", eventlbl(i));
		for(size_t j = 0; j < result_cnt; j++) {
			printf("%.2f, ", results[j].val[i] * scale);
			//~ printf("%lu, ", results[j].val[i] );
		}
		printf("\n");
	}
}

void pctBenchmarkOne(pctTracker32 *tracker, unsigned int runs) {
	assert(tracker);
	assert(tracker->results);
	assert(tracker->func);
	
	int irqstatus = irq_disable();
	for(unsigned int i = 0; i < runs; i++) {
		int ret = 0;
		ret = pctBenchmarkFunc(tracker);
		if (ret) {
			irq_restore(irqstatus);
			printf("error calling pctBenchmarkFunc (%i)\n", ret);
			fflush(stdout);
			return;
		}
	}
	irq_restore(irqstatus);
}

void pctBenchmarkMany(pctTracker32 *tracker, pctTrackerResults32 *results, void **funcs, size_t func_cnt, unsigned int runs) {
	assert(tracker);
	assert(results);
	assert(funcs);
	assert(func_cnt > 0);
	
	int irqstatus = irq_disable();
	for(size_t j = 0; j < func_cnt; j++) {
		tracker->results = results + j;
		tracker->func = funcs[j];
		for(unsigned int i = 0; i < runs; i++) {
			int ret = 0;
			ret = pctBenchmarkFunc(tracker);
			if (ret) {
				irq_restore(irqstatus);
				printf("error calling pctBenchmarkFunc (%i)\n", ret);
				fflush(stdout);
				return;
			}
		}
	}
	irq_restore(irqstatus);
}

void pctPurgeArea(void *area, size_t size) {
	for(size_t i = 0; i < size; i += 32) {
		__asm__ __volatile__("ocbp @%0" : : "r" (area + i) : "memory");
	}
}
void pctPrefArea(void *area, size_t size) {
	for(size_t i = 0; i < size; i += 32) {
		__asm__ __volatile__("pref @%0" : : "r" (area + i) : "memory");
	}
}
void pctMovcaArea(void *area, size_t size) {
	register int __value  __asm__("r0") = 0;
	for(size_t i = 0; i < size; i += 32) {
		__asm__ __volatile__ (
			"movca.l r0,@%0\n\t"
			:  
			: "r" (area + i), "r" (__value)
			: "memory"  );
	}
}

#define OCI_BANK0(a) (typeof (&(a)[0]))(((unsigned int)(a)) & ~(1 << 25))
#define OCI_BANK1(a) (typeof (&(a)[0]))(((unsigned int)(a)) |  (1 << 25))
#define CACHE_SIZE	(16*1024)
static char CacheFlushArea[CACHE_SIZE] __attribute__ ((aligned (32)));

void pctPurgeCache(pctPurgeType banks) {
	volatile unsigned int *CCR = (void*)0xff00001c;
	unsigned int ccrval = *CCR;
	unsigned int size = CACHE_SIZE;
	//Purges the cache by using MOVCA to flush any existing dirty cachelines, then
	//invalidates cacheline to clear dirty bit so MOVCA garbage can't trigger
	//a writeback later
	
	//Treat any invalid value as request to flush entire cache
	if (banks > PCT_PURGE_OCI1)
		banks = PCT_PURGE_ALL;
	
	//Check OCINDEX
	if (ccrval & (1<<7)) {
		//Each bank is half size of full cache
		size >>= 1;
	} else {
		//If not using OCINDEX, ignore banks parameter, only purge cache once
		banks = PCT_PURGE_OCI0;
	}
	
	//Check OCRAM
	if (ccrval & (1<<5)) {
		//Usable cache size is halved
		size >>= 1;
	}
	
	if (banks != PCT_PURGE_OCI1) {
		void *ptr = CacheFlushArea;
		for(size_t i = 0; i < size; i += 64) {
			sh4CachelineAllocate(ptr + i, 0);
			sh4Invalidate(ptr + i);
			sh4CachelineAllocate(ptr + i + 32, 0);
			sh4Invalidate(ptr + i + 32);
		}
	}
	
	if (banks != PCT_PURGE_OCI0) {
		void *ptr = OCI_BANK1(CacheFlushArea);
		for(size_t i = 0; i < size; i += 64) {
			sh4CachelineAllocate(ptr + i, 0);
			sh4Invalidate(ptr + i);
			sh4CachelineAllocate(ptr + i + 32, 0);
			sh4Invalidate(ptr + i + 32);
		}
	}
}
void pctNopArea(void *area, size_t size) {
	(void)area;
	(void)size;
}
