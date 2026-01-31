/*
* svc.h                                                     Version 6.0.0
*
* ARM-M SVC System call shell function macros
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

#ifndef SVC_H
#define SVC_H

#if SMX_CFG_SSMX

/* Note: In the following macros, r12 passes flags to smx_SVC_Handler via the
   exception frame.
      Bit0 = 1 means > 4 pars.
      Bit1 = 1 means deferred action function.
*/

/* Macro used for <= 4 parameters for non-heap functions. */
#undef  sb_SVC     /* replace the one in barmm.h since SMX_CFG_SSMX not yet defined there */
#define sb_SVC(id) \
   { \
      __asm("mov r12, #0"); \
      __asm("svc %0" : : "i" (id)); \
   }

/* Macro for > 4 parameters for non-heap functions. Pushes r4 so the stack is 
   consistent with sb_SVCH4() */
#define sb_SVCG4(id) \
   { \
      __asm("mov r12, #1"); \
      __asm("push {r4} \n\t"); \
      __asm("svc %0" : : "i" (id)); \
      __asm("pop {r4} \n\t"); \
   }

/* Same as sb_SVC, except loads temp da_enter flag into r12 */
#define sb_SVCda(id) \
   { \
      __asm("mov r12, #2"); \
      __asm("svc %0" : : "i" (id)); \
   }

/* Same as sb_SVCG4, except loads n and temp da_enter flag into r12 */
#define sb_SVCG4da(id) \
   { \
      __asm("mov r12, #3"); \
      __asm("push {r4} \n\t"); \
      __asm("svc %0" : : "i" (id)); \
      __asm("pop {r4} \n\t"); \
   }

/* Special macros for smxu_Heap functions. These assume that hmtx is free for
   the heap being accessed and make the heap call. If hmtx is free, the return 
   value is from the heap call. If hmtx is busy and timeout == 0, 0 is
   returned. If hmtx is busy and timeout > 0, the current task is suspended 
   on hmtx. If a timeout occurs, 0 is returned, else SMX_HEAP_RETRY is returned.
   In the latter case, the heap function is called again. The 3 parameter
   version uses r3 as a scratch register, which is volatile and need not be
   preserved. Others add a push and pop of r4.
*/
#define sb_SVCH(id) /* for 3 or less parameters */ \
   { \
      __asm("mov r12, #0"); \
      __asm("mov r3, r0 \n\t"); \
      __asm("svc %0 \n\t" : : "i" (id)); \
      __asm("cmp r0, %0 \n\t" : : "i" (SMX_HEAP_RETRY)); \
      __asm("itt eq \n\t" \
            "moveq r0, r3 \n\t" \
            "svceq %0 \n\t" : : "i" (id)); \
   }

#define sb_SVCH4(id) /* for 4 parameters */ \
   { \
      __asm("mov r12, #0"); \
      __asm("push {r4} \n\t"); \
      __asm("mov r4, r0 \n\t"); \
      __asm("svc %0 \n\t" : : "i" (id)); \
      __asm("cmp r0, %0 \n\t" : : "i" (SMX_HEAP_RETRY)); \
      __asm("itt eq \n\t" \
            "moveq r0, r4 \n\t" \
            "svceq %0 \n\t" : : "i" (id)); \
      __asm("pop {r4} \n\t"); \
   }

#define sb_SVCHG4(id) /* for greater than 4 parameters (in task stack) */ \
   { \
      __asm("mov r12, #1"); \
      __asm("push {r4} \n\t"); \
      __asm("mov r4, r0 \n\t"); \
      __asm("svc %0 \n\t" : : "i" (id)); \
      __asm("cmp r0, %0 \n\t" : : "i" (SMX_HEAP_RETRY)); \
      __asm("itt eq \n\t" \
            "moveq r0, r4 \n\t" \
            "svceq %0 \n\t" : : "i" (id)); \
      __asm("pop {r4} \n\t"); \
   }

#define NI _Pragma("inline=never")
#pragma diag_suppress=Pe940  /* suppress no return value warning */

#endif /* SMX_CFG_SSMX */

#endif /* SVC_H */