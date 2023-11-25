//extern void TransformVecs(float *src, int srcstride, float *dst, int dststride, int cnt);

#define alignedsp	r1
#define count		r2
#define srcpref		r3
#define src		r4
#define srcstride	r5
#define dst		r6
#define dststride	r7
#define count_mod	r8

//#define PREF(a...)	a
//#define MOVCA(a...)	a
#define PREF(a...)
#define MOVCA(a...)
	
.globl _TransformVecs
TransformVecsStart:
	.Ltv_just_one:
		fmov	@src, dr0
		mov	#8, r0
		fmov	@(r0, src), dr2
		ftrv	xmtrx, fv0
		fmov	dr0, @dst
		bra	.Lexit
		 fmov	dr2, @(r0, dst)
		
	.Ltv_just_two:
		fmov	@src, dr0
		mov	#8, r0
		fmov	@(r0, src), dr2
		add	srcstride, src
		
		fmov	@src, dr4
		fmov	@(r0, src), dr6
		add	srcstride, src
		
		ftrv	xmtrx, fv0
		
		ftrv	xmtrx, fv4
		
		fmov	dr0, @dst
		fmov	dr2, @(r0, dst)
		add	dststride, dst
		
		fmov	dr4, @dst
		bra	.Lexit
		 fmov	dr6, @(r0, dst)
		
	.Lless_than_four:
		mov	count_mod, r0
		cmp/eq	#1, r0
		
		bt	.Ltv_just_one
		cmp/eq	#2, r0
		
		bt	.Ltv_just_two
		
	.Ltv_just_three:
		fmov	@src, dr0
		mov	#8, r0
		fmov	@(r0, src), dr2
		add	srcstride, src
		
		fmov	@src, dr4
		fmov	@(r0, src), dr6
		add	srcstride, src
		
		ftrv	xmtrx, fv0
		fmov	@src, dr8
		
		fmov	@(r0, src), dr10
		add	srcstride, src
		
		ftrv	xmtrx, fv4
		
		fmov	dr0, @dst
		fmov	dr2, @(r0, dst)
		add	dststride, dst
		
		ftrv	xmtrx, fv8
		
		fmov	dr4, @dst
		fmov	dr6, @(r0, dst)
		add	dststride, dst
	
		fmov	dr8, @dst
		bra	.Lexit
		 fmov	dr10, @(r0, dst)

//Function entry
_TransformVecs:
//		pref	@src
		mov	#8, r0

		mov.l	@r15, count
		mov	#-8, alignedsp
		
		mov.l	count_mod, @-r15
		mov	#0x3, count_mod
		
		and	r15, alignedsp
		fschg
		
		fmov	dr12, @-alignedsp
		and	count, count_mod
		
		fmov	dr14, @-alignedsp
		shlr2	count
		
		tst	count, count
	PREF(mov	src, srcpref)
		
	PREF(add	#32, srcpref)
		bt	.Lless_than_four
		
	//Four or more
	PREF(pref	@srcpref)
	PREF(add	#32, srcpref)
		fmov	@src, dr4
				
		fmov	@(r0,src), dr6
		add	srcstride, src
		
		fmov	@src, dr8
		fmov	@(r0,src), dr10
		add	srcstride, src
	
	PREF(pref	@srcpref)
	PREF(add	#32, srcpref)
		ftrv	xmtrx, fv4
		
		fmov	@src, dr12
		fmov	@(r0,src), dr14
		add	srcstride, src

		ftrv	xmtrx, fv8	
//		pref	@src
		
		ftrv	xmtrx, fv12
		
// ********START OF MAIN LOOP********
	1:	//18 cycles per loop (4.5 cycles per vec)
		fmov	@src, dr0
		
		fmov	@(r0,src), dr2
		add	srcstride, src
		
	PREF(pref	@srcpref)
	PREF(add	#32, srcpref)
	MOVCA(movca.l	r0, @dst)
		fmov	dr4, @dst
		
		fmov	dr6, @(r0,dst)
		add	dststride, dst
		
		ftrv	xmtrx, fv0
		fmov	@src, dr4
		
		fmov	@(r0,src), dr6
		add	srcstride, src
		
		fmov	dr8, @dst
		
		fmov	dr10, @(r0,dst)
		add	dststride, dst
		
		ftrv	xmtrx, fv4
		fmov	@src, dr8
		
		fmov	@(r0,src), dr10
		add	srcstride, src
	PREF(pref	@srcpref)
	PREF(add	#32, srcpref)
	MOVCA(movca.l	r0, @dst)
	
		fmov	dr12, @dst
		
		fmov	dr14, @(r0,dst)
		add	dststride, dst
		
		ftrv	xmtrx, fv8
		fmov	@src, dr12
		
		fmov	@(r0,src), dr14
		add	srcstride, src
		
		fmov	dr0, @dst
		dt	count
		
		fmov	dr2, @(r0,dst)
		add	dststride, dst
		
		bf/s	1b
		 ftrv	xmtrx, fv12
		 
// ********END OF MAIN LOOP********
		
	//Remainder
		mov	count_mod, r0
		cmp/eq	#1, r0
		
		bt	.Ltv_mod_one
		cmp/eq	#2, r0
		
		bt	.Ltv_mod_two
		cmp/eq	#3, r0
		
		bt	.Ltv_mod_three
	//Ltv_mod_zero
	.Lexit:
		fmov	@alignedsp+, dr14
		fmov	@alignedsp+, dr12
		fschg
		rts
		 mov.l	@r15+, count_mod
		
	.Ltv_mod_one:
		//fv4
		fmov	dr4, @dst
		mov	#8, r0
		bra	.Lexit
		 fmov	dr6, @(r0, dst)
	.Ltv_mod_two:
		//fv4 fv8
		fmov	dr4, @dst
		mov	#8, r0
		fmov	dr6, @(r0, dst)
		add	dststride, dst
		fmov	dr8, @dst
		bra	.Lexit
		 fmov	dr10, @(r0, dst)
	.Ltv_mod_three:
		//fv4 fv8 fv12
		fmov	dr4, @dst
		mov	#8, r0
		fmov	dr6, @(r0, dst)
		add	dststride, dst
		fmov	dr8, @dst
		fmov	dr10, @(r0, dst)
		add	dststride, dst
		fmov	dr12, @dst
		bra	.Lexit
		 fmov	dr14, @(r0, dst)
		
	
