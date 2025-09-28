/*
* acfg.h                                                    Version 5.4.0
*
* Application Configuration Constants.
*
* See the SMX Quick Start for documentation of these settings.
* See xcfg.h for configuration constants that control the compilation of smx.
*
* Copyright (c) 1989-2025 Micro Digital Inc.
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
* Authors: Ralph Moore, David Moore
*
*****************************************************************************/

#ifndef SMX_ACFG_H
#define SMX_ACFG_H

/* sizes and quantities */

#pragma section = "mheap"
#define SMX_HEAP_ADDRESS      ((u32)__section_begin("mheap"))
#define SMX_HEAP_SPACE        ((u32)__section_size("mheap"))
#define SMX_HEAP_DC_SIZE      (SMX_HEAP_SPACE/32 & ~0x7) /* initial donor chunk size <1> */
#define SMX_HEAP_CSZ_MAX      4136                       /* maximum dynamic chunk size <3> */
#define SMX_HEAP_USE_MAX      (SMX_HEAP_SPACE*3/4)       /* level to turn on cmerge */
#define SMX_HEAP_USE_MIN      (SMX_HEAP_USE_MAX - 256)   /* level to turn off cmerge */

#define SMX_NUM_BLOCKS         30   /* number of blocks of all sizes */
#define SMX_NUM_EQS             2
#define SMX_NUM_EGS             2
#define SMX_NUM_LSRS           10
#define SMX_NUM_MSGS           25
#define SMX_NUM_MTXS           14
#define SMX_NUM_PIPES           5
#define SMX_NUM_POOLS           5   /* number of block and msg pools */
#define SMX_NUM_SEMS           30
#define SMX_NUM_STACKS          5   /* number of stacks in stack pool */
#define SMX_NUM_TASKS          30   /* includes Nulltask and Idle */
#define SMX_NUM_TIMERS          5   /* number of timer control blocks, TMRCBs */
#define SMX_NUM_XCHGS          15   /* number of message exchanges */

#define SMX_SIZE_HT            40   /* number of handles in the handle table */
#define SMX_SIZE_LQ           100   /* size of LSR queue */
#define SMX_SIZE_SA_PRT_RING 1000   /* size of the smxAware print ring (bytes) */
#define SMX_SIZE_STACK_PAD    124   /* size of stack pads for all stacks <2> */
#define SMX_SIZE_STACK_IDLE   900   /* idle task stack size <2> */

/* stack pool stack sizes */
#if SMX_CFG_SSMX
 #if SB_CPU_ARMM7
#define SMX_SIZE_STACK_BLK   1024   /* must be a power of 2 */
 #elif SB_CPU_ARMM8
#define SMX_SIZE_STACK_BLK   1024   /* must be a multiple of 32 */
 #endif
#define SMX_SIZE_STACK       (SMX_SIZE_STACK_BLK - SMX_SIZE_STACK_PAD \
                                                       - SMX_RSA_SIZE)
#else /* !SMX_CFG_SSMX */
#define SMX_SIZE_STACK        500   /* stack pool stack size <2> */
#define SMX_SIZE_STACK_BLK   (SMX_SIZE_STACK + SMX_SIZE_STACK_PAD \
                                                   + SMX_RSA_SIZE)
#endif 

#define SMX_TICKS_PER_SEC     100 


/* demo configuration */

#if !defined(SMX_TXPORT)
#define SB_LCD_DEMO             1
#else
#define SB_LCD_DEMO             0
#endif

#define SB_FPU_DEMO             1

#if defined(MW_FATFS) && SMX_CFG_SSMX
#define MW_FATFS_DEMO           1
#else
#define MW_FATFS_DEMO           0
#endif


/* portal configuration */

#if SMX_CFG_PORTAL
#define CP_PORTAL               1  /* enable console partition portal */

 #if MW_FATFS_DEMO
#define FP_PORTAL               1  /* enable file system partition portal */
 #else
#define FP_PORTAL               0  /* keep 0 */
 #endif

#else   /* keep all 0 */
#define CP_PORTAL               0
#define FP_PORTAL               0
#endif /* SMX_CFG_PORTAL */


/* error checks */
#if ((SMX_SIZE_STACK_PAD + SMX_SIZE_STACK) % 8 != 0) || \
    ((SMX_SIZE_STACK_PAD + SMX_SIZE_STACK_IDLE) % 8 != 0)
#error Stacks must start on 8-byte boundaries.
#endif


/* Notes:
   1. Must be at least 24.
   2. SMX_SIZE_STACK      + SMX_SIZE_STACK_PAD must be a multiple of 8 and 
      SMX_SIZE_STACK_IDLE + SMX_SIZE_STACK_PAD must be a multiple of 8 and 
      stack pool must be aligned on an 8-byte boundary.
   3. Replace default value with actual value for application. Does not include 
      chunks permanently allocated at the start of operation.
*/
#endif /* SMX_ACFG_H */