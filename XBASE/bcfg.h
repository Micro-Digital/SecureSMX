/*
* bcfg.h                                                    Version 6.0.0
*
* smxBase configuration constants
*
* Copyright (c) 2010-2026 Micro Digital Inc.
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

