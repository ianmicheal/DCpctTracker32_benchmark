#include <kos.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "sq.h"

#include "pvr_cxt.h"

#include "pctracker.h"

#define USE_OCINDEX	0

void sh4WriteCCN(unsigned int new_ccn);
void sh4EnableOCIndex(void);
void sh4DisableOCIndex(void);
void TransformVecs(float *src, int srcstride, float *dst, int dststride, int cnt);
void TransformVecsP(float *src, float *dst, int dststride, int cnt);

#define OCI_BANK0(a) (typeof (&(a)[0]))(((unsigned int)(a)) & ~(1 << 25))
#define OCI_BANK1(a) (typeof (&(a)[0]))(((unsigned int)(a)) |  (1 << 25))

char buff0[64*1024] __attribute__ ((aligned (32)));
char buff1[64*1024] __attribute__ ((aligned (32)));

void SetupUncachedAll(pctTracker32 *pct) {
	(void)pct;
	pctPurgeCache(PCT_PURGE_ALL);
}
void SetupUncachedR4(pctTracker32 *pct) {
	pctPurgeArea((void*)pct->param_r[0], 16*1024);
}
void SetupCachedR4(pctTracker32 *pct) {
	pctPurgeArea((void*)pct->param_r[0], 16*1024);
}
void SetupTVUncachedSrcCachedDst(pctTracker32 *pct) {
	pctPurgeArea((void*)pct->param_r[0], 8192);
	pctPrefArea((void*)pct->param_r[2], 8192);
}
void SetupTVCachedSrcCachedDst(pctTracker32 *pct) {
	pctPrefArea((void*)pct->param_r[0], 8192);
	pctPrefArea((void*)pct->param_r[2], 8192);
}
void SetupTVPUncachedSrcCachedDst(pctTracker32 *pct) {
	pctPurgeArea((void*)pct->param_r[0], 8192);
	pctPrefArea((void*)pct->param_r[1], 8192);
}
void SetupTVPCachedSrcCachedDst(pctTracker32 *pct) {
	pctPrefArea((void*)pct->param_r[0], 8192);
	pctPrefArea((void*)pct->param_r[1], 8192);
}
void SetupUncachedR5(pctTracker32 *pct) {
	pctPurgeArea((void*)pct->param_r[1], 16*1024);
	//~ pctPrefArea((void*)pct->param_r[1], 8192);
}

const char * PrefTestRunName(size_t run) {
	//Not thread safe, but so what?
	static char eventname[20];
	if (run == 0) {
		return "No pref";
	} else {
		int ret = snprintf(eventname, sizeof(eventname), "%i cy delay", ((int)run) - 1);
		assert(ret > 0 && (size_t)ret < sizeof(eventname));
		return eventname;
	}
}
const char * TATestRunName(size_t run) {
	//Not thread safe, but so what?
	static char eventname[20];
	int ret = snprintf(eventname, sizeof(eventname), "%i tris", ((int)run) + 1);
	assert(ret > 0 && (size_t)ret < sizeof(eventname));
	return eventname;
}

#define TA_BENCH_CNT	16
#define TA_BENCH_TEXTURED	1
#define TA_BENCH_SPECULAR	1
//Each function submits a different number of triangles in a single strip.
//TABenchList[0] is one tri, TABenchList[1] is two tris, etc...
extern void (*TABenchList[TA_BENCH_CNT])(pctTracker32 *tracker);

void BenchTA(pctTrackerResults32 *result_array, unsigned int stripcnt, unsigned int tiles) {
	assert(tiles >= 1);
	assert(tiles < 1280/32);
	
	pctTracker32 tracker;
	pctInit(&tracker, NULL);
	pctEventEnableSingle(&tracker, PE_CYCLE_COUNT);
	//~ pctPrintEvents(&tracker, NULL);
	//~ fflush(stdout);
	
	for(int i = 0; i < TA_BENCH_CNT; i++) {
		pvr_wait_ready();
		pvr_scene_begin();
		pvr_list_begin(PVR_LIST_OP_POLY);
		
		//Set TA to accept opaque polygons
		void *sq = sqPrepare((void*)PVR_TA_INPUT);
		pvr_context cxt;
		pc_set_default_polygon_packed(&cxt);
		pc_set_textured(&cxt, TA_BENCH_TEXTURED);
		pc_set_specular(&cxt, TA_BENCH_SPECULAR);
		sqCopy32(sq, &cxt);
		
		PCT_SET_R(tracker, 0, sq);
		PCT_SET_R(tracker, 1, stripcnt);
		PCT_SET_F(tracker, 0, 1);	//x0
		PCT_SET_F(tracker, 1, 1);	//y0 (not currently implemented)
		PCT_SET_F(tracker, 2, 1 + 32*(tiles-1));	//x1
		PCT_SET_F(tracker, 3, 1);	//y1 (not currently implemented)
		tracker.setup = NULL;
		tracker.results = result_array + i;
		TABenchList[i](&tracker);
		
		pvr_list_finish();
		pvr_scene_finish();
	}
}

void DoTABenchmark() {
	printf("\nBenchmarking TA... (texture: %i, specular %i)\n", TA_BENCH_TEXTURED, TA_BENCH_SPECULAR);
	
	#define MAX_TILES	16
	int stripcnt = 2000;
	int runs = 3;
	pctTrackerResults32 result_array[TA_BENCH_CNT];
	pctTracker32 tracker;
	pctInit(&tracker, NULL);
	
	int ta_cycles[MAX_TILES][TA_BENCH_CNT];	//clock cycles per striplen/tilecnt combo
	memset(ta_cycles, 0, sizeof(ta_cycles));
	
	pvr_init_params_t pvr_params = {
		{ PVR_BINSIZE_8, PVR_BINSIZE_0, PVR_BINSIZE_0, PVR_BINSIZE_0, PVR_BINSIZE_8 },
		2.0 * 1024 * 1024,
		0,	//dma
		1, 	//fsaa on if we wanted to try more tiles
		0	//autosort disable
	};
	pvr_init(&pvr_params);
	
	for(int k = 0; k < runs; k++) {
		//This benchmark is split into multiple runs to avoid running out of vertex memory
		printf("TA bench run %i...\n", k);
		fflush(stdout);
		for(int i = 0; i < MAX_TILES; i++) {
			int tilecnt = i+1;
			
			pctClearResultsMany(result_array, TA_BENCH_CNT);
			BenchTA(result_array, stripcnt, tilecnt);
			
			//Save cycle count
			for(int j = 0; j < TA_BENCH_CNT; j++)
				ta_cycles[i][j] += result_array[j].val[PE_CYCLE_COUNT];
		}
	}


	//Print cycles per strip count
	{
		//Header
		printf("\nAverage cycles per strip:\n , ");
		for(int j = 0; j < TA_BENCH_CNT; j++)
			printf("%i tris, ", j+1);
		//Actual data
		for(int i = 0; i < MAX_TILES; i++) {
			printf("\n%i tiles, ", i+1);
			for(int j = 0; j < TA_BENCH_CNT; j++)
				printf("%.3f, ", ta_cycles[i][j] /(float)(stripcnt*runs));
		}
		printf("\n");
	}
	//Print cycles per vertex
	{
		//Header
		printf("\nAverage cycles per vertex:\n , ");
		for(int j = 0; j < TA_BENCH_CNT; j++)
			printf("%i tris, ", j+1);
		//Actual data
		for(int i = 0; i < MAX_TILES; i++) {
			printf("\n%i tiles, ", i+1);
			for(int j = 0; j < TA_BENCH_CNT; j++)
				printf("%.3f, ", ta_cycles[i][j] /(float)(stripcnt*runs*(j+3)));
		}
		printf("\n");
	}
	//Print cycles per triangle
	{
		//Header
		printf("\nAverage cycles per tri:\n , ");
		for(int j = 0; j < TA_BENCH_CNT; j++)
			printf("%i tris, ", j+1);
		printf("\n");
		//Actual data
		for(int i = 0; i < MAX_TILES; i++) {
			printf("%i tiles, ", i+1);
			for(int j = 0; j < TA_BENCH_CNT; j++) {
				printf("%.3f, ", ta_cycles[i][j] /(float)(stripcnt*runs*(j+1)));
			}
			printf("\n");
		}
	}

	if (0) {
		pvr_stats_t stats;
		pvr_get_stats(&stats);
		printf("Vert buff: %i bytes\n", stats.vtx_buffer_used);
	}
}

void DoPrefFtrvBenchmark(void *src, void *dst) {
	pctTrackerResults32 results;
	pctTracker32 tracker;
	pctInit(&tracker, &results);
	int runs = 100, count = 128;
	
	printf("\n");
	printf("Transforming vertices (no optimization for uncached source) - uncached source, cached destination:\n");
	pctInitFunc(&tracker, &TransformVecs, &SetupTVUncachedSrcCachedDst);
	PCT_SET_R(tracker, 0, src);
	PCT_SET_R(tracker, 1, 16);	//src stride
	PCT_SET_R(tracker, 2, dst);
	PCT_SET_R(tracker, 3, 32);	//dst stride
	PCT_SET_S(tracker, 0, count);
	pctClearResults(&results);
	pctBenchmarkOne(&tracker, runs);
	pctPrint(&results, runs, runs*count);
	
	printf("\n");
	printf("Transforming vertices (no optimization for uncached source) - cached source, cached destination:\n");
	pctInitFunc(&tracker, &TransformVecs, &SetupTVCachedSrcCachedDst);
	pctClearResults(&results);
	pctBenchmarkOne(&tracker, runs);
	pctPrint(&results, runs, runs*count);
	fflush(stdout);
	
	printf("\n");
	printf("Transforming vertices (optimized for uncached source) - uncached source, cached destination:\n");
	pctInitFunc(&tracker, &TransformVecsP, &SetupTVPUncachedSrcCachedDst);
	PCT_SET_R(tracker, 0, src);
	PCT_SET_R(tracker, 1, dst);
	PCT_SET_R(tracker, 2, 32);	//dst stride
	PCT_SET_R(tracker, 3, count);
	pctClearResults(&results);
	pctBenchmarkOne(&tracker, runs);
	pctPrint(&results, runs, runs*count);
	fflush(stdout);
	
	printf("Transforming vertices (optimized for uncached source) - cached source, cached destination:\n");
	pctInitFunc(&tracker, &TransformVecsP, &SetupTVPCachedSrcCachedDst);
	pctClearResults(&results);
	pctBenchmarkOne(&tracker, runs);
	pctPrint(&results, runs, runs*count);
	fflush(stdout);
}

void DoSQBenchmark(void *src, void *dst) {
	printf("Pref/SQ flush test\n");
	extern int BenchSQ(pctTracker32 *pct);
	
	pctTrackerResults32 results;
	pctTracker32 tracker;
	int count = 128;
	
	pctInit(&tracker, &results);
	pctInitFunc(&tracker, NULL, &SetupUncachedAll);
	pctClearResults(&results);
	//BenchSQ manually reads the tracker parameters, so set those
	PCT_SET_R(tracker, 0, sqPrepare(dst));
	PCT_SET_R(tracker, 1, src);
	PCT_SET_R(tracker, 2, count);	//dst stride
	
	BenchSQ(&tracker);
	
	pctPrint(&results, count, 1);
	fflush(stdout);
}

void DoPrefBenchmark(void *src) {
	#define PREF_BENCH_CNT	42
	extern const char BenchPrefString[];
	extern void *PrefBenchList[PREF_BENCH_CNT];
	pctTrackerResults32 result_array[PREF_BENCH_CNT];
	pctTracker32 tracker;
	int count = 128;
	
	printf("\nChecking multiple prefetch-to-use timings:\n");
	printf("Pref function settings: \"%s\"\n", BenchPrefString);
	fflush(stdout);
	
	pctInit(&tracker, NULL);
	pctClearResultsMany(result_array, PREF_BENCH_CNT);
	pctInitFunc(&tracker, NULL, &SetupUncachedAll);
	PCT_SET_R(tracker, 0, src);
	PCT_SET_R(tracker, 1, count);
	
	pctBenchmarkMany(&tracker, result_array,  PrefBenchList, PREF_BENCH_CNT, 40);
	
	pctPrintArrayCSVCol(result_array, PREF_BENCH_CNT, 40*count, &PrefTestRunName, &pfcLabel);
	fflush(stdout);
}

int main() {
	void *src = buff0;
	void *dst = buff1;
	if (USE_OCINDEX) {
		sh4EnableOCIndex();
		dst = OCI_BANK1(dst);
	}

	//Benchmark prefetch and FTRV
	DoPrefFtrvBenchmark(src, dst);
	
	//Benchmark memory read/SQ flush interleaving. Modified bench_pref.S
	//Simple benchmark using asm macros for more precise benchmark
	DoSQBenchmark(src, dst);
	
	//Benchmark PREF to use timing
	DoPrefBenchmark(src);
	
	//Benchmark TA
	DoTABenchmark();
	
	fflush(stdout);
	
	return 0;
}


	
