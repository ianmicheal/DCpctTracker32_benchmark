//extern void TransformVecs(float *src, float *dst, int dststride, int cnt);

#define alignedsp	r1
#define srcpref		r3
#define src		r4
#define dst		r5
#define dststride	r6
#define count		r7

//#define PREF(a...)	a
//#define MOVCA(a...)	a
#define PREF(a...)
#define MOVCA(a...)
	
.globl _TransformVecsP
TransformVecsPStart:
	

//Function entry
_TransformVecsP:
		
		mov	#-8, alignedsp
		fschg
		
		and	r15, alignedsp
		fmov	dr12, @-alignedsp
		
		mov	src, srcpref
		fmov	dr14, @-alignedsp
		
		pref	@srcpref	//pref fv0/fv4
		add	#32, srcpref
		
		mov	#8, r0
		
		pref	@srcpref	//pref fv8/fv12
	.align 2
	1:
		add	#32, srcpref
		fmov	@src+, dr0
		
		dt	count
		fmov	@src+, dr2
		
		fmov	@src+, dr4
		
		fmov	@src+, dr6
		
		bt/s	.Lwrite_one
		 ftrv	xmtrx, fv0
		 
		dt	count
		pref	@srcpref	//pref next fv0/fv4
		
		add	#32, srcpref
		bt/s	.Lwrite_two
				
		 ftrv	xmtrx, fv4
		dt	count
		
		fmov	@src+, dr8
		
		fmov	@src+, dr10
		
		fmov	@src+, dr12
		
		fmov	@src+, dr14
		bt/s	.Lwrite_three
		 ftrv	xmtrx, fv8
		
		dt	count
		
		ftrv	xmtrx, fv12
		fmov	dr0, @dst
		
		fmov	dr2, @(r0,dst)
		add	dststride, dst
		
		fmov	dr4, @dst
		
		fmov	dr6, @(r0,dst)
		add	dststride, dst
		
		fmov	dr8, @dst
		
		fmov	dr10, @(r0,dst)
		add	dststride, dst
		
		fmov	dr12, @dst
		
		fmov	dr14, @(r0,dst)
		
		pref	@srcpref	//pref fv8/fv12
		bf/s	1b
		 add	dststride, dst
		
		
	.Lexit:
		fmov	@alignedsp+, dr14
		fmov	@alignedsp+, dr12
		rts
		 fschg
	
	.align 2
	.Lwrite_one:
		fmov	dr0, @dst
		bra	.Lexit
		 fmov	dr2, @(r0, dst)
	.Lwrite_two:
		//ftrv	xmtrx, fv4
		fmov	dr0, @dst
		fmov	dr2, @(r0, dst)
		add	dststride, dst
		fmov	dr4, @dst
		bra	.Lexit
		 fmov	dr6, @(r0, dst)
	.Lwrite_three:
		//ftrv	xmtrx, fv8
		fmov	dr0, @dst
		fmov	dr2, @(r0, dst)
		add	dststride, dst
		fmov	dr4, @dst
		fmov	dr6, @(r0, dst)
		add	dststride, dst
		fmov	dr8, @dst
		bra	.Lexit
		 fmov	dr10, @(r0, dst)
