/*
* txcfg.h                                                   Version 5.4.0
*
* Application Configuration Constants for ThreadX
*
* See the SMX Quick Start for documentation of these settings.
*
* The following configuration constants work with pre-compiled smx libraries
* via the smx_cf structure, which is defined in main.c. See xsmx\xcfg.h for
* configuration constants that control the compilation of smx libraries.
*
* Copyright (c) 1989-2024 Micro Digital Inc.
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
* Authors: Ralph Moore and David Moore
*
*****************************************************************************/

#ifndef SMX_TXCFG_H
#define SMX_TXCFG_H 1

/* Sizes and Quantities */

#pragma section = "mheap"

#define SMX_HEAP_ADDRESS      ((u32)__section_begin("mheap")) /* heap starting address */

#if SMX_CFG_SSMX
/* For the MPU case, each middleware module has its own heap, so the sizes
   here are just for their task stacks and demo task stacks. They have to
   have to cover region alignment too for ARMM7, and some stacks are large.
*/
#define SMX_HEAP_SPACE        (28000 + SMX_SIZE_HT*8 /*sizeof(HTREC)*/ \
                               + SMX_NUM_STACKS*SMX_SIZE_STACK_BLK)
#else
#define SMX_HEAP_SPACE        (30000 + SMX_SIZE_HT*8 /*sizeof(HTREC)*/ \
                               + SMX_NUM_STACKS*SMX_SIZE_STACK_BLK)
#endif

/* Notes about sizes in HEAP_SPACE calc:
   First term is for demos, etc. Increase if necessary.
*/

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
#define SMX_NUM_STACKS          2   /* number of stacks in stack pool */
#define SMX_NUM_TASKS          30   /* includes Nulltask and Idle */
#define SMX_NUM_TIMERS          5   /* number of timer control blocks, TMRCBs */
#define SMX_NUM_XCHGS          15   /* number of message exchanges */

#define SMX_SIZE_HT            40   /* number of handles in the handle table */
#define SMX_SIZE_LQ           100
#define SMX_SIZE_SA_PRT_RING 1000  /* size of the smxAware print ring (bytes) */
#define SMX_SIZE_STACK_PAD     20  /* size of stack pads for all stacks <2> */
#define SMX_SIZE_STACK        500  /* size of stacks in the stack pool <2> */
#define SMX_SIZE_STACK_IDLE   (SMX_SIZE_STACK + 400) /* size of idle task stack (for init/exit too) <2> */

#if SMX_CFG_SSMX
#define SMX_SIZE_STACK_BLK   1024
#if SMX_SIZE_STACK_BLK < (SMX_SIZE_STACK + SMX_SIZE_STACK_PAD + SMX_RSA_SIZE)
#error Choose next larger power of 2 for STACK_BLK_SIZE.
#endif
#else
#define SMX_SIZE_STACK_BLK    ((SMX_SIZE_STACK + SMX_SIZE_STACK_PAD + SMX_RSA_SIZE + 7) & 0xFFFFFFF8)
#endif

#define SMX_TICKS_PER_SEC     100 

/* Safety Checks */
#if (SMX_SIZE_STACK_PAD % 4 != 0) || (SMX_SIZE_STACK % 4 != 0) || (SMX_SIZE_STACK_IDLE % 4 != 0)
#error SMX_CFG_STACK SIZE settings must be multiples of 4.
#endif

/* Notes:
   1. Must be a multiple of 8 and at least 24.
   2. Must be a multiple of 8.
   3. Replace default value with actual maximum dynamic chunk size for your application.
      Dynamic means a chunk which is allocated and freed as the application runs;
      it does not include chunks permanently allocated at the start.
*/

#endif /* SMX_TXCFG_H */
