/*
* smxaware.h                                                Version 5.4.0
*
* smxAware and smxAware Live command defines and structures.
* This file is shared with the smxAware Live server (target) and client (PC).
*
* Copyright (c) 1999-2025 Micro Digital Inc.
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


