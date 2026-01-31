/*
* lpc55s69evk.h                                             Version 6.0.0
*
* Preinclude File for NXP LPC55S69-EVK board.
*
* Specifies global settings, such as the CPU type to ensure that all
* SMX libraries and the application are built consistently.
* The IDE project points to a prefix/preinclude file that includes it.
*
* Copyright (c) 2009-2026 Micro Digital Inc.
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
***********************************************************************/

/*
* Define the target board and processor so the code can check them.
*/
#define SB_BRD_NXP_LPC55S69_EVK
#define SB_CPU_LPC55S69
#define SB_CPU_LPC55S6X
#define SB_CPU_LPC55SXX

#define SB_CPU_ARMM
#define SB_CPU_ARMM7 0
#define SB_CPU_ARMM8 1

/* Special defines needed by NXP BSP. */
#define CPU_LPC55S69JBD100_cm33_core0  /* SMX running on core0 */

/*
* Set target processor type consistently in all projects. No pragma for it.
*/

