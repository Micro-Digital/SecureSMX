/*
* startup.c                                                 Version 6.0.0
*
* Startup code extension. Called early in the compiler's startup code.
* You can add any other early init code here, as desired.
*
* IAR:        Called by __iar_program_start() in cmain.s
* CrossWorks: Called by _start in thumb_crt0.s
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
* Author: David Moore
*
*****************************************************************************/

#include "bbase.h"
#include "bsp.h"

#if SMX_CFG_SSMX
#pragma section_prefix = ".sys"
#endif

#ifdef __cplusplus
extern "C" {
#endif

int __low_level_init(void)
{
   sb_MSFillResv(32);

   #if SB_CPU_ARMM8
   #pragma section = "CSTACK"
   __set_MSPLIM((u32)__section_begin("CSTACK") + SB_SIZE_MS_PAD);  /* pad for stack overflow handling */
   #endif

   /* USER: You can add any other early init code here. sb_PeripheralsInit()
            is another place that runs later. Decide which is appropriate.
   */

   return 1;  /* 1 tells IAR startup code to run seg_init; 0 skips */
}

#ifdef __cplusplus
}
#endif
