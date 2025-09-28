/*
* stm32746geval.h                                           Version 5.4.0
*
* Preinclude File for STMicro STM32746G-EVAL Board
*
* Specifies global settings, such as the CPU type to ensure that all
* SMX libraries and the application are built consistently.
* The IDE project points to a prefix/preinclude file that includes it.
*
* Copyright (c) 2009-2025 Micro Digital Inc.
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

