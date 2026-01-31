/*
* smxaware.h                                                Version 6.0.0
*
* smxAware and smxAware Live command defines and structures.
* This file is shared with the smxAware Live server (target) and client (PC).
*
* Copyright (c) 1999-2026 Micro Digital Inc.
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
* Author: Marty Cochran
*
*****************************************************************************/

#ifndef SMX_SMXAWARE_H
#define SMX_SMXAWARE_H

#if defined(SMXAWARE)

#define SMXAWARE_DATA_VERSION     0x400
#define ENABLE_SMXOBJECT_SYMBOLS  1

#define MP_MPU_TYPE_ARMM7  0x01
#define MP_MPU_TYPE_ARMM8  0x02
#define SB_PROCESSOR       (7 << 12)    /* ARMM */

/* build settings for smx_BuildFlags (32 bit flags max) */
#define SMX_CFG_TOKENS_DEFINED       0x0001

typedef struct
{
   u32  length;                         /* length of ring buffer */
   u32  next;                           /* next byte to write */
   u32  unused;
   u32  version;
   u8   rng_buf[SMX_SIZE_SA_PRT_RING+4];  /* +4 explained in Note 1 at the end of this file */
} SA_PRINT;

#if ((SA_PRINT_RING_SIZE/4) * 4 != SA_PRINT_RING_SIZE)
#error SA_PRINT_RING_SIZE must be divisible by 4
#endif

#if defined(SMXAWARE_LIVE)
#error Copy this section from v5.3.1 and update to match SMX especially smxLiveSymTable[].
#endif /* SMXAWARE_LIVE */

/*
   Notes:

   1. Each ring buffer entry starts with a u32 TimeOfDispatch. Following
      TimeOfDispatch is a null terminated string. TimeOfDispatch may run
      over the end of the buffer by up to 4 bytes so 4 bytes is added to
      the buffer size.
*/

#endif /* SMXAWARE */
#endif /* SMX_SMXAWARE_H */


