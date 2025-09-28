/*
* startup.c                                                 Version 5.4.0
*
* Startup code extension. Called early in the compiler's startup code.
* You can add any other early init code here, as desired.
*
* IAR:        Called by __iar_program_start() in cmain.s
* CrossWorks: Called by _start in thumb_crt0.s
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
