/*
* mpatmplt.h                                                Version 5.4.0
*
* Memory Protection Array template definitions.
*
* INCLUDE ONLY IN FILES THAT DEFINE MPA TEMPLATES since this uses
* very short macro names which could cause strange compiler errors.
*
* Copyright (c) 2016-2025 Micro Digital Inc.
* All rights reserved. www.smxrtos.com
*
* This software, documentation, and accompanying materials are made available
* under the Apache License, Version 2.0. You may not use this file except in
* compliance with the License. http://www.apache.org/licenses/LICENSE-2.0
*
* SPDX-License-Identifier: Apache-2.0
*
* This Work is protected by patents listed in smx.h. A patent license is
* granted according to the License above. This entire comment block must be
* preserved in all copies of this file.
*
* Support services are offered by MDI. Inquire at support@smxrtos.com.
*
* Author: Ralph Moore
*
*****************************************************************************/

#ifndef SB_MPATMPLT_H
#define SB_MPATMPLT_H

#if SB_CPU_ARMM7

/* region attribute fields */
#define EN           1     /* region enable */
#define N0     (1 << 8)    /* subregion 0 disable */
#define N1     (1 << 9)    /* subregion 1 disable */
#define N2     (1 << 10)   /* subregion 2 disable */
#define N3     (1 << 11)   /* subregion 3 disable */
#define N4     (1 << 12)   /* subregion 4 disable */
#define N5     (1 << 13)   /* subregion 5 disable */
#define N6     (1 << 14)   /* subregion 6 disable */
#define N7     (1 << 15)   /* subregion 7 disable */
#define B      (1 << 16)   /* bufferable */
#define C      (1 << 17)   /* cacheable */
#define S      (1 << 18)   /* shareable */
#define T0     (1 << 19)   /* TEX bit 0 */
#define PRW    (1 << 24)   /* priv read/write */
#define PRWRO  (2 << 24)   /* priv read/write, read only */
#define RW     (3 << 24)   /* read/write */
#define PRO    (5 << 24)   /* priv read only */
#define RO     (6 << 24)   /* read only */
#define XN     (1 << 28)   /* execute never */
#define V      (1 << 4)    /* region valid */

/* standard region attributes */
#define DATARW    (XN |  RW | C) /* read/write data <1> */
#define DATARO    (XN |  RO | C) /* read only data */
#define CODE      (      RO | C) /* code */
#define IOR       (XN |  RW)     /* read/write data non-cacheable */
#define PDATARW   (XN | PRW | C) /* priv read/write data */
#define PDATARO   (XN | PRO | C) /* priv read only data */
#define PCODE     (     PRO | C) /* priv code */
#define PIOR      (XN | PRW)     /* priv read/write data non-cacheable */

/* write back and write allocate for dcache */
#define WBWA      (T0 | C | B)

#define NZ(s)     (31-__CLZ(SS(s)))          /* number of leading 0's */
#define RA(s)     ((u32)__section_begin(s))  /* region address */
#define SS(s)     ((u32)__section_size(s))   /* section size */

/* contiguous subregion end ranges */
#define N67       (N6 | N7)
#define N57       (N5 | N67)
#define N47       (N4 | N57)
#define N37       (N3 | N47)
#define N27       (N2 | N37)
#define N17       (N1 | N27)

/* subregion disables based upon section size */
#define SRD(s)    ((SS(s)%5==0 ? N57 : (SS(s)%6==0 ? N67 : (SS(s)%7==0 ? N7 : 0))))

/* region size index */
#define RSI(s)    ((u32)((SS(s)&(~(1<<NZ(s)))) ? NZ(s) : NZ(s)-1)<<1) /* from section size */
#define RSIC(c)   ((u32)(30-__CLZ((u32)&c))<<1) /* from constant */
#define RSIN(n)   ((u32)(30-__CLZ((u32)n))<<1)  /* from number */

/* region format */
#if MP_MPA_DEV
#define RGN(rbar, rasr, name) {rbar, rasr, name}
#else
#define RGN(rbar, rasr, name) {rbar, rasr}
#endif

/* region sizes defined in linker command file */
extern u32 cpcsz;
extern u32 cpdsz;
extern u32 fpucsz;
extern u32 fpudsz;
extern u32 fscsz;
extern u32 fsdsz;
extern u32 fpdcsz;
extern u32 fpddsz;
extern u32 lcdbufsz;
extern u32 lcdcsz;
extern u32 lcddsz;
extern u32 ledcsz;
extern u32 leddsz;
extern u32 scsz;
extern u32 sdsz;
extern u32 opconcsz;
extern u32 opcondsz;
extern u32 ucomcsz;
extern u32 ucomdsz;

#elif SB_CPU_ARMM8

/* region attribute fields */
#define EN           1     /* region enable */
#define AF0       0x44     /* MAIR field 0: normal memory, non-cacheable */
#define AF1        0x4     /* MAIR field 1: device I/O, nGnRE */
#define AF2          0     /* MAIR field 2: define AF2-7, as needed */
#define AF3          0     /* MAIR field 3 */
#define AF4          0     /* MAIR field 4 */
#define AF5          0     /* MAIR field 5 */
#define AF6          0     /* MAIR field 6 */
#define AF7          0     /* MAIR field 7 */
#define SHN    (0 << 3)    /* non-shareable */
#define SHI    (2 << 3)    /* outer shareable */
#define SHO    (3 << 3)    /* inner shareable */
#define XN           1     /* execute never */
   
/* standard region attributes <2> */
#define DATARW    0x3      /* read/write data << 1 | XN */
#define DATARO    0x7      /* read only data << 1 | XN */
#define CODE      0x6      /* code */
#define IOR       0x3      /* read/write data */
#define PDATARW   0x1      /* priv read/write data << 1 | XN */
#define PDATARO   0x5      /* priv read only data << 1 | XN */
#define PCODE     0x4      /* priv code */
#define PIOR      0x1      /* priv read/write data */

/* region starting address and last address */
#define RA(s)     ((u32)__section_begin(s))             /* region address */
#define RLA(s)    (RA(s)+((u32)__section_size(s)-0x20)) /* +0x1F = region last address */
#define AI(a)     ((a)<<1)                      /* attribute index */

/* region format */
#if MP_MPA_DEV /*<4>*/
#define RGN(n, rbar, rlar, name) {rbar, rlar, name}
#else
#define RGN(n, rbar, rlar, name) {rbar, rlar}
#endif
#endif   /* SB_CPU_ARMM7/8 */

/* Sections for MPA templates <3> */
#pragma section = "cp_code"
#pragma section = "cp_data"
#pragma section = "EVB"
#pragma section = "fpu_code"
#pragma section = "fpu_data"
#pragma section = "fs_code"
#pragma section = "fs_data"
#pragma section = "fpd_code"
#pragma section = "fpd_data"
#pragma section = "lcd_code"
#pragma section = "lcd_data"
#pragma section = "LCD_BUF"
#pragma section = "led_code"
#pragma section = "led_data"
#pragma section = "opcon_code"
#pragma section = "opcon_data"
#pragma section = "ram_block"
#pragma section = "rom_block"
#pragma section = "rom_block_all"
#pragma section = "sram_block"
#pragma section = "sram_block_all"
#pragma section = "rom_cblock"
#pragma section = "sram_cblock"
#pragma section = "svc_code"
#pragma section = "svc_data"
#pragma section = "sys_code"
#pragma section = "sys_data"
#pragma section = "ucom_code"
#pragma section = "ucom_data"

#endif /* SB_MPATMPLT_H */

/* Notes:
   1. TEXCB = b00010 defines normal memory. This is necessary to avoid alignment 
      problems for some memory buses. For external RAM setting TEXCB = b00011
      enables the write buffer for faster performance and also defines normal
      memory.
   2. Combine with cache and device attributes.
   3. Defined as blocks in linker command file but as sections here.
   4. n is used only for readability.
*/
