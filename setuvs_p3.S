//Copies 8 bytes from src to dst, trys to minimize write stalls during prefetches
//Stride of src is assumed to be 8, Stride of dst is assumed to be 32
//vert_cnt must be greater than 0
//Execution time without cache misses is about vert_cnt*10/4+some
//void SetUVs32Pref3(float *dst, float *src, size_t vert_cnt);

#define oddcnt	r1
#define srcpref	r3
#define dst	r4
#define src	r5
#define vert_cnt	r6

#define PREF(reg)	pref	@reg
//#define PREF(reg)	
//#define PREF(reg)	nop

.globl _SetUVs32Pref3
.align 5
_SetUVs32Pref3:
		fmov	fr12, @-r15
		fmov	fr13, @-r15
		fmov	fr14, @-r15
		fmov	fr15, @-r15
		fschg
		PREF(src)
		
		mov	src, srcpref
		add	#32, srcpref
		
		mov	vert_cnt, r0
		shlr	vert_cnt
		
		and	#1, r0
		tst	vert_cnt, vert_cnt
		
		mov	r0, r1
		mov	#32, r0
		bt	.Lless_than_four
	.align 2
	1:
		PREF(srcpref)
		add	#32, srcpref
		
		fmov	@src+, dr0
		dt	vert_cnt
		
		fmov	@src+, dr2
		bt	.Lwrite2_then_less_than_four
		
		fmov	@src+, dr4
		dt	vert_cnt
		
		fmov	@src+, dr6
		bt	.Lwrite4_then_less_than_four
		
		fmov	@src+, dr8
		dt	vert_cnt
		
		fmov	@src+, dr10
		bt	.Lwrite6_then_less_than_four
		
		fmov	@src+, dr12
		dt	vert_cnt
		
		fmov	@src+, dr14
		
		fmov	dr0, @dst
		
		fmov	dr2, @(r0, dst)
		add	#64, dst
				
		fmov	dr4, @dst
		
		fmov	dr6, @(r0, dst)
		add	#64, dst
		
		fmov	dr8, @dst
		
		fmov	dr10, @(r0, dst)
		add	#64, dst
				
		fmov	dr12, @dst
		
		fmov	dr14, @(r0, dst)
		add	#64, dst
		
		PREF(srcpref)
		bf/s	1b
		 add	#32, srcpref
	
	.Lless_than_four:
		tst	r1, r1
		fmov	@src+, dr0
		bt	2f
	1:
		dt	r1
		fmov	dr0, @dst
		bf/s	1b
		 fmov	@src+, dr0
	2:
		fschg
		fmov	@r15+, fr15
		fmov	@r15+, fr14
		fmov	@r15+, fr13
		rts
		 fmov	@r15+, fr12
	
	.align 2
	.Lwrite2_then_less_than_four:
		fmov	dr0, @dst
		fmov	dr2, @(r0, dst)
		bra	.Lless_than_four
		 add	#64, dst
	.Lwrite4_then_less_than_four:
		fmov	dr0, @dst
		fmov	dr2, @(r0, dst)
		add	#64, dst
		fmov	dr4, @dst
		fmov	dr6, @(r0, dst)
		bra	.Lless_than_four
		 add	#64, dst
	.Lwrite6_then_less_than_four:
		fmov	dr0, @dst
		fmov	dr2, @(r0, dst)
		add	#64, dst
		fmov	dr4, @dst
		fmov	dr6, @(r0, dst)
		add	#64, dst
		fmov	dr8, @dst
		fmov	dr10, @(r0, dst)
		bra	.Lless_than_four
		 add	#64, dst

		
		
