#ifndef PCTRACKER_H
#define PCTRACKER_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "perfcnt.h"

typedef struct pctTracker32 pctTracker32;
typedef struct pctTrackerResults32 pctTrackerResults32;
//~ typedef void (*pctTestFunc)();
typedef void *pctTestFunc;
typedef void (*pctSetup)(pctTracker32 *);

struct pctTrackerResults32 {
	uint32_t val[PE_COUNT /* 0x2a */];
};

//Uses 32-bits for counters, events may overflow if run for too long.
//Cycle counter event will overflow after 21 seconds
struct pctTracker32 {
	//Destination of perf counter results
	//Always used
	//Set by pctInit and pctBenchmarkMany
	pctTrackerResults32 *results;
	
	//Always used (initialized and used by asm macros, not touched by C code)
	unsigned int cur_event;
	
	//Events to run
	//Set with pctEvent* functions
	uint64_t events;
	
	//Function benchmarked by pctBenchmarkOne/pctBenchmarkMany
	//Only used if calling something with pctBenchmarkOne/pctBenchmarkMany,
	//and not used by uses inline PCT asm macros
	//Set with pctInitFunc
	pctTestFunc func;
	
	//Called by PCT asm macro before each test
	//Always used
	//Set with pctInitFunc
	pctSetup setup;
	
	//Values passed to func by pctBenchmarkOne/pctBenchmarkMany
	//Only used if calling something with pctBenchmarkFunc
	//These can be freely used for any purpose if using inline PCT asm macros
	//Set with PCT_SET_*
	uintptr_t param_r[4];
	uintptr_t param_stack[8];
	float param_f[8];
};

#define PCT_SET_R(tracker, idx, val)	do { (tracker).param_r[idx] = (uintptr_t)(val); } while(0)
#define PCT_SET_S(tracker, idx, val)	do { (tracker).param_stack[idx] = (uintptr_t)(val); } while(0)
#define PCT_SET_F(tracker, idx, val)	do { (tracker).param_f[idx] = (val); } while(0)



/*
	Initalizes pctTracker32
	All events default to enabled.
	results should be non-NULL, unless using pctBenchmarkMany
*/
void pctInit(pctTracker32 *pct, pctTrackerResults32 *results);

/*
	func can be NULL if calling a custom benchmark using asm macros, but must
	be set if using pctBenchmarkOne/pctBenchmarkMany
	
	setup can be NULL if no setup is required
*/
void pctInitFunc(pctTracker32 *pct, pctTestFunc func, pctSetup setup);

/*
	Enables/disables one event
*/
void pctEventSet(pctTracker32 *pct, bool enable, sh4PerfEvent event);
/*
	Enables a single event, and disables all others
*/
void pctEventEnableSingle(pctTracker32 *pct, sh4PerfEvent event);
/*
	Enables/disables events in the range [first, last].
	
	This will enable non-events like 0x0C or 0x1B if contained in the range.
	
	first and last will be clamped to be less than PE_COUNT
*/
void pctEventSetRange(pctTracker32 *pct, bool enable, sh4PerfEvent first, sh4PerfEvent last);
/*
	Enables/disables all normal events, and runs non-event #0 as an icache warm up run.
	Runs 31 events total (1 warmup + 30 real)
*/
void pctEventSetAll(pctTracker32 *pct, bool enable);


/*
	Runs the benchmark for all enabled events. The value of each performance counter
	is added to the results (it does not replace the result value).
	
	When multiple runs are used, all events for one run are tested before starting another run.
	
	For example, if events PE_CYCLE_COUNT and PE_STALL_OC_MISS_CYCLES are enabled,
	and three runs are used, the function is run in this order:
		1. run 1 PE_CYCLE_COUNT
		2. run 1 PE_STALL_OC_MISS_CYCLES
		3. run 2 PE_CYCLE_COUNT
		4. run 2 PE_STALL_OC_MISS_CYCLES
		5. run 3 PE_CYCLE_COUNT
		6. run 3 PE_STALL_OC_MISS_CYCLES
*/
void pctBenchmarkOne(pctTracker32 *tracker, unsigned int runs);

/*
	Runs the benchmark for all enabled events for multiple functions.
	The results for each function is written to its own pctTrackerResults32.
*/
void pctBenchmarkMany(pctTracker32 *tracker, pctTrackerResults32 *results, void **funcs, size_t func_cnt, unsigned int runs);

//Clears the values of a pctTrackerResults32
void pctClearResults(pctTrackerResults32 *pctr);
//Clears array of pctTrackerResults32s
void pctClearResultsMany(pctTrackerResults32 *pctr, size_t result_cnt);

/*
	Prints pctTrackerResults32. Up to three columns. First column is always raw numbers,
	
	Columns 2 and 3 are scaled versions of the data, divided by scale or scale2.
	If you do 200 runs and want to display the average, pass 200 for the scale value.
	The extra columns are hidden by passing 1.0f or 0.0f.
*/

void pctPrint(const pctTrackerResults32 *results, float scale, float scale2);
//pfcLabel from perfcnt.h is a good function to use for a pctLabelEvent
typedef const char * (*pctLabelEvent)(sh4PerfEvent event);
typedef const char * (*pctLabelRun)(size_t event);
void pctPrintArrayCSVRow(const pctTrackerResults32 *results, size_t result_cnt, float scale, pctLabelRun runlbl, pctLabelEvent eventlbl);
void pctPrintArrayCSVCol(const pctTrackerResults32 *results, size_t result_cnt, float scale, pctLabelRun runlbl, pctLabelEvent eventlbl);

/*
	Prints list of enabled events in a tracker. If eventlbl is NULL, pfcLabel is used.
*/
void pctPrintEvents(const pctTracker32 *tracker, pctLabelEvent eventlbl);


/*
	These functions can help setup cache state for benchmarking. The act of
	calling and returning from the setup function will modify the state of the cache
	a bit, so it's not possible to get it entirely exact.
	
	Movca/Pref can be used to ensure cache hits, Purge can be used to ensure cache miss
	Nop does nothing.
*/
void pctPrefArea(void *area, size_t size);
void pctNopArea(void *area, size_t size);
void pctMovcaArea(void *area, size_t size);
void pctPurgeArea(void *area, size_t size);

typedef enum {
	PCT_PURGE_ALL,
	PCT_PURGE_OCI0,
	PCT_PURGE_OCI1,
} pctPurgeType;
/*
	Moves everything out of data cache; dirty lines are written back to memory, clean lines are evicted.
	No need to know the addreess of what's in the cache, like you do with pctPurgeArea.
	"banks" parameter controls how cache is flushed when OCINDEX is enabled
	
	When OCINDEX is not enabled (normal usage), banks parameter is ignored and the entire cache is always purged
	When OCINDEX is enabled, banks parameter controls which bank is flushed: 
		PCT_PURGE_ALL  = both
		PCT_PURGE_OCI0 = bank 0
		PCT_PURGE_OCI1 = bank 1
	You can pass PCT_PURGE_ALL to always flush entire cache, regardless of OCINDEX mode
*/
void pctPurgeCache(pctPurgeType banks);

#endif
