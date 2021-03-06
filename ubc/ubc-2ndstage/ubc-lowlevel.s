    .section .text

    .global _ubc_wait
    .global _ubc_works

_ubc_wait:
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    rts
    nop

_ubc_works:
!BEGIN REGISTER SAVE
!GENERAL
    mov.l   r0, @-r15
    mov.l   r1, @-r15
    mov.l   r2, @-r15
    mov.l   r3, @-r15
    mov.l   r4, @-r15
    mov.l   r5, @-r15
    mov.l   r6, @-r15
    mov.l   r7, @-r15
    mov.l   r8, @-r15
    mov.l   r9, @-r15
    mov.l   r10, @-r15
    mov.l   r11, @-r15
    mov.l   r12, @-r15
    mov.l   r13, @-r15
    mov.l   r14, @-r15
!CONTROL
    stc.l   r0_bank, @-r15
    stc.l   r1_bank, @-r15
    stc.l   r2_bank, @-r15
    stc.l   r3_bank, @-r15
    stc.l   r4_bank, @-r15
    stc.l   r5_bank, @-r15
    stc.l   r6_bank, @-r15
    stc.l   r7_bank, @-r15
    stc.l   dbr, @-r15
    stc.l   sr, @-r15
    stc.l   gbr, @-r15
    stc.l   vbr, @-r15
!SAVED CONTROL (exception)
!    stc.l   ssr, @-r15
!    stc.l   sgr, @-r15
!    stc.l   spc, @-r15
!SYSTEM
    sts.l   macl, @-r15
    sts.l   mach, @-r15
    sts.l   pr, @-r15
    sts.l   fpul, @-r15
    sts.l   fpscr, @-r15
!FLOATING POINT
    ! Initialize the FP registers for saving
!    mov     #0, r0          ! Set known FP flags
!    lds     r0, fpscr
    ! Save the first bank
!    fmov.s  fr0, @-r15
!    fmov.s  fr1, @-r15
!    fmov.s  fr2, @-r15
!    fmov.s  fr3, @-r15
!    fmov.s  fr3, @-r15
!    fmov.s  fr4, @-r15
!    fmov.s  fr5, @-r15
!    fmov.s  fr6, @-r15
!    fmov.s  fr7, @-r15
!    fmov.s  fr8, @-r15
!    fmov.s  fr9, @-r15
!    fmov.s  fr10, @-r15
!    fmov.s  fr11, @-r15
!    fmov.s  fr12, @-r15
!    fmov.s  fr13, @-r15
!    fmov.s  fr14, @-r15
!    fmov.s  fr15, @-r15
    ! Switch to the second bank
!    frchg
    ! Now save the second bank
!    fmov.s  fr0, @-r15
!    fmov.s  fr1, @-r15
!    fmov.s  fr2, @-r15
!    fmov.s  fr3, @-r15
!    fmov.s  fr3, @-r15
!    fmov.s  fr4, @-r15
!    fmov.s  fr5, @-r15
!    fmov.s  fr6, @-r15
!    fmov.s  fr7, @-r15
!    fmov.s  fr8, @-r15
!    fmov.s  fr9, @-r15
!    fmov.s  fr10, @-r15
!    fmov.s  fr11, @-r15
!    fmov.s  fr12, @-r15
!    fmov.s  fr13, @-r15
!    fmov.s  fr14, @-r15
!    fmov.s  fr15, @-r15
    ! Back to the first bank
!    frchg
!END REGISTER SAVE

    ! Arguments are passed on r4
    mov     r15, r4

    mov.l   hdl_except, r2
    jsr     @r2
    nop

!BEGIN REGISTER RESTORE
!FLOATING POINT
    ! Initialize the FP registers for saving
!    mov     #0, r0          ! Set known FP flags
!    lds     r0, fpscr
    ! Load the second bank first
!    frchg
!    fmov.s  @r15+, fr15
!    fmov.s  @r15+, fr14
!    fmov.s  @r15+, fr13
!    fmov.s  @r15+, fr12
!    fmov.s  @r15+, fr11
!    fmov.s  @r15+, fr10
!    fmov.s  @r15+, fr9
!    fmov.s  @r15+, fr8
!    fmov.s  @r15+, fr7
!    fmov.s  @r15+, fr6
!    fmov.s  @r15+, fr5
!    fmov.s  @r15+, fr4
!    fmov.s  @r15+, fr3
!    fmov.s  @r15+, fr2
!    fmov.s  @r15+, fr1
!    fmov.s  @r15+, fr0
    ! Now back to the first bank
!    frchg
!    fmov.s  @r15+, fr15
!    fmov.s  @r15+, fr14
!    fmov.s  @r15+, fr13
!    fmov.s  @r15+, fr12
!    fmov.s  @r15+, fr11
!    fmov.s  @r15+, fr10
!    fmov.s  @r15+, fr9
!    fmov.s  @r15+, fr8
!    fmov.s  @r15+, fr7
!    fmov.s  @r15+, fr6
!    fmov.s  @r15+, fr5
!    fmov.s  @r15+, fr4
!    fmov.s  @r15+, fr3
!    fmov.s  @r15+, fr2
!    fmov.s  @r15+, fr1
!    fmov.s  @r15+, fr0
!SYSTEM
    lds.l   @r15+, fpscr
    lds.l   @r15+, fpul
    lds.l   @r15+, pr
    lds.l   @r15+, mach
    lds.l   @r15+, macl
!CONTROL
    ldc.l   @r15+, vbr
    ldc.l   @r15+, gbr
    ldc.l   @r15+, sr
    ldc.l   @r15+, dbr
    ldc.l   @r15+, r7_bank
    ldc.l   @r15+, r6_bank
    ldc.l   @r15+, r5_bank
    ldc.l   @r15+, r4_bank
    ldc.l   @r15+, r3_bank
    ldc.l   @r15+, r2_bank
    ldc.l   @r15+, r1_bank
    ldc.l   @r15+, r0_bank
!GENERAL
    mov.l   @r15+, r14
    mov.l   @r15+, r13
    mov.l   @r15+, r12
    mov.l   @r15+, r11
    mov.l   @r15+, r10
    mov.l   @r15+, r9
    mov.l   @r15+, r8
    mov.l   @r15+, r7
    mov.l   @r15+, r6
    mov.l   @r15+, r5
    mov.l   @r15+, r4
    mov.l   @r15+, r3
    mov.l   @r15+, r2
    mov.l   @r15+, r1
    mov.l   @r15+, r0
!END REGISTER RESTORE

    ! Only UBC exceptions do this (in case we missed recovering the stack)
    stc     sgr, r15
    rte
    nop

! All 32-bit reference variables go after this. They need to be 4-byte
! aligned, I guess.
.align 4
hdl_except:
    .long   _ubc_break_work
cool_value:
    .long   0x12345678
