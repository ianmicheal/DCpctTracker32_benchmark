
/*
#define uvs	r2
#define pos_stride	r3
#define poss	r8
#define pos_stride	r9
#define colors	r10
#define color_stride	r11

	typedef struct VertLayout {
		float *pos;
		size_t pos_stride;
		void *color;
		size_t color_stride;
		float *uv;
		size_t uv_stride;
	};
	void SubmitVertsCust(PVRVertex32 *dst, int vert_cnt, const short *idxs, const VertLayout *vl)
	
	
	typedef struct Vert {
		v4f pos;
		float light[2];
		float uv[2];
	};
	
	Submits vert_cnt vertices. On the final vertex, this function reads one past the end of idxs, and writes
	a TA command past the end of dst. It does NOT submit this vertex.
	So if vert_cnt is 3, it reads and writes 3 vertices properly, reads one extra idx value, and writes a fourth
	TA command long to a fourth vertex, corrupting any memory or SQ there. This is not usually a problem, since
	the SQ is not flushed so it never gets written to the TA or DMA buffer. If writing directly to memory, it's still usually isn't
	a problem because any following vertices will overwrite the excess TA command. Just make that the size of dst is
	(vert_cnt+1) * sizeof(pvr_vertex32)
	
	idx is array of 16-bit indices into vl. Index can range from 0 to 32767, with the high bit signifying end of
	strip. Since clearing bit 15 and higher with the SH4 would require a spare register to store the constant 0x7fff,
	the entire value is inverted instead.
	
	Basically, an index is a one's complement (not two's!) number where the absolute value of the idx is which vertex,
	 and the sign bit controls if the index is the end of the strip
	
	So for two strips of (0, 1, 2, 3) and (5, 4, 3, 0), the idxs would be [0, 1, 2, ~3, 5, 4, 3, ~0]
	Note thaat ~ is used, and not -.
	If we used two's complement, it wouldn't be possible to encode vertex 0 as end of strip, because -0 == 0,
	whereas ~0 != 0
	
	void * SubmitVerts32(pvr_vertex32 *dst, int vert_cnt, const short *idxs, const Vert *vl);
*/

#define APPROX_RECIP	0

#define VERTEX_SIZE	32

#if VERTEX_SIZE == 32
	#define vertex_shift_amt	5
#elif VERTEX_SIZE == 64
	#define vertex_shift_amt	6
#else
	#error Bad VERTEX_SIZE, should be 32 or 64
#endif

//#define raw_idx	r1
#define pos_idx	r2
#define vshift	r3
#define dst	r4
#define vert_cnt	r5
#define idxs	r6
#define verts	r7
#define vert_cmd	r8
#define	vert_cmd_eos	r9
#define	curcmd		r10

#define vx	fr0
#define vy	fr1
#if APPROX_RECIP
#define invw	fr3
#else
#define invw	fr2
#endif

.globl _SubmitVerts32
_SubmitVerts32:
		mov.l	r8, @-r15
		mov	#0xe0, vert_cmd
		mov.l	r9, @-r15
		shll8	vert_cmd
		shll16	vert_cmd
		mov	#-1, vert_cmd_eos
		
		fschg			//***TO DOUBLES***
		mov	#vertex_shift_amt, vshift
	
	//Get first idx
		mov.W	@idxs+, pos_idx
		mov.l	vert_cmd, @dst	//default to normal vert command
		cmp/pz	pos_idx		//is this an end of strip?
		bt	2f
		mov.l	vert_cmd_eos, @dst	//use eos command
		not	pos_idx, pos_idx
	2:	shld	vshift, pos_idx
		
		mov	verts, r0
		bra	1f
		 add	pos_idx, r0
	.align 2
	.Lloop:
		pref	@dst		//flush store queue
		fschg			//***TO DOUBLES***
		add	#28, dst
	1:
		//loop is 21 or 22 cycles with approx recip no cache miss?
	
		fmov.d	@r0+, dr0	//pos
		fmov.d	@r0+, dr2
#if APPROX_RECIP
		fmov.d	@r0+, dr4	//color
		fmul	invw, invw
#else
		fldi1	invw
		fdiv	fr3, invw
		fmov.d	@r0+, dr4	//color
#endif
		
	//Get next idx during divide, write TA command for next vertex
		mov.W	@idxs+, pos_idx
		//pref	@idxs
		add	#32, dst
		mov.l	vert_cmd, @(0, dst)	//default to normal vert command
		fmov.d	@r0+, dr6	//uv
		cmp/pz	pos_idx		//is this an end of strip?
#if APPROX_RECIP
		fsrra	invw
#endif
		bt	2f
		mov.l	vert_cmd_eos, @(0, dst)	//use eos command
		not	pos_idx, pos_idx
	2:	
		mov	verts, r0
		shld	vshift, pos_idx
		
		add	pos_idx, r0
		//pref	@r0
		
		
		fmov.d	dr6, @-dst	//uv
		fmov.d	dr4, @-dst	//color
		fschg			//***TO SINGLES***
		
		fmov.s	fr2, @-dst	//inv w
		fmul	invw, vy
		fmul	invw, vx
	
		dt	vert_cnt
		fmov.s	fr1, @-dst	//scry
		bf/s	.Lloop
		 fmov.s	fr0, @-dst	//scrx
		
		
		mov.l	@r15+, r9
		//fschg			//***TO SINGLES***
		mov.l	@r15+, r8
		mov	dst, r0
		pref	@r0		//flush store queue
		rts
		 add	#28, r0
		 
	.align 5
	.Lvert_cmd:
		.long 0xe0000000

