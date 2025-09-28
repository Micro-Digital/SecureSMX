/*
* bcfg.h                                                    Version 5.4.0
*
* smxBase configuration constants
*
* Copyright (c) 2010-2025 Micro Digital Inc.
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
* Authors: David Moore, Ralph Moore
*
*****************************************************************************/

#ifndef SB_BCFG_H
#define SB_BCFG_H

#define SB_CFG_CON               1  /* enable console operations */
#define SB_CFG_FIXED_WIDTH_TYPES 1  /* use int8_t, etc. to define u8, etc. */
#define SB_CFG_TM                1  /* enable time measurements <1> */

#if defined(SB_CPU_STM32)
#define SB_CFG_UARTI             1  /* interrupt-driven UART */
#else
#define SB_CFG_UARTI             0  /* polled UART */
#endif

#define SB_SIZE_MS_PAD        0x80  /* main stack pad size for stack overflow handling (ARMM8) */
#define SB_SIZE_OMB           1024  /* output message buffer size in bytes <2> */

/* Notes:
   1. Must also set in xarmm_iar.inc.
   2. Must be multiple of 16 for tdisp.c.
*/
#endif /* SB_BCFG_H */

