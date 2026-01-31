/*
* stm32746geval.h                                           Version 6.0.0
*
* Preinclude File for STMicro STM32746G-EVAL Board
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
* Author: XiaoYi Chen
*
*****************************************************************************/

/*
* Define the target board and processor so the code can check them.
*/
#define SB_BRD_STMICRO_STM32746GEVAL
#define SB_BRD_STMICRO_STM32
#define SB_CPU_STM32F746NG
#define SB_CPU_STM32F746
#define SB_CPU_STM32F74X
#define SB_CPU_STM32F7XX
#define SB_CPU_STM32

#define SB_CPU_ARMM
#define SB_CPU_ARMM7 1
#define SB_CPU_ARMM8 0

/* Special defines needed by STMicro HAL Library. */
#define STM32F746xx 1           /* 1 so same as if defined on command line, but checked #if defined() */
//#define USE_STM32756G_EVAL_REVA /* Board Rev A. Comment out for Rev B. */
#define USE_HAL_DRIVER_V1_4     /* HAL BSP v1.4 or later */
#define USE_IOEXPANDER          /* Enable the IO expander */
#define DATA_IN_ExtSDRAM        /* Keep out SDRAM init code in HAL since we do in hwmain.c. */
#define HSE_VALUE 25000000      /* External osc/crystal Hz */ //USER: Set for your board

/*
* Set target processor type consistently in all projects. No pragma for it.
*/

