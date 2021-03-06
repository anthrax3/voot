/*  system.h

    $Id: system.h,v 1.10 2002/12/17 11:31:57 quad Exp $

*/

#ifndef __COMMON_SYSTEM_H__
#define __COMMON_SYSTEM_H__

#include "vars.h"

/* NOTE: The UBC register bases. */

#define _UBC_BASE       (0xff200000)
#define _UBC_ASID_BASE  (0xff000000)

/*
    NOTE: Abbreviation expansion...

    BAR?    - Break Address Register
    BAMR?   - Break Address Mask Register
    BBR?    - Break Bus Cycle Register (rules)

    BRCR    - Break Control Register (global rules)
*/

/* NOTE: UBC channel A registers. */

#define UBC_R_BARA      (REGISTER(int)      (_UBC_BASE + 0x00))
#define UBC_R_BAMRA     (REGISTER(char)     (_UBC_BASE + 0x04))
#define UBC_R_BBRA      (REGISTER(short)    (_UBC_BASE + 0x08))

/* NOTE: UBC channel B registers. */

#define UBC_R_BARB      (REGISTER(int)      (_UBC_BASE + 0x0C))
#define UBC_R_BAMRB     (REGISTER(char)     (_UBC_BASE + 0x10))
#define UBC_R_BBRB      (REGISTER(short)    (_UBC_BASE + 0x14))

/* NOTE: UBC global register. */

#define UBC_R_BRCR      (REGISTER(short)    (_UBC_BASE + 0x20))

/* NOTE: Exception/Interrupt registers. */

#define REG_EXPEVT          (REGISTER(uint32) (0xff000024))
#define REG_INTEVT          (REGISTER(uint32) (0xff000028))
#define REG_TRA             (REGISTER(uint32) (0xff000020))

/* NOTE: Exception types from the lowlevel handler. */

#define EXP_TYPE_GEN        1
#define EXP_TYPE_CACHE      2
#define EXP_TYPE_INT        3
#define EXP_TYPE_ALL        255

/* NOTE: SH4 exception codes. */

#define EXP_CODE_INT9       0x320
#define EXP_CODE_INT11      0x360
#define EXP_CODE_INT13      0x3A0
#define EXP_CODE_TRAP       0x160
#define EXP_CODE_UBC        0x1E0
#define EXP_CODE_RXI        0x720
#define EXP_CODE_ALL        0xFFE
#define EXP_CODE_BAD        0xFFF

/* NOTE: VBR vectors. */

#define VBR_GEN(tab)    ((void *) ((unsigned int) tab) + 0x100)
#define VBR_CACHE(tab)  ((void *) ((unsigned int) tab) + 0x400)
#define VBR_INT(tab)    ((void *) ((unsigned int) tab) + 0x600)

/* NOTE: System-level memory ranges. */

#define SYS_MEM_START       0x8C000000
#define SYS_MEM_END         0x8CFFFFFF

typedef struct
{
    uint32  pr;
    uint32  mach;
    uint32  macl;

    uint32  spc;
    uint32  ssr;

    uint32  vbr;
    uint32  gbr;
    uint32  sr;
    uint32  dbr;

    uint32  r7_bank;
    uint32  r6_bank;
    uint32  r5_bank;
    uint32  r4_bank;
    uint32  r3_bank;
    uint32  r2_bank;
    uint32  r1_bank;
    uint32  r0_bank;

    float   fr0_b;
    float   fr1_b;
    float   fr2_b;
    float   fr3_b;
    float   fr4_b;
    float   fr5_b;
    float   fr6_b;
    float   fr7_b;
    float   fr8_b;
    float   fr9_b;
    float   fr10_b;
    float   fr11_b;
    float   fr12_b;
    float   fr13_b;
    float   fr14_b;
    float   fr15_b;

    float   fr0_a;
    float   fr1_a;
    float   fr2_a;
    float   fr3_a;
    float   fr4_a;
    float   fr5_a;
    float   fr6_a;
    float   fr7_a;
    float   fr8_a;
    float   fr9_a;
    float   fr10_a;
    float   fr11_a;
    float   fr12_a;
    float   fr13_a;
    float   fr14_a;
    float   fr15_a;

    uint32  fpscr;
    uint32  fpul;

    uint32  r14;
    uint32  r13;
    uint32  r12;
    uint32  r11;
    uint32  r10;
    uint32  r9;
    uint32  r8;
    uint32  r7;
    uint32  r6;
    uint32  r5;
    uint32  r4;
    uint32  r3;
    uint32  r2;
    uint32  r1;

    uint32  exception_type;
    uint32  r0;
} register_stack;

/* NOTE: External definitions. */

extern void *   dbr         (void);
extern void     dbr_set     (const void *set);

extern void *   sgr         (void);

extern void     flush_cache (void);

extern void     ubc_wait    (void);

#endif
