# DCpctTracker32_benchmark
Code and info found at https://dcemulation.org/phpBB/viewtopic.php?p=1060092#p1060092
 SH4's performance counter into a kind of library/framework,

Info from TapamN this is his code and framework and benchmark i uploaded here for safe keeping
Re: High Performance Rendering
Post by TapamN » Wed Jun 28, 2023 7:04 am

I've been turning my benchmark code for the SH4's performance counter into a kind of library/framework, which is included in the attached file. I'm not going to spend the time to fully document and clean it up (This took longer than expected; I want to go back and finish the VGA patches. At least I'm mostly finished with Zelda for now...), but I guess I got it somewhere it might be usable by other people.

Not being able to write to cache during prefetch unfortunately seems correct. It also probably means you can't read the cache when a line is being flushed with OCBWB.

I also found also some kind of delay when switching between read memory accesses and write accesses (I'm talking about real memory accesses, not cache access). Doing (RAM read, RAM write)x4 is much slower than (RAM read)x4, (RAM write)x4. Doing (Readx2, Writex2)x2 is in-between. I got
CODE: SELECT ALL

RRRRWWWW 28.2 cycles
RRWWRRWW 32.2 cycles
RWRWRWRW 39.8 cycles
You can experiement with this by modifying the BenchSQ assembly function in bench_sq.S.

My code for transforming an array of 4D vectors, which could do ~4.8 cycles per vector if source and destination are in cache, would run at ~15.2 cycles per vector if the source was not in cache. I wrote a version that tries to do only cached reads during prefetch, and group prefetches/writes into 64 byte blocks instead of 32, and it manages ~11.4 cycles per vector with uncached source.

Another thing I noticed is that all memory stalls seem to stall the rest of the CPU, like FPU. So if you do a FDIV, then cache miss before the FDIV completes, the FDIV will also stall and make no progress during the cache miss. If you prefetch something, then do a slow instruction like an FDIV, and can't waste enough time before reading the prefetched data to ensure you won't stall, adding manual delays before the read might be helpful.

As for comparing once-per-vertex indexed T&L versus recalculating shared vertices...

I used a low-poly sphere with 330 triangles, with 206 unique vertices. It has long strips and all vertices are shared, so it's biased towards indexed T&L. Something like tree made out of scattered, disconnected quads would have fewer shared vertices and would be biased towards repeated T&L. The sum of the length of all strips for the sphere was 402 vertices.

For the indexed T&L, I transforms the vertex position and wrote the UV coord to a buffer in-cache. Position and UV were stored as SoA. Then from this buffer, each indexed vertex is fetched, perspective is applied (meaning perspective is calculated multiple times for shared vertices, but skipping perspective there doesn't significantly change the speed), then the TA command is written to the SQ and flushed. I used 16-bit indices, with the highest bit marking whether a vertex is the end of the strip or not. No lighting or clipping is done (it doesn't even set the lighting value; I used GL_REPLACE for the TexEnv).

For repeated T&L, a vertex is fetched, position is transformed, perspective if applied, one light + ambient is calculated (disabling the lighting would not speed anything up), and it is submitted to the TA via SQs. The length of each strip is stored in a separate array as 16-bit values. No clipping is done.

In the end, the timing for both versions end up as...
CODE: SELECT ALL

                 T&L     TA Submit   Total (All numbers in cycles)
              (206 vert) (402 vert)  (330 tris)
Indexed  T&L     5053     10986      16039 (fullbright lighting)
Repeated T&L        0     17456      17456 (includes one light + ambient)
For indexed T&L, about 3000 cycles were for transforming the position, and 1850 cycles were for setting the UV coordinate (Which works out to nearly 9 cycles to copy 8 bytes. Yikes, that stupid can't-write-during-prefetch thing is murder. Not using prefetch is only one cycle slower.) Not sure about the missing 150 cycles, maybe some of my notes are bad. The indexed T&L would be slower if any kind of lighting was added (even prelit), there's also going to be some additional overhead from having DMA in the background, and indexed's efficiency will vary depending on how cache friendly everything is arranged, so in a real-world scenario it wouldn't be winning.

Indexed's T&L drops to 1K cycles if the source is already in cache (submit time is unchanged), or about 12K total. Repeated T&L's total drops to 15K cycles if already in cache.

I think there's a bit of room to improve the indexed performance, but it's difficult to tell how much. Those numbers are just what I got with the time I spent, using what I already had available.

I'm not including a working rendering benchmark for indexed vs repeated T&L in the attachment because it relied on a lot of my existing code that take even more time to seperate out, but I included the assembly I used for the indexed T&L (transform_p.S, setuvs_p3.S, and submit.S). The repeated T&L code I used is what's here.

About the library for helping collect data with the SH4's performance counter...

The performance counter on the SH4 can only measure two events at a time. The library reruns the function multiple times measuring a different event each time so it can record all events. It can also measure multiple functions and print their results as CSV. (At first, I tried copy and pasting individual results into a spreadsheet, but that was very error prone.) There are examples in main.c.

There are two ways to measure a function. You can measure a function by passing a function pointer to the benchmark function, or inline the measurement code into an assembly function, which allows for more precise measurements or control over what is measured. You can run the benchmark multiple times so you can get an average to hide noise from anything like SDRAM refresh.

A pctTracker32 struct manages the measurement state, and a pctTrackerResults32 stores the accumulated perf. counter results. They use 32-bit values for the counter results, so some counters (like the cycle counter) could overflow if run for too long. I didn't bother implementing 64-bit versions, since I don't think you need to run the tests for that long.

For both regular functions and functions with inline measurements, you initialize pctTracker32 and pctTrackerResults32 structs, then call pctInitFunc to set up the function called (not necessary if measuring asm function with inline tracker) and an optional setup function, called before the measured function is called. You can use the setup function to setup the cache to a consistent state. There are some convenience functions for flushing/preloading the cache that can be used in your setup functions.

When you use the tracker, it adds the perf counter results to the struct, so you can do multiple runs and average things out later. Make sure to manually clear the pctTrackerResults32 when required.

You can set up the parameters passed to a regular function with PCT_SET_? macros. They set up the r4-r7, fr4-fr11, and some stack parameters passed to the function, so you have match the SH4 calling convention manually. If you use the inline tracker macros, the cache setup still gets called, but the rest of the setup has to be done yourself. You can still use the space in the pctTracker32 struct to pass data, though.

DoPrefFtrvBenchmark in main.c shows how to measure a single, regular function, via function pointer.

bench_sq.S shows how to set up an assembly function for inline measurement. DoSQBenchmark in main.c shows how to call it.

DoPrefBenchmark and DoTABenchmark in main.c show measuring an array of normal functions.

If you want to measure multiple inline measured functions, you have to do that yourself.

It's possible to enable/disable measuring certain events with the pctEvent* functions. For example, if you are measuring the TA by drawing polygons, and you don't want to overfill the vertex buffer, you can disable testing certain events to reduce the number of times your code is called.

pctPrint* functions print results stored in a pctTrackerResults32. You can scale values to print averages. If your function loops over 100 items, and you want to know the average number of events per item, you can pass 100 as the scale value.

There are more explanations in the code and headers, but if you have any questions about using it, just ask. Since I didn't originally plan ahead for others to use it, I had to go back and clean it up, but I don't really feel like putting the effort in to do it fully.

Oh, Ian, I just noticed that you asked about flushing the cache in one of your posts. I guess I missed it (despite being bolded) since it was still in the quote. I wrote something that was a direct replacement for KOS's dcache_flush_range, but looking at it again, I think there might be a problem with it, so I want to double check it before I post it. This library has a function in pctracker.c for flushing and invalidating the entire cache, pctPurgeCache. that might help. It might do more than you need, but it's still probably faster than what KOS has.
