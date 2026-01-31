/*
* mparmm.h                                                  Version 6.0.0
*
* MPU ARMM definitions.
*
* Copyright (c) 2016-2026 Micro Digital Inc.
* All rights reserved. www.smxrtos.com
*
* SPDX-License-Identifier: GPL-2.0-only OR LicenseRef-MDI-Commercial
*
* This software, documentation, and accompanying materials are made available
* under a dual license, either GPLv2 or Commercial. You may not use this file
* except in compliance with either License. GPLv2 is at www.gnu.org/licenses.
* It does not permit the incorporation of this code into proprietary programs.
*
* Commercial license and support services are available from Micro Digital.
* Inquire at support@smxrtos.com.
*
* This Work embodies patents listed in smx.h. A patent license is hereby
* granted to use these patents in this Work and Derivative Works, except in
* another RTOS or OS.
*
* This entire comment block must be preserved in all copies of this file.
*
* Author: Ralph Moore
*
*****************************************************************************/

#ifndef SB_MPU_H
#define SB_MPU_H

/*===========================================================================*
*                                 CONSTANTS                                  *
*===========================================================================*/

#define MP_MPA_DEV      1  /* 1 for development, 0 for better performance */

#if MP_MPA_DEV
#define MP_MPR_SZ       3  /* (in words) */
#else
#define MP_MPR_SZ       2  /* (in words) */
#endif

/* MPU slot allocation -- must agree with xarmm_iar.inc */
#if (defined(SMX_TSMX) && SB_CPU_ARMM7)
#define MP_MPU_ACTVSZ   6  /* TSMX setting; don't change */
#define MP_MPU_STATSZ   2  /* TSMX setting; don't change */
#else
#define MP_MPU_ACTVSZ   8  /* number of slots */
#define MP_MPU_STATSZ   0  /* number of slots */
#endif

#define MP_MPU_FAS   MP_MPU_STATSZ                        /* first active slot */
#define MP_MPU_SZ    (MP_MPU_STATSZ + MP_MPU_ACTVSZ)      /* MPU size */
#define MP_TMSK_DFLT (0xFFFFFFFF >> (32 - MP_MPU_ACTVSZ)) /* default test mask */

#if SB_CPU_ARMM7

#define MP_DRT       0x80000000 /* dynamic region test for RASR -- bit31 == 1 */
#define MP_DATARW    ((1 << 28) | (3 << 24) | (1 << 17))  /* XN |  RW | C  */
#define MP_DATARO    ((1 << 28) | (6 << 24) | (1 << 17))  /* XN |  RO | C  read only data */
#define MP_CODE      (            (6 << 24) | (1 << 17))  /*       RO | C  code */
#define MP_EN        (1 << 0)                             /* region enable */
#define MP_IOR       ((1 << 28) | (3 << 24))              /* XN | PRW      read/write data non-cacheable */
#define MP_PDATARW   ((1 << 28) | (1 << 24) | (1 << 17))  /* XN | PRW | C  priv read/write data */
#define MP_PDATARO   ((1 << 28) | (5 << 24) | (1 << 17))  /* XN | PRO | C  priv read only data */
#define MP_PCODE     (            (5 << 24) | (1 << 17))  /*      PRO | C  priv code */
#define MP_PIOR      ((1 << 28) | (1 << 24))              /* XN | PRW      priv read/write data non-cacheable */
#define MP_V         (1 << 4)    /* region valid */

#elif SB_CPU_ARMM8

#define MP_DRT       0xFFFFFFFF /* dynamic region test for RLAR -- illegal value */
#define MP_DATARW    ((000<<5) | (00<<3) | (01<<1) | 1) /* AI | SH | AP | XN */
#define MP_DATARO    0x7        /* read only data << 1 | XN */
#define MP_CODE      0x6        /* code */
#define MP_EN        (1 << 0)   /* region enable */
#define MP_IOR       0x3        /* read/write data */
#define MP_PDATARW   0x1        /* priv read/write data << 1 | XN */
#define MP_PDATARO   0x5        /* priv read only data << 1 | XN */
#define MP_PCODE     0x4        /* priv code */
#define MP_PIOR      0x1        /* priv read/write data */
#endif

/*===========================================================================*
*                                  OTHER                                     *
*===========================================================================*/

/* MPU macros */
#define mp_MPA_PTR(t, n) ((u32*)t->mpap + (MP_MPR_SZ)*n) /* MPA slot n pointer */
#define mp_DYN_RGN(dyn)  {(u32)&dyn, MP_DRT}             /* dynamic region */

typedef struct {        /* MEMORY PROTECTION REGION */
  #if SB_CPU_ARMM7
   u32  rbar;              /* region address and slot */
   u32  rasr;              /* region attributes and size */
  #elif SB_CPU_ARMM8
   u32  rbar;              /* region address, shareability, and access permissions */
   u32  rlar;              /* region last address, attributes index, and EN */
  #endif
  #if MP_MPA_DEV
   const char *name;       /* region name (for debugging) */
  #endif
} MPR, *MPR_PTR;

typedef MPR MPA[];         /* Memory Protection Array */
extern  MPA  mpa_dflt;     /* default MPA */

#endif /* SB_MPU_H */
