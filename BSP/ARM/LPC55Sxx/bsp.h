/*
* bsp.h                                                     Version 6.0.0
*
* Board Support Package API Header for NXP LPC55Sxx processors.
* All are similar so just use conditionals here for differences.
*
* Contains BSP-specific defines, types, prototypes, and configuration
* settings.
*
* See XBASE\bbsp.h.
*
* Copyright (c) 2002-2026 Micro Digital Inc.
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
* Author: Xingsheng Wan
*
*****************************************************************************/

#ifndef SMX_BSP_H
#define SMX_BSP_H

#include "armdefs.h"  /* for processor defines */
#include "uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Configuration */

#define SB_CPU_HZ        (CPU_FREQ)      /* CPU Clock */
#define SB_PERIPH_HZ     (PERIPH_FREQ)   /* Master Clock */

/* SB_CPU_MHZ: 3 variants are provided for fractional clock rates.
   Use the one that rounds properly for each purpose.
*/
#define SB_CPU_MHZ_DOWN  ((SB_CPU_HZ) / 1000000)             /* round down */
#define SB_CPU_MHZ_RND   (((SB_CPU_HZ) + 500000) / 1000000)  /* round to nearest */
#define SB_CPU_MHZ_UP    (((SB_CPU_HZ) + 999999) / 1000000)  /* round up */

/* Console Configuration. See documentation in XBASE\bbsp.h. */

#define SB_CON_BAUD 115200

#if defined(SB_BRD_NXP_LPC55S69_EVK)
#define SB_CON_IN_PORT       0  /* ports are numbered at 0 (see comment above) */
#define SB_CON_OUT_PORT      0  /* ports are numbered at 0 (see comment above) */
#else
#error Define SB_CFG_CON settings for your board here in bsp.h.
#endif

#define SB_LCD             0     /* 1 if board has LCD display. 0 if not. */

/* Defines */

#define SB_INT_MIN         0
#define SB_INT_MAX         (SB_IRQ_MAX+16)
#define SB_INT_NUM         (SB_INT_MAX-SB_INT_MIN+1)

#define SB_IRQ_MIN         0     /* see irq_table[] in irqtable.c */
#define SB_IRQ_MAX         59

#define SB_IRQ_NUM         (SB_IRQ_MAX-SB_IRQ_MIN+1)
#define SB_IRQ_PRI_MIN     INTC_VIC_MIN_PRIORITY
#define SB_IRQ_PRI_MAX     INTC_VIC_MAX_PRIORITY

#define SB_IRQ_PRI_BITS    0xE0  /* only 3 bits 7:5 are used for priority */
#define SB_IRQ_PRI_INCR    0x20  /* priority increment */

/* Max number of interrupt mask registers */
#define SB_IRQ_MASK_REGS   ((SB_IRQ_MAX + 1 + 31) / 32)

#define SB_TICK_IRQ        -1    /* Dummy value. smx tick uses SysTick which is not in irq_table[]. */

#define SB_TICK_TMR_COUNTS_PER_TICK ((SB_CPU_HZ)/(SMX_TICKS_PER_SEC))

#pragma section = "EVT" /* defined in .icf file */

/* Typedefs */

typedef struct
{
   u8  pri;            /* interrupt priority */
   /* no need for vector number since easy to calculate from IRQ */
} SB_IRQ_REC;

typedef union { ISR_PTR __fun; void * __ptr; } intvec_elem;

/* Function Prototypes */

bool sb_IRQTableEntryWrite(int irq_num, int pri);

#ifdef __cplusplus
}
#endif

#endif /* SMX_BSP_H */
