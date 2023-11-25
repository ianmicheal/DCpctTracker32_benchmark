!sh4WriteCCN also flushes and invalidates cache
!I don't think sh4WriteCCN handles flushing/invalidation correctly when
!OCRAM is enabled, but OCINDEX is better in every way, so there's never
!a reason to use OCRAM.
!
!sh4WriteCCN(unsigned int new_ccn);
!sh4EnableOCIndex(void);
!sh4DisableOCIndex(void);

.globl _sh4DisableOCIndex
.globl _sh4EnableOCIndex
.globl _sh4WriteCCN

.set .LCCN_OIX, 0x80
.set .LDATA_CACHELINE_CNT, 512
.set .LCACHELINE_SIZE, 32

_sh4DisableOCIndex:
		!Get existing CCN and clear OIX bit, then call sh4WriteCCN
		mov.l	.LCCN, r5
		mov.l	@r5, r0
		or	#.LCCN_OIX, r0	!Set OIX bit...
		bra	1f
		 xor	#.LCCN_OIX, r0	!...then reverse the OIX bit to clear it

_sh4EnableOCIndex:
		!Get existing CCN and set OIX bit, then call sh4WriteCCN
		mov.l	.LCCN, r5
		mov.l	@r5, r0
		or	#.LCCN_OIX, r0
	1:
		mov	r0, r4
		!Fallthough to sh4WriteCCN
		
_sh4WriteCCN:
		mov.l	.LCCN, r5
		
		!Block IRQs
		mov.l	.LBlockBit, r0
		stc	sr, r7
		or	r7, r0
		ldc	r0, sr
		
		!Flush and invalidate data cache
		mov.l	.LOCTags, r0
		mov	#.LDATA_CACHELINE_CNT>>8, r1
		shll8	r1
		mov	#0, r2
	1:
		mov.l	r2, @r0
		dt	r1
		add	#.LCACHELINE_SIZE, r0
		bf	1b
		
		!Jump to uncached P1 area before writing CCN
		mova	1f, r0
		mov	#0xa0, r1
		shll16	r1
		shll8	r1
		or	r1, r0
		jmp	@r0
		 nop
		
	.align 2
	1:
		!Write to CCN
		mov.l	r4, @r5
		
		!Can't touch cache for a while after writing CCN
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		
		!Restore SR (unblock IRQs)
		ldc	r7, sr
		rts
		 nop
		
	.align 2
	.LCCN:
		.long	0xFF00001C
	.LBlockBit:
		.long	0x10000000
	.LOCTags:
		.long	0xF4000000
