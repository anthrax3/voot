!   biosfont-lowlevel.s
!
!   $Id: biosfont-lowlevel.s,v 1.3 2002/11/14 20:56:02 quad Exp $
!
! DESCRIPTION
!
!   Accessors to the biosfont section of the Dreamcast BIOS.
!

    .section .text

    .global _bfont_bios

_bfont_bios:
    mov.l   syscall_b4, r0
    mov.l   @r0, r0
    jmp     @r0
    mov     r4, r1

    .align  4

syscall_b4:
    .long   0x8c0000b4
