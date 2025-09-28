/*
* svc.h                                                     Version 5.4.0
*
* ARM-M SVC System call shell function macros
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

#ifndef SVC_H
#define SVC_H

#if SMX_CFG_SSMX

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