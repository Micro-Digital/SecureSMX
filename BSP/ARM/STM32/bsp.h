/*
* bsp.h                                                     Version 5.4.0
*
* Board Support Package API Header for STMicro STM32 processors.
* All are similar so just use conditionals here for differences.
*
* Contains BSP-specific defines, types, prototypes, and configuration
* settings.
*
* See XBASE\bbsp.h.
*
* Copyright (c) 2002-2025 Micro Digital Inc.
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
* Author: David Moore, Xingsheng Wan
*
*****************************************************************************/

#ifndef SMX_BSP_H
#define SMX_BSP_H

#include "armdefs.h"  /* for processor defines */

/* Configuration */

#if defined(SB_CPU_STM32F7XX) 
#define SB_USB_FS  0  /* Set to 1 if using USB full speed (on either USB controller) since PLL48 clock is used. */
#endif

#if defined(SB_CPU_STM32F7XX)
 #if SB_USB_FS
#define SB_CPU_HZ 192000000
 #else
#define SB_CPU_HZ 200000000
 #endif
#elif defined(SB_CPU_STM32U575)
#define SB_CPU_HZ 160000000
#else
#error Define SB_CPU_HZ for your processor here in bsp.h.
#endif

/* SB_CPU_MHZ: 3 variants are provided for fractional clock rates.
   Use the one that rounds properly for each purpose.
*/
#define SB_CPU_MHZ_DOWN  ((SB_CPU_HZ) / 1000000)             /* round down */
#define SB_CPU_MHZ_RND   (((SB_CPU_HZ) + 500000) / 1000000)  /* round to nearest */
#define SB_CPU_MHZ_UP    (((SB_CPU_HZ) + 999999) / 1000000)  /* round up */

/* Console Configuration. See documentation in XBASE\bbsp.h. */

#define SB_CON_BAUD 115200

#if SB_CFG_CON
#if defined(SB_BRD_STMICRO_STM32746GEVAL) || defined(SB_BRD_STMICRO_STM32F746G_DISCOVERY) || \
    defined(SB_BRD_STMICRO_STM32U575IEV)
#define SB_CON_IN_PORT       1  /* ports are numbered at 1 */
#define SB_CON_OUT_PORT      1  /* ports are numbered at 1 */
#else
#error Define SB_CFG_CON settings for your board here in bsp.h.
#endif
#endif /* SB_CFG_CON */

#if defined(SMX_TXPORT)
#define SB_LCD             0     /* keep 0 */
#elif defined(SB_BRD_STMICRO_STM32U575IEV)
#define SB_LCD             0     /* 1 if board has LCD display. 0 if not. */
#elif defined(SB_BRD_STMICRO_STM32)
#define SB_LCD             1     /* 1 if board has LCD display. 0 if not. */
#else
#define SB_LCD             0     /* 1 if board has LCD display. 0 if not. */
#endif

/* Defines */

#define SB_INT_MIN         0
#define SB_INT_MAX         (SB_IRQ_MAX+16)
#define SB_INT_NUM         (SB_INT_MAX-SB_INT_MIN+1)

#define SB_IRQ_MIN         0     /* see sb_irq_table[] in irqtable.c */
#if defined(SB_CPU_STM32F74X) || defined(SB_CPU_STM32F75X)
#define SB_IRQ_MAX         97
#elif defined(SB_CPU_STM32U5XX)
#define SB_IRQ_MAX         124
#else
#define SB_IRQ_MAX         59
#endif
#define SB_IRQ_NUM         (SB_IRQ_MAX-SB_IRQ_MIN+1)
#define SB_IRQ_PRI_MIN     INTC_VIC_MIN_PRIORITY
#define SB_IRQ_PRI_MAX     INTC_VIC_MAX_PRIORITY

#define SB_IRQ_PRI_BITS    0xF0  /* only 4 bits 7:4 are used for priority */
#define SB_IRQ_PRI_INCR    0x10  /* priority increment */

/* Max number of interrupt mask registers */
#define SB_IRQ_MASK_REGS ((SB_IRQ_MAX + 1 + 31) / 32)

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

#ifdef __cplusplus
extern "C" {
#endif

/* Function Prototypes */

bool sb_IRQTableEntryWrite(int irq_num, int pri);

#if defined(MW_FATFS)
u8 sbu_BSP_SD_Init(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* SMX_BSP_H */
